#!/usr/bin/env python3
"""Download, validate, and atomically publish MetaHook gamedata."""

import argparse
import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
import time
import uuid
from contextlib import contextmanager
from dataclasses import dataclass
from http.client import HTTPException
from pathlib import Path
from typing import Dict, Iterator, List, Optional, Set, Tuple
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin, urlsplit
from urllib.request import Request, urlopen


DEFAULT_INDEX_URL = (
    "https://hlnd2t.github.io/GoldSrc_VibeSignatures/gamesymbols/index.json"
)
INDEX_URL_ENVIRONMENT_VARIABLE = "GOLDSRC_VIBESIGNATURES_INDEX_URL"
INDEX_FILE_NAME = "index.json"
SUPPORTED_INDEX_SCHEMA_VERSION = 4
SUPPORTED_SNAPSHOT_SCHEMA_VERSION = 5
SUPPORTED_SNAPSHOT_CONTRACT_VERSION = 8
SUPPORTED_ANALYSIS_OUTPUT_CONTRACT_VERSION = 3
MAXIMUM_INDEX_BYTES = 1024 * 1024
MAXIMUM_SNAPSHOT_BYTES = 16 * 1024 * 1024
MAXIMUM_VERSIONS = 4096
MAXIMUM_RECORDS = 100000
HTTP_TIMEOUT_SECONDS = 120
HTTP_ATTEMPTS = 3
HTTP_RETRY_DELAYS_SECONDS = (1, 2)
LOCK_TIMEOUT_SECONDS = 600
HTTP_USER_AGENT = "MetaHook-GameDataUpdater/1.0"
GAME_VERSION_PATTERN = re.compile(r"^[A-Za-z0-9_]+-[0-9]+[A-Za-z]*$")
LOWERCASE_SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")
SHA256_PATTERN = re.compile(r"^[0-9A-Fa-f]{64}$")

MANIFEST_SCHEMA_VERSION = 1

# Record kinds this consumer understands, mirroring GameData.cpp's Normalize*
# dispatch. A manifest may only reference these.
KIND_LITERALS = frozenset(
    {
        "function",
        "global",
        "patch",
        "scalar",
        "structMember",
        "virtualFunction",
        "vtable",
    }
)

# Directories inside the cache root. The cache is persistent (never cleaned
# between builds); only ephemeral staging directories are removed after publish.
CACHE_RAW_DIRECTORY = "raw"
CACHE_RAW_SNAPSHOT_DIRECTORY = "snapshots"
CACHE_RAW_INDEX_FILE_NAME = "index.json"

# Payload keys GameData.cpp actually reads, per record kind. Everything else in
# a payload is dropped during pruning; a manifest may drop any of these too via
# stripPayloadFields.
CONSUMED_PAYLOAD_FIELDS: Dict[str, Tuple[str, ...]] = {
    "function": (
        "func_rva",
        "func_size",
        "func_sig",
        "func_sig_allow_across_function_boundary",
    ),
    "global": (
        "gv_rva",
        "gv_sig",
        "gv_sig_va",
        "gv_va",
        "gv_inst_offset",
        "gv_inst_disp",
        "gv_inst_length",
        "gv_sig_allow_across_function_boundary",
    ),
    "patch": ("patch_rva",),
    "scalar": ("scalar_name", "scalar_value"),
    "structMember": ("struct_name", "member_name", "offset"),
    "virtualFunction": (
        "vfunc_index",
        "vtable_name",
        "func_rva",
        "func_size",
        "vfunc_sig",
        "func_sig",
        "vfunc_sig_allow_across_function_boundary",
        "func_sig_allow_across_function_boundary",
    ),
    "vtable": ("vtable_rva", "vtable_size", "vtable_symbol", "vtable_numvfunc"),
}

# Snapshot top-level members GameData.cpp reads. A manifest may drop any of
# these too (stripTopLevelFields is applied on top).
CONSUMED_TOP_LEVEL_FIELDS = ("schemaVersion", "source", "binaries", "records")

# `source` sub-fields GameData.cpp reads.
CONSUMED_SOURCE_FIELDS = ("snapshotSchemaVersion", "analysisOutputContractVersion")

# Per-record members GameData.cpp reads.
CONSUMED_RECORD_FIELDS = ("platform", "module", "symbolName", "kind", "payload")


class UpdateError(Exception):
    pass


class RetryableDownloadError(Exception):
    pass


class DuplicateJsonMemberError(ValueError):
    pass


@dataclass(frozen=True)
class SnapshotEntry:
    game_version: str
    file_name: str
    sha256: str
    size: int
    snapshot_schema_version: int
    file_count: int
    last_publish_time: str


@dataclass(frozen=True)
class GameDataIndex:
    raw: bytes
    entries: List[SnapshotEntry]


@dataclass(frozen=True)
class ConditionalGroup:
    """Symbols required only under a condition. `when` members are OR-ed
    conditions; an empty `when` matches every declared gameVersion. `exempt`
    names gameVersions the group does not apply to. `numbered_prefixes` are
    numbered patch sets (contiguous from _0) checked inside the group."""
    when: Tuple[str, ...]
    exempt: Tuple[str, ...]
    symbols: Dict[str, Dict[str, str]]  # moduleName -> {symbolName: expectedKind}
    numbered_prefixes: Tuple[Tuple[str, str], ...]


@dataclass(frozen=True)
class ConsumerManifest:
    """A consumer's declaration of which symbols it needs and which fields it
    reads. Shared by the launcher and by external plugins so the same
    synchronization and pruning pipeline can serve every consumer."""

    name: str
    index_url: Optional[str]
    game_versions: Tuple[str, ...]
    # moduleName -> {symbolName: expectedKind} for symbols required on every
    # declared gameVersion. A bare string kind is treated as module "engine".
    symbols: Dict[str, Dict[str, str]]
    # moduleName -> {symbolName: expectedKind} kept when present, not required.
    optional_symbols: Dict[str, Dict[str, str]]
    # (moduleName, prefix) numbered patch sets required on every gameVersion.
    numbered_patch_sets: Tuple[Tuple[str, str], ...]
    strip_payload_fields: Dict[str, Tuple[str, ...]]  # kind -> fields to drop
    strip_record_fields: Tuple[str, ...]
    strip_top_level_fields: Tuple[str, ...]
    # symbol -> gameVersions where it is deliberately absent and not required.
    symbol_exemptions: Dict[str, Tuple[str, ...]]
    # Groups where at least one member must be present (per module).
    alternative_groups: Tuple[Tuple[str, ...], ...]
    # Conditional requirement groups (engine-family / identity conditions).
    conditional_groups: Tuple[ConditionalGroup, ...]


def _flatten_module_symbols(value: object, field: str) -> Dict[str, Dict[str, str]]:
    """Normalise a symbols block into {module: {name: kind}}.

    Accepts either the flat form ({name: kind}, module defaults to "engine") or
    the module-scoped form ({module: {name: kind}}).
    """
    if value is None:
        return {}
    if not isinstance(value, dict):
        raise UpdateError("manifest {} must be an object".format(field))

    # Module-scoped when every value is a mapping (name.kind) or a list of names
    # (present-only, any kind). Otherwise the whole block is a flat engine map.
    module_scoped = len(value) > 0 and all(
        isinstance(v, (dict, list)) for v in value.values()
    )

    result: Dict[str, Dict[str, str]] = {}
    if module_scoped:
        for module, mapping in value.items():
            if not isinstance(module, str) or not module:
                raise UpdateError("manifest {} has an invalid module name".format(field))
            if isinstance(mapping, list):
                names: Dict[str, str] = {}
                for symbol in mapping:
                    if not isinstance(symbol, str) or not symbol:
                        raise UpdateError(
                            "manifest {}['{}'] contains an invalid symbol name".format(field, module)
                        )
                    names[symbol] = ""
                result[module] = names
            else:
                result[module] = _validate_kind_map(mapping, "{}['{}']".format(field, module))
    else:
        result["engine"] = _validate_kind_map(value, field)
    return result


def _validate_kind_map(value: object, field: str) -> Dict[str, str]:
    if not isinstance(value, dict):
        raise UpdateError("manifest {} must be an object".format(field))
    result: Dict[str, str] = {}
    for symbol, kind in value.items():
        if not isinstance(symbol, str) or not symbol:
            raise UpdateError("manifest {} contains an invalid symbol name".format(field))
        if kind is None:
            result[symbol] = ""  # present-only, any kind
            continue
        if kind not in KIND_LITERALS:
            raise UpdateError(
                "manifest {} entry '{}' has an unsupported kind: {!r}".format(field, symbol, kind)
            )
        result[symbol] = kind
    return result


def _parse_numbered_sets(value: object) -> Tuple[Tuple[str, str], ...]:
    """Normalise numberedPatchSets into (module, prefix) pairs. Accepts a bare
    string prefix (module "engine") or a {module, prefix} object."""
    if value is None:
        return ()
    if not isinstance(value, list):
        raise UpdateError("manifest numberedPatchSets must be an array")
    result: List[Tuple[str, str]] = []
    for item in value:
        if isinstance(item, str) and item:
            result.append(("engine", item))
        elif isinstance(item, dict):
            module = item.get("module", "engine")
            prefix = item.get("prefix")
            if not isinstance(module, str) or not module or not isinstance(prefix, str) or not prefix:
                raise UpdateError("manifest numberedPatchSets entry is invalid")
            result.append((module, prefix))
        else:
            raise UpdateError("manifest numberedPatchSets entry is invalid")
    return tuple(result)


def log(message: str) -> None:
    print("[MetaHook gamedata] {}".format(message), flush=True)


def require_python_version() -> None:
    if sys.version_info < (3, 8):
        raise UpdateError(
            "Python 3.8 or newer is required; current version is {}.{}.{}".format(
                sys.version_info.major,
                sys.version_info.minor,
                sys.version_info.micro,
            )
        )


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download, validate, and publish MetaHook gamedata."
    )
    parser.add_argument(
        "--target-dir",
        required=True,
        help="Destination metahook/gamedata directory.",
    )
    parser.add_argument(
        "--manifest",
        required=True,
        help="Consumer manifest (JSON) selecting the symbols to keep and the "
        "payload fields to strip.",
    )
    parser.add_argument(
        "--temp-root",
        help="Same-volume persistent cache directory: verified raw snapshots are "
        "reused across builds, and the last index is used to build offline.",
    )
    parser.add_argument(
        "--index-url",
        help=(
            "HTTPS index URL. Defaults to the manifest's indexUrl, then {} or "
            "the built-in catalog URL.".format(INDEX_URL_ENVIRONMENT_VARIABLE)
        ),
    )
    parser.add_argument(
        "--validate-only",
        action="store_true",
        help="Validate the existing target directory without using the network.",
    )
    return parser.parse_args()


def load_manifest(path: Path) -> ConsumerManifest:
    try:
        raw = Path(os.path.abspath(str(path))).read_bytes()
    except OSError as error:
        raise UpdateError("manifest is missing or unreadable: {} ({})".format(path, error)) from error

    document = parse_json_document(raw, "manifest")
    if not isinstance(document, dict):
        raise UpdateError("manifest root is not an object")

    schema_version = document.get("schemaVersion")
    if schema_version != MANIFEST_SCHEMA_VERSION:
        raise UpdateError(
            "manifest schemaVersion must be {}".format(MANIFEST_SCHEMA_VERSION)
        )

    name = document.get("name")
    if not isinstance(name, str) or not name:
        raise UpdateError("manifest name is missing or invalid")

    index_url = document.get("indexUrl")
    if index_url is not None:
        if not isinstance(index_url, str) or not index_url:
            raise UpdateError("manifest indexUrl must be a non-empty string when present")

    game_versions = document.get("gameVersions")
    if not isinstance(game_versions, list) or not game_versions:
        raise UpdateError("manifest gameVersions must be a non-empty array")
    seen_versions: Set[str] = set()
    for value in game_versions:
        if not isinstance(value, str) or not GAME_VERSION_PATTERN.fullmatch(value):
            raise UpdateError("manifest contains an invalid gameVersion: {!r}".format(value))
        if value in seen_versions:
            raise UpdateError("manifest contains duplicate gameVersion: {}".format(value))
        seen_versions.add(value)

    symbols = _flatten_module_symbols(document.get("symbols"), "symbols")
    if not symbols:
        raise UpdateError("manifest symbols must not be empty")

    optional_value = document.get("optionalSymbols", {})
    if isinstance(optional_value, list):
        # Accept a bare list of names for present-only symbols of any kind.
        optional_symbols: Dict[str, Dict[str, str]] = {"engine": {}}
        for symbol in optional_value:
            if not isinstance(symbol, str) or not symbol:
                raise UpdateError("manifest optionalSymbols contains an invalid symbol name")
            optional_symbols["engine"][symbol] = ""
    else:
        optional_symbols = _flatten_module_symbols(optional_value, "optionalSymbols")

    patch_sets = _parse_numbered_sets(document.get("numberedPatchSets", []))

    raw_strip_payload = document.get("stripPayloadFields", {})
    if not isinstance(raw_strip_payload, dict):
        raise UpdateError("manifest stripPayloadFields must be an object")
    strip_payload: Dict[str, Tuple[str, ...]] = {}
    for kind, fields in raw_strip_payload.items():
        if kind not in KIND_LITERALS:
            raise UpdateError(
                "manifest stripPayloadFields references an unsupported kind: {!r}".format(kind)
            )
        if not isinstance(fields, list) or not all(isinstance(f, str) and f for f in fields):
            raise UpdateError(
                "manifest stripPayloadFields['{}'] must be an array of field names".format(kind)
            )
        strip_payload[kind] = tuple(dict.fromkeys(fields))

    def require_string_list(value: object, field: str) -> Tuple[str, ...]:
        if value is None:
            return ()
        if not isinstance(value, list) or not all(isinstance(item, str) and item for item in value):
            raise UpdateError("manifest {} must be an array of field names".format(field))
        return tuple(dict.fromkeys(value))

    raw_exemptions = document.get("symbolExemptions", {})
    if not isinstance(raw_exemptions, dict):
        raise UpdateError("manifest symbolExemptions must be an object")
    symbol_exemptions: Dict[str, Tuple[str, ...]] = {}
    for symbol, versions in raw_exemptions.items():
        if not isinstance(symbol, str) or not symbol:
            raise UpdateError("manifest symbolExemptions contains an invalid symbol name")
        if not isinstance(versions, list) or not all(
            isinstance(v, str) and GAME_VERSION_PATTERN.fullmatch(v) for v in versions
        ):
            raise UpdateError(
                "manifest symbolExemptions['{}'] must be an array of gameVersions".format(symbol)
            )
        symbol_exemptions[symbol] = tuple(versions)

    raw_groups = document.get("alternativeGroups", [])
    if not isinstance(raw_groups, list):
        raise UpdateError("manifest alternativeGroups must be an array")
    alternative_groups: List[Tuple[str, ...]] = []
    for group in raw_groups:
        if (
            not isinstance(group, list)
            or len(group) < 2
            or not all(isinstance(member, str) and member for member in group)
        ):
            raise UpdateError(
                "manifest alternativeGroups entries must be arrays of >=2 symbol names"
            )
        alternative_groups.append(tuple(group))

    raw_conditional = document.get("conditionalGroups", [])
    if not isinstance(raw_conditional, list):
        raise UpdateError("manifest conditionalGroups must be an array")
    conditional_groups: List[ConditionalGroup] = []
    for index, group in enumerate(raw_conditional):
        field = "conditionalGroups[{}]".format(index)
        if not isinstance(group, dict):
            raise UpdateError("manifest {} must be an object".format(field))
        when = require_string_list(group.get("when"), "{} when".format(field))
        for value in when:
            if not GAME_VERSION_PATTERN.fullmatch(value):
                raise UpdateError("manifest {} contains an invalid gameVersion: {!r}".format(field, value))
        exempt = require_string_list(group.get("exempt"), "{} exempt".format(field))
        for value in exempt:
            if not GAME_VERSION_PATTERN.fullmatch(value):
                raise UpdateError("manifest {} contains an invalid gameVersion: {!r}".format(field, value))
        symbols_map = _flatten_module_symbols(group.get("symbols"), "{} symbols".format(field))
        if not symbols_map:
            raise UpdateError("manifest {} symbols must not be empty".format(field))
        conditional_groups.append(
            ConditionalGroup(
                when=when,
                exempt=exempt,
                symbols=symbols_map,
                numbered_prefixes=_parse_numbered_sets(group.get("numberedPatchSets")),
            )
        )

    return ConsumerManifest(
        name=name,
        index_url=index_url,
        game_versions=tuple(game_versions),
        symbols=symbols,
        optional_symbols=optional_symbols,
        numbered_patch_sets=patch_sets,
        strip_payload_fields=strip_payload,
        strip_record_fields=require_string_list(document.get("stripRecordFields"), "stripRecordFields"),
        strip_top_level_fields=require_string_list(document.get("stripTopLevelFields"), "stripTopLevelFields"),
        symbol_exemptions=symbol_exemptions,
        alternative_groups=tuple(alternative_groups),
        conditional_groups=tuple(conditional_groups),
    )


def manifest_keep_set(manifest: ConsumerManifest, records: List[object], game_version: str) -> Set[Tuple[str, str]]:
    """Resolve the exact set of (module, symbolName) keys to keep for one
    snapshot.

    Required symbols are always kept (absent ones are handled by the release
    validator, not here). Optional symbols are kept only when present. A
    numbered patch set is kept as a contiguous run starting at `_0`; the run
    ends at the first missing index, matching the consumer's enumeration.
    Conditional groups add (module, symbol) pairs for the gameVersions they
    apply to. Symbols with no module are treated as belonging to "engine".
    """
    present: Set[Tuple[str, str]] = {
        (record.get("module", "engine"), record.get("symbolName"))
        for record in records
        if isinstance(record, dict)
    }
    present_names = {name for _, name in present}

    keep: Set[Tuple[str, str]] = set()

    for module, symbol_map in manifest.symbols.items():
        for symbol in symbol_map:
            keep.add((module, symbol))
    for module, symbol_map in manifest.optional_symbols.items():
        for symbol in symbol_map:
            if symbol in present_names:
                keep.add((module, symbol))
    for module, prefix in manifest.numbered_patch_sets:
        index = 0
        while (module, "{}_{}".format(prefix, index)) in present:
            keep.add((module, "{}_{}".format(prefix, index)))
            index += 1

    for group in manifest.conditional_groups:
        if game_version in group.exempt:
            continue
        if group.when and game_version not in group.when:
            continue
        for module, symbol_map in group.symbols.items():
            for symbol in symbol_map:
                keep.add((module, symbol))
        for module, prefix in group.numbered_prefixes:
            index = 0
            while (module, "{}_{}".format(prefix, index)) in present:
                keep.add((module, "{}_{}".format(prefix, index)))
                index += 1

    return keep


def normalize_index_url(argument_value: Optional[str], manifest_value: Optional[str] = None) -> str:
    index_url = (argument_value or "").strip()
    if not index_url:
        index_url = os.environ.get(INDEX_URL_ENVIRONMENT_VARIABLE, "").strip()
    if not index_url:
        index_url = (manifest_value or "").strip()
    if not index_url:
        index_url = DEFAULT_INDEX_URL

    parsed = urlsplit(index_url)
    if parsed.scheme.lower() != "https" or not parsed.netloc:
        raise UpdateError("index URL must be an absolute HTTPS URL: {!r}".format(index_url))
    if parsed.username or parsed.password:
        raise UpdateError("index URL must not contain user information")
    if parsed.query or parsed.fragment:
        raise UpdateError("index URL must not contain a query string or fragment")
    if not parsed.path or parsed.path.endswith("/"):
        raise UpdateError("index URL must identify index.json")
    return index_url


def reject_duplicate_json_members(
    pairs: List[Tuple[str, object]],
) -> Dict[str, object]:
    result: Dict[str, object] = {}
    for name, value in pairs:
        if name in result:
            raise DuplicateJsonMemberError(
                "duplicate JSON object member: {}".format(name)
            )
        result[name] = value
    return result


def reject_json_constant(value: str) -> object:
    raise ValueError("invalid JSON constant: {}".format(value))


def parse_json_document(contents: bytes, description: str) -> object:
    try:
        text = contents.decode("utf-8")
    except UnicodeDecodeError as error:
        raise UpdateError("{} is not valid UTF-8: {}".format(description, error)) from error

    try:
        return json.loads(
            text,
            object_pairs_hook=reject_duplicate_json_members,
            parse_constant=reject_json_constant,
        )
    except (json.JSONDecodeError, DuplicateJsonMemberError, ValueError) as error:
        raise UpdateError("{} is not valid JSON: {}".format(description, error)) from error


def is_uint(value: object, maximum: int) -> bool:
    return (
        isinstance(value, int)
        and not isinstance(value, bool)
        and 0 <= value <= maximum
    )


def require_string(
    source: Dict[str, object],
    name: str,
    description: str,
    maximum_length: int,
) -> str:
    value = source.get(name)
    if not isinstance(value, str) or not value or len(value) > maximum_length:
        raise UpdateError(
            "{} contains a missing or invalid string field: {}".format(
                description,
                name,
            )
        )
    return value


def require_uint(
    source: Dict[str, object],
    name: str,
    description: str,
    maximum: int,
) -> int:
    value = source.get(name)
    if not is_uint(value, maximum):
        raise UpdateError(
            "{} contains a missing or invalid unsigned field: {}".format(
                description,
                name,
            )
        )
    return value


def _validate_optional_uint(
    source: Dict[str, object],
    name: str,
    description: str,
    maximum: int,
) -> Optional[int]:
    value = source.get(name)
    if value is None:
        return None
    if not is_uint(value, maximum):
        raise UpdateError(
            "{} contains an invalid unsigned field: {}".format(description, name)
        )
    return value


def _validate_optional_string(
    source: Dict[str, object],
    name: str,
    description: str,
    maximum_length: int,
) -> Optional[str]:
    value = source.get(name)
    if value is None:
        return None
    if not isinstance(value, str) or not value or len(value) > maximum_length:
        raise UpdateError(
            "{} contains an invalid string field: {}".format(description, name)
        )
    return value


def is_safe_snapshot_file_name(file_name: str) -> bool:
    return (
        6 <= len(file_name) <= 255
        and file_name not in (".", "..")
        and "\0" not in file_name
        and "/" not in file_name
        and "\\" not in file_name
        and ":" not in file_name
        and file_name.endswith(".json")
    )


def parse_index(contents: bytes, description: str) -> GameDataIndex:
    if not contents or len(contents) > MAXIMUM_INDEX_BYTES:
        raise UpdateError("{} size is invalid".format(description))

    document = parse_json_document(contents, description)
    if not isinstance(document, dict):
        raise UpdateError("{} root is not an object".format(description))

    schema_version = require_uint(
        document,
        "schemaVersion",
        description,
        0xFFFFFFFF,
    )
    if schema_version != SUPPORTED_INDEX_SCHEMA_VERSION:
        raise UpdateError(
            "{} schemaVersion {} is unsupported".format(description, schema_version)
        )

    versions = document.get("versions")
    if not isinstance(versions, list) or len(versions) > MAXIMUM_VERSIONS:
        raise UpdateError("{} contains an invalid versions array".format(description))

    entries: List[SnapshotEntry] = []
    seen_game_versions: Set[str] = set()
    seen_file_names: Set[str] = set()
    for ordinal, value in enumerate(versions):
        entry_description = "{} version entry {}".format(description, ordinal)
        if not isinstance(value, dict):
            raise UpdateError("{} is not an object".format(entry_description))

        game_version = require_string(value, "gameVersion", entry_description, 64)
        if not GAME_VERSION_PATTERN.fullmatch(game_version):
            raise UpdateError(
                "{} contains an invalid gameVersion: {!r}".format(
                    entry_description,
                    game_version,
                )
            )
        if game_version in seen_game_versions:
            raise UpdateError(
                "{} contains duplicate gameVersion: {}".format(
                    description,
                    game_version,
                )
            )
        seen_game_versions.add(game_version)

        file_name = require_string(value, "url", entry_description, 255)
        sha256 = require_string(value, "sha256", entry_description, 64)
        size = require_uint(value, "size", entry_description, MAXIMUM_SNAPSHOT_BYTES)
        snapshot_schema_version = require_uint(
            value,
            "snapshotSchemaVersion",
            entry_description,
            0xFFFFFFFF,
        )
        file_count = require_uint(
            value,
            "fileCount",
            entry_description,
            MAXIMUM_RECORDS,
        )
        last_publish_time = require_string(
            value,
            "lastPublishTime",
            entry_description,
            128,
        )

        if not is_safe_snapshot_file_name(file_name):
            raise UpdateError(
                "{} contains an unsafe snapshot URL: {!r}".format(
                    entry_description,
                    file_name,
                )
            )
        if file_name in seen_file_names:
            raise UpdateError(
                "{} contains duplicate snapshot URL: {}".format(
                    description,
                    file_name,
                )
            )
        seen_file_names.add(file_name)
        if not LOWERCASE_SHA256_PATTERN.fullmatch(sha256):
            raise UpdateError(
                "{} contains an invalid lowercase SHA-256".format(entry_description)
            )
        if size == 0:
            raise UpdateError("{} contains an invalid size".format(entry_description))
        if snapshot_schema_version != SUPPORTED_SNAPSHOT_CONTRACT_VERSION:
            raise UpdateError(
                "{} uses unsupported snapshot contract {}".format(
                    entry_description,
                    snapshot_schema_version,
                )
            )

        # Accept both the upstream content-addressed name
        # (<gameVersion>.<sha256>.json) and the stable pruned name
        # (<gameVersion>.json) this tool publishes.
        content_addressed_name = "{}.{}.json".format(game_version, sha256)
        pruned_name = "{}.json".format(game_version)
        if file_name not in (content_addressed_name, pruned_name):
            raise UpdateError(
                "{} URL is neither content-addressed nor stable; expected {!r} or {!r}".format(
                    entry_description,
                    content_addressed_name,
                    pruned_name,
                )
            )

        entries.append(
            SnapshotEntry(
                game_version=game_version,
                file_name=file_name,
                sha256=sha256,
                size=size,
                snapshot_schema_version=snapshot_schema_version,
                file_count=file_count,
                last_publish_time=last_publish_time,
            )
        )

    return GameDataIndex(raw=contents, entries=entries)


def validate_snapshot_contents(contents: bytes, entry: SnapshotEntry) -> None:
    description = "snapshot {}".format(entry.game_version)
    if len(contents) != entry.size:
        raise UpdateError(
            "{} size mismatch: expected {}, got {}".format(
                description,
                entry.size,
                len(contents),
            )
        )

    actual_sha256 = hashlib.sha256(contents).hexdigest()
    if actual_sha256 != entry.sha256:
        raise UpdateError(
            "{} SHA-256 mismatch: expected {}, got {}".format(
                description,
                entry.sha256,
                actual_sha256,
            )
        )

    document = parse_json_document(contents, description)
    if not isinstance(document, dict):
        raise UpdateError("{} root is not an object".format(description))

    schema_version = require_uint(
        document,
        "schemaVersion",
        description,
        0xFFFFFFFF,
    )
    if schema_version != SUPPORTED_SNAPSHOT_SCHEMA_VERSION:
        raise UpdateError(
            "{} schemaVersion {} is unsupported".format(description, schema_version)
        )

    source = document.get("source")
    if not isinstance(source, dict):
        raise UpdateError("{} source is not an object".format(description))

    source_game_version = require_string(
        source,
        "gameVersion",
        "{} source".format(description),
        64,
    )
    source_snapshot_schema_version = require_uint(
        source,
        "snapshotSchemaVersion",
        "{} source".format(description),
        0xFFFFFFFF,
    )
    analysis_output_contract_version = require_uint(
        source,
        "analysisOutputContractVersion",
        "{} source".format(description),
        0xFFFFFFFF,
    )
    if analysis_output_contract_version != SUPPORTED_ANALYSIS_OUTPUT_CONTRACT_VERSION:
        raise UpdateError(
            "{} uses unsupported analysis output contract {}".format(
                description,
                analysis_output_contract_version,
            )
        )

    # Provenance fields are present in the upstream snapshot and stripped by a
    # consumer manifest during pruning. They are validated when present but must
    # not be required, so both raw (cached) and pruned (published) snapshots pass.
    _validate_optional_uint(source, "configDigestVersion", "{} source".format(description), 0xFFFFFFFF)
    config_sha256 = _validate_optional_string(
        source, "configSha256", "{} source".format(description), 80
    )
    source_file_count = _validate_optional_uint(
        source, "fileCount", "{} source".format(description), MAXIMUM_RECORDS
    )
    _validate_optional_string(source, "lastPublishTime", "{} source".format(description), 128)

    if source_game_version != entry.game_version:
        raise UpdateError("{} source.gameVersion does not match index".format(description))
    if source_snapshot_schema_version != entry.snapshot_schema_version:
        raise UpdateError(
            "{} source.snapshotSchemaVersion does not match index".format(description)
        )
    if source_file_count is not None and source_file_count != entry.file_count:
        raise UpdateError("{} source.fileCount does not match index".format(description))
    if config_sha256 is not None and (
        not config_sha256.startswith("sha256:")
        or not SHA256_PATTERN.fullmatch(config_sha256[7:])
    ):
        raise UpdateError("{} source.configSha256 is invalid".format(description))

    records = document.get("records")
    if not isinstance(records, list) or len(records) != entry.file_count:
        raise UpdateError("{} records count does not match index".format(description))


def resolve_snapshot_url(index_url: str, file_name: str) -> str:
    snapshot_url = urljoin(index_url, file_name)
    index_parts = urlsplit(index_url)
    snapshot_parts = urlsplit(snapshot_url)
    if snapshot_parts.scheme.lower() != "https":
        raise UpdateError("snapshot URL must use HTTPS: {}".format(snapshot_url))
    if (
        snapshot_parts.hostname != index_parts.hostname
        or snapshot_parts.port != index_parts.port
    ):
        raise UpdateError("snapshot URL must not change host or port: {}".format(snapshot_url))
    return snapshot_url


def read_url_bytes(url: str, maximum_bytes: int) -> bytes:
    request = Request(
        url,
        headers={
            "Accept": "application/json",
            "User-Agent": HTTP_USER_AGENT,
        },
    )
    with urlopen(request, timeout=HTTP_TIMEOUT_SECONDS) as response:
        declared_length: Optional[int] = None
        content_length = response.headers.get("Content-Length")
        if content_length:
            try:
                parsed_length = int(content_length)
            except ValueError:
                parsed_length = -1
            if parsed_length >= 0:
                declared_length = parsed_length
            if declared_length is not None and declared_length > maximum_bytes:
                raise UpdateError(
                    "{} declares {} bytes, exceeding the {} byte limit".format(
                        url,
                        declared_length,
                        maximum_bytes,
                    )
                )

        contents = response.read(maximum_bytes + 1)
        if len(contents) > maximum_bytes:
            raise UpdateError("{} exceeds the {} byte limit".format(url, maximum_bytes))
        if declared_length is not None and len(contents) != declared_length:
            raise RetryableDownloadError(
                "{} returned {} bytes, but declared {}".format(
                    url,
                    len(contents),
                    declared_length,
                )
            )
        return contents


def format_download_error(error: BaseException) -> str:
    if isinstance(error, HTTPError):
        return "HTTP {} {}".format(error.code, error.reason)
    if isinstance(error, URLError):
        return str(error.reason)
    return str(error)


def fetch_index(url: str) -> bytes:
    last_error: Optional[BaseException] = None
    for attempt in range(1, HTTP_ATTEMPTS + 1):
        try:
            return read_url_bytes(url, MAXIMUM_INDEX_BYTES)
        except HTTPError as error:
            if 400 <= error.code < 500:
                raise UpdateError(
                    "index request failed without retry: {}".format(
                        format_download_error(error)
                    )
                ) from error
            last_error = error
        except UpdateError:
            raise
        except (
            RetryableDownloadError,
            URLError,
            HTTPException,
            OSError,
            TimeoutError,
        ) as error:
            last_error = error

        if attempt < HTTP_ATTEMPTS:
            delay = HTTP_RETRY_DELAYS_SECONDS[attempt - 1]
            log(
                "index download attempt {} failed: {}; retrying in {} second(s)".format(
                    attempt,
                    format_download_error(last_error),
                    delay,
                )
            )
            time.sleep(delay)

    raise UpdateError(
        "index download failed after {} attempts: {}".format(
            HTTP_ATTEMPTS,
            format_download_error(last_error),
        )
    )


def download_snapshot(
    url: str,
    destination: Path,
    entry: SnapshotEntry,
) -> None:
    last_error: Optional[BaseException] = None
    for attempt in range(1, HTTP_ATTEMPTS + 1):
        part_path = destination.with_name(destination.name + ".part")
        try:
            if part_path.exists():
                part_path.unlink()

            request = Request(
                url,
                headers={
                    "Accept": "application/json",
                    "User-Agent": HTTP_USER_AGENT,
                },
            )
            digest = hashlib.sha256()
            total_size = 0
            with urlopen(request, timeout=HTTP_TIMEOUT_SECONDS) as response:
                content_length = response.headers.get("Content-Length")
                if content_length:
                    try:
                        declared_length = int(content_length)
                    except ValueError:
                        declared_length = -1
                    if declared_length >= 0 and declared_length != entry.size:
                        raise RetryableDownloadError(
                            "Content-Length mismatch: expected {}, got {}".format(
                                entry.size,
                                declared_length,
                            )
                        )

                with part_path.open("wb") as output:
                    while True:
                        chunk = response.read(64 * 1024)
                        if not chunk:
                            break
                        total_size += len(chunk)
                        if total_size > entry.size:
                            raise RetryableDownloadError(
                                "download exceeded expected size {}".format(entry.size)
                            )
                        digest.update(chunk)
                        output.write(chunk)

            if total_size != entry.size:
                raise RetryableDownloadError(
                    "size mismatch: expected {}, got {}".format(
                        entry.size,
                        total_size,
                    )
                )
            actual_sha256 = digest.hexdigest()
            if actual_sha256 != entry.sha256:
                raise RetryableDownloadError(
                    "SHA-256 mismatch: expected {}, got {}".format(
                        entry.sha256,
                        actual_sha256,
                    )
                )

            part_path.replace(destination)
            validate_snapshot_contents(destination.read_bytes(), entry)
            return
        except HTTPError as error:
            if 400 <= error.code < 500:
                raise UpdateError(
                    "snapshot {} request failed without retry: {}".format(
                        entry.game_version,
                        format_download_error(error),
                    )
                ) from error
            last_error = error
        except RetryableDownloadError as error:
            last_error = error
        except UpdateError:
            raise
        except (URLError, HTTPException, OSError, TimeoutError) as error:
            last_error = error
        finally:
            if part_path.exists():
                part_path.unlink()

        if attempt < HTTP_ATTEMPTS:
            delay = HTTP_RETRY_DELAYS_SECONDS[attempt - 1]
            log(
                "snapshot {} download attempt {} failed: {}; retrying in {} second(s)".format(
                    entry.game_version,
                    attempt,
                    format_download_error(last_error),
                    delay,
                )
            )
            time.sleep(delay)

    raise UpdateError(
        "snapshot {} download failed after {} attempts: {}".format(
            entry.game_version,
            HTTP_ATTEMPTS,
            format_download_error(last_error),
        )
    )


def validate_target_path(target_dir: Path) -> Path:
    absolute_target = Path(os.path.abspath(str(target_dir)))
    # Accept the launcher's own catalog (<...>/metahook/gamedata) and a plugin
    # catalog nested under it (<...>/metahook/gamedata/<plugin>). Any other
    # location is refused so the synchronizer cannot publish somewhere unexpected.
    is_launcher_catalog = (
        absolute_target.name.lower() == "gamedata"
        and absolute_target.parent.name.lower() == "metahook"
    )
    is_nested_plugin_catalog = (
        absolute_target.parent.name.lower() == "gamedata"
        and absolute_target.parent.parent.name.lower() == "metahook"
    )
    if not (is_launcher_catalog or is_nested_plugin_catalog):
        raise UpdateError(
            "refusing to update unexpected target directory: {}".format(absolute_target)
        )
    if absolute_target.is_symlink():
        raise UpdateError(
            "refusing to replace a symbolic-link target directory: {}".format(
                absolute_target
            )
        )
    if absolute_target.exists() and not absolute_target.is_dir():
        raise UpdateError(
            "target path exists but is not a directory: {}".format(absolute_target)
        )
    return absolute_target


def validate_temp_root(temp_root: Path, target_dir: Path) -> Path:
    absolute_temp_root = Path(os.path.abspath(str(temp_root)))
    if absolute_temp_root == target_dir or target_dir in absolute_temp_root.parents:
        raise UpdateError(
            "temporary root must not be inside the target directory: {}".format(
                absolute_temp_root
            )
        )
    if absolute_temp_root.anchor.lower() != target_dir.anchor.lower():
        raise UpdateError("temporary root and target directory must be on the same volume")
    absolute_temp_root.mkdir(parents=True, exist_ok=True)
    if absolute_temp_root.is_symlink() or not absolute_temp_root.is_dir():
        raise UpdateError(
            "temporary root is not a regular directory: {}".format(absolute_temp_root)
        )
    return absolute_temp_root


@contextmanager
def acquire_update_lock(target_dir: Path) -> Iterator[None]:
    try:
        import msvcrt
    except ImportError as error:
        raise UpdateError("the gamedata updater requires Windows") from error

    target_key = os.path.normcase(str(target_dir)).encode("utf-8")
    lock_root = Path(tempfile.gettempdir()) / "MetaHookGameDataUpdater"
    lock_root.mkdir(parents=True, exist_ok=True)
    lock_name = "metahook-gamedata-{}.lock".format(
        hashlib.sha256(target_key).hexdigest()
    )
    lock_path = lock_root / lock_name
    lock_file = lock_path.open("a+b")
    acquired = False
    deadline = time.monotonic() + LOCK_TIMEOUT_SECONDS
    waiting_logged = False

    try:
        if lock_path.stat().st_size == 0:
            lock_file.write(b"\0")
            lock_file.flush()

        while not acquired:
            try:
                lock_file.seek(0)
                msvcrt.locking(lock_file.fileno(), msvcrt.LK_NBLCK, 1)
                acquired = True
            except OSError as error:
                if time.monotonic() >= deadline:
                    raise UpdateError(
                        "timed out waiting for the gamedata update lock after {} seconds".format(
                            LOCK_TIMEOUT_SECONDS
                        )
                    ) from error
                if not waiting_logged:
                    log("waiting for another gamedata updater")
                    waiting_logged = True
                time.sleep(0.25)

        yield
    finally:
        if acquired:
            lock_file.seek(0)
            msvcrt.locking(lock_file.fileno(), msvcrt.LK_UNLCK, 1)
        lock_file.close()


def read_local_index(target_dir: Path) -> Optional[bytes]:
    index_path = target_dir / INDEX_FILE_NAME
    if index_path.is_symlink() or not index_path.is_file():
        return None
    try:
        if index_path.stat().st_size > MAXIMUM_INDEX_BYTES:
            return None
        return index_path.read_bytes()
    except OSError:
        return None


def validate_local_snapshot(
    target_dir: Path,
    entry: SnapshotEntry,
) -> Tuple[bool, str]:
    snapshot_path = target_dir / entry.file_name
    if snapshot_path.is_symlink() or not snapshot_path.is_file():
        return False, "file is missing or is not a regular file"

    try:
        if snapshot_path.stat().st_size != entry.size:
            return False, "file size does not match index"
        contents = snapshot_path.read_bytes()
        validate_snapshot_contents(contents, entry)
    except (OSError, UpdateError) as error:
        return False, str(error)
    return True, ""


def collect_target_names(target_dir: Path) -> Set[str]:
    if not target_dir.exists():
        return set()
    try:
        return {entry.name for entry in target_dir.iterdir()}
    except OSError as error:
        raise UpdateError(
            "failed to inspect target directory {}: {}".format(target_dir, error)
        ) from error


def copy_reusable_snapshot(
    source: Path,
    destination: Path,
    entry: SnapshotEntry,
) -> bool:
    try:
        shutil.copy2(str(source), str(destination))
        validate_snapshot_contents(destination.read_bytes(), entry)
        return True
    except (OSError, UpdateError) as error:
        log(
            "snapshot {} could not be reused after staging: {}".format(
                entry.game_version,
                error,
            )
        )
        if destination.exists():
            destination.unlink()
        return False


def publish_stage(stage_dir: Path, target_dir: Path) -> None:
    target_dir.parent.mkdir(parents=True, exist_ok=True)
    backup_dir = target_dir.parent / ".gamedata.backup.{}.{}".format(
        os.getpid(),
        uuid.uuid4().hex,
    )
    had_target = target_dir.exists()

    try:
        if had_target:
            target_dir.replace(backup_dir)
        try:
            stage_dir.replace(target_dir)
        except Exception:
            if had_target and backup_dir.exists() and not target_dir.exists():
                backup_dir.replace(target_dir)
            raise

        if had_target and backup_dir.exists():
            try:
                shutil.rmtree(str(backup_dir))
            except OSError as error:
                log(
                    "WARNING: installed the new generation but could not remove "
                    "backup {}: {}".format(backup_dir, error)
                )
    except OSError as error:
        if had_target and backup_dir.exists() and not target_dir.exists():
            backup_dir.replace(target_dir)
        raise UpdateError("failed to publish gamedata atomically: {}".format(error)) from error


def validate_existing_directory(target_dir: Path) -> None:
    local_index_raw = read_local_index(target_dir)
    if local_index_raw is None:
        raise UpdateError("index.json is missing or invalid in {}".format(target_dir))
    local_index = parse_index(local_index_raw, "local index")

    expected_names = {INDEX_FILE_NAME}
    expected_names.update(entry.file_name for entry in local_index.entries)
    actual_names = collect_target_names(target_dir)
    if actual_names != expected_names:
        missing_names = sorted(expected_names - actual_names)
        extra_names = sorted(actual_names - expected_names)
        details = []
        if missing_names:
            details.append("missing: {}".format(", ".join(missing_names)))
        if extra_names:
            details.append("undeclared: {}".format(", ".join(extra_names)))
        raise UpdateError(
            "target directory contents differ from index ({})".format(
                "; ".join(details)
            )
        )

    for entry in local_index.entries:
        valid, reason = validate_local_snapshot(target_dir, entry)
        if not valid:
            raise UpdateError(
                "snapshot {} failed validation: {}".format(entry.game_version, reason)
            )

    log(
        "offline validation passed for {} ({} snapshot(s))".format(
            target_dir,
            len(local_index.entries),
        )
    )


def validate_manifest_coverage(target_dir: Path, manifest: ConsumerManifest) -> None:
    """Check that the packaged target actually carries the manifest's symbols."""
    local_index_raw = read_local_index(target_dir)
    if local_index_raw is None:
        raise UpdateError("index.json is missing or invalid in {}".format(target_dir))
    local_index = parse_index(local_index_raw, "local index")
    by_game_version = {entry.game_version: entry for entry in local_index.entries}

    missing_versions = [gv for gv in manifest.game_versions if gv not in by_game_version]
    if missing_versions:
        raise UpdateError(
            "target is missing manifest gameVersion(s): {}".format(", ".join(missing_versions))
        )

    for game_version in manifest.game_versions:
        entry = by_game_version[game_version]
        document = parse_json_document(
            (target_dir / entry.file_name).read_bytes(),
            "snapshot {}".format(game_version),
        )
        if not isinstance(document, dict):
            raise UpdateError("snapshot {} root is not an object".format(game_version))
        records = document.get("records")
        if not isinstance(records, list):
            raise UpdateError("snapshot {} records is not an array".format(game_version))
        present_names = {
            record.get("symbolName")
            for record in records
            if isinstance(record, dict)
        }
        # Group members may satisfy each other: if any member of an alternative
        # group is present, the others need not be (e.g. native cvar_hooks vs the
        # numbered callsite patch set).
        satisfied_by_group: Set[str] = set()
        for group in manifest.alternative_groups:
            if any(member in present_names for member in group):
                satisfied_by_group.update(group)

        required_names: Set[str] = set()
        for symbol_map in manifest.symbols.values():
            required_names.update(symbol_map)
        for group in manifest.conditional_groups:
            if game_version in group.exempt:
                continue
            if group.when and game_version not in group.when:
                continue
            for symbol_map in group.symbols.values():
                required_names.update(symbol_map)

        absent = [
            symbol
            for symbol in required_names
            if symbol not in present_names
            and symbol not in satisfied_by_group
            and game_version not in manifest.symbol_exemptions.get(symbol, ())
        ]
        if absent:
            raise UpdateError(
                "snapshot {} is missing required symbol(s): {}".format(
                    game_version, ", ".join(sorted(absent))
                )
            )

    log(
        "manifest '{}' coverage validated for {} ({} snapshot(s))".format(
            manifest.name, target_dir, len(manifest.game_versions)
        )
    )


def serialize_pruned(document: object) -> bytes:
    """Deterministic, compact JSON with a trailing newline so the pruned bytes
    (and therefore the index sha256) are stable across runs and platforms."""
    text = json.dumps(document, ensure_ascii=False, separators=(",", ":"), sort_keys=False)
    return (text + "\n").encode("utf-8")


def prune_record(record: object, manifest: ConsumerManifest) -> Optional[dict]:
    if not isinstance(record, dict):
        return None
    if record.get("platform") != "windows":
        return None

    pruned: dict = {}
    for field in CONSUMED_RECORD_FIELDS:
        if field in record and field not in manifest.strip_record_fields:
            pruned[field] = record[field]

    kind = record.get("kind")
    if kind in CONSUMED_PAYLOAD_FIELDS:
        payload = record.get("payload")
        if isinstance(payload, dict):
            stripped = set(manifest.strip_payload_fields.get(kind, ()))
            keep = [
                field
                for field in CONSUMED_PAYLOAD_FIELDS[kind]
                if field in payload and field not in stripped
            ]
            if keep:
                pruned["payload"] = {field: payload[field] for field in keep}

    return pruned


def prune_snapshot(document: dict, manifest: ConsumerManifest, game_version: str) -> dict:
    if document.get("schemaVersion") != SUPPORTED_SNAPSHOT_SCHEMA_VERSION:
        raise UpdateError("snapshot schemaVersion is unsupported for pruning")
    records = document.get("records")
    if not isinstance(records, list):
        raise UpdateError("snapshot records is not an array")

    keep_set = manifest_keep_set(manifest, records, game_version)

    kept_records = []
    used_modules = set()
    for record in records:
        if not isinstance(record, dict):
            continue
        key = (record.get("module", "engine"), record.get("symbolName"))
        if key not in keep_set:
            continue
        pruned = prune_record(record, manifest)
        if pruned is None:
            continue
        kept_records.append(pruned)
        if isinstance(pruned.get("module"), str):
            used_modules.add(pruned["module"])

    # Emit only the top-level members the manifest permits. Anything not
    # requested is dropped, so the C++ loader never reads a field this consumer
    # did not ask to keep (and therefore never fails on a pruned field).
    allowed_top = set(CONSUMED_TOP_LEVEL_FIELDS) - set(manifest.strip_top_level_fields)
    pruned_document: dict = {}
    for field in ("schemaVersion",):
        if field in allowed_top and field in document:
            pruned_document[field] = document[field]

    source = document.get("source")
    if "source" in allowed_top and isinstance(source, dict):
        allowed_source = set(CONSUMED_SOURCE_FIELDS) - set(manifest.strip_top_level_fields)
        pruned_source = {
            field: source[field]
            for field in CONSUMED_SOURCE_FIELDS
            if field in allowed_source and field in source
        }
        if isinstance(source.get("gameVersion"), str):
            pruned_source["gameVersion"] = source["gameVersion"]
        pruned_document["source"] = pruned_source

    binaries = document.get("binaries")
    if "binaries" in allowed_top and isinstance(binaries, dict):
        pruned_binaries: dict = {}
        for module in used_modules:
            entry = binaries.get(module)
            if not isinstance(entry, dict):
                continue
            windows = entry.get("windows")
            if not isinstance(windows, dict) or not isinstance(windows.get("crc64"), str):
                continue
            pruned_binaries[module] = {"windows": {"crc64": windows["crc64"]}}
        pruned_document["binaries"] = pruned_binaries

    if "records" in allowed_top:
        pruned_document["records"] = kept_records

    return pruned_document


def select_manifest_entries(
    index: GameDataIndex,
    manifest: ConsumerManifest,
) -> List[SnapshotEntry]:
    by_game_version = {entry.game_version: entry for entry in index.entries}
    missing = [gv for gv in manifest.game_versions if gv not in by_game_version]
    if missing:
        raise UpdateError(
            "manifest '{}' names gameVersion(s) absent from the index: {}".format(
                manifest.name, ", ".join(missing)
            )
        )
    return [by_game_version[gv] for gv in manifest.game_versions]


def ensure_cached_snapshot(
    cache_root: Path,
    target_dir: Path,
    index_url: str,
    entry: SnapshotEntry,
) -> tuple:
    """Return (path_to_valid_raw_snapshot, downloaded_bool).

    Reuse order: the cache (keyed by upstream sha256), then the current pruned
    target, then the network. The cache is authoritative because a pruned target
    file no longer matches the upstream content-addressed name.
    """
    cache_path = cache_root / CACHE_RAW_DIRECTORY / CACHE_RAW_SNAPSHOT_DIRECTORY / entry.file_name
    cache_path.parent.mkdir(parents=True, exist_ok=True)

    if cache_path.is_file() and not cache_path.is_symlink():
        valid, _ = validate_local_snapshot(cache_path.parent, entry)
        if valid:
            return cache_path, False

    existing_target = target_dir / entry.file_name
    if existing_target.is_file() and not existing_target.is_symlink():
        valid, _ = validate_local_snapshot(target_dir, entry)
        if valid and copy_reusable_snapshot(existing_target, cache_path, entry):
            return cache_path, False

    snapshot_url = resolve_snapshot_url(index_url, entry.file_name)
    log("downloading snapshot {} from {}".format(entry.game_version, snapshot_url))
    download_snapshot(snapshot_url, cache_path, entry)
    return cache_path, True


def update_game_data(
    manifest: ConsumerManifest,
    index_url: str,
    target_dir: Path,
    cache_root: Path,
) -> None:
    log("checking {}".format(index_url))
    remote_index_raw: Optional[bytes] = None
    used_offline_fallback = False
    try:
        remote_index_raw = fetch_index(index_url)
    except UpdateError as error:
        cached_index_path = cache_root / CACHE_RAW_DIRECTORY / CACHE_RAW_INDEX_FILE_NAME
        if cached_index_path.is_file() and not cached_index_path.is_symlink():
            remote_index_raw = cached_index_path.read_bytes()
            used_offline_fallback = True
            log("index download failed ({}); using the cached index".format(error))
        else:
            raise

    remote_index = parse_index(remote_index_raw, "remote index")
    entries = select_manifest_entries(remote_index, manifest)

    stage_dir = Path(tempfile.mkdtemp(prefix="metahook-gamedata-", dir=str(cache_root)))
    downloaded_count = 0
    try:
        pruned_versions = []
        for entry in entries:
            raw_path, downloaded = ensure_cached_snapshot(cache_root, target_dir, index_url, entry)
            if downloaded:
                downloaded_count += 1

            document = parse_json_document(raw_path.read_bytes(), "snapshot {}".format(entry.game_version))
            if not isinstance(document, dict):
                raise UpdateError("snapshot {} root is not an object".format(entry.game_version))

            pruned = prune_snapshot(document, manifest, entry.game_version)
            pruned_bytes = serialize_pruned(pruned)
            file_name = "{}.json".format(entry.game_version)
            (stage_dir / file_name).write_bytes(pruned_bytes)

            kept_records = pruned.get("records", [])
            pruned_versions.append(
                {
                    "gameVersion": entry.game_version,
                    "url": file_name,
                    "sha256": hashlib.sha256(pruned_bytes).hexdigest(),
                    "size": len(pruned_bytes),
                    "snapshotSchemaVersion": SUPPORTED_SNAPSHOT_CONTRACT_VERSION,
                    "fileCount": len(kept_records),
                    "lastPublishTime": entry.last_publish_time,
                }
            )

        pruned_index = {"schemaVersion": SUPPORTED_INDEX_SCHEMA_VERSION, "versions": pruned_versions}
        (stage_dir / INDEX_FILE_NAME).write_bytes(serialize_pruned(pruned_index))

        validate_existing_directory(stage_dir)
        publish_stage(stage_dir, target_dir)

        # Persist the index we built from so the next run can build offline.
        if not used_offline_fallback:
            cached_index_path = cache_root / CACHE_RAW_DIRECTORY / CACHE_RAW_INDEX_FILE_NAME
            cached_index_path.parent.mkdir(parents=True, exist_ok=True)
            cached_index_path.write_bytes(remote_index_raw)
    finally:
        if stage_dir.exists():
            shutil.rmtree(str(stage_dir))

    log(
        "published {} pruned snapshot(s) to {} (downloaded {}, kept symbols from manifest '{}')".format(
            len(entries),
            target_dir,
            downloaded_count,
            manifest.name,
        )
    )


def main() -> int:
    try:
        require_python_version()
        arguments = parse_arguments()
        manifest = load_manifest(Path(arguments.manifest))
        target_dir = validate_target_path(Path(arguments.target_dir))

        if arguments.validate_only:
            validate_existing_directory(target_dir)
            validate_manifest_coverage(target_dir, manifest)
            return 0

        if not arguments.temp_root:
            raise UpdateError("--temp-root is required unless --validate-only is used")
        cache_root = validate_temp_root(Path(arguments.temp_root), target_dir)
        index_url = normalize_index_url(arguments.index_url, manifest.index_url)
        with acquire_update_lock(target_dir):
            update_game_data(manifest, index_url, target_dir, cache_root)
        return 0
    except UpdateError as error:
        print(
            "[MetaHook gamedata] ERROR: {}".format(error),
            file=sys.stderr,
            flush=True,
        )
        return 1
    except Exception as error:
        print(
            "[MetaHook gamedata] ERROR: unexpected failure: {}".format(error),
            file=sys.stderr,
            flush=True,
        )
        return 1


if __name__ == "__main__":
    sys.exit(main())
