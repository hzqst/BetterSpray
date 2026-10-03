import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('gamedata_sync', Path(__file__).parents[1] / 'sync-gamedata.py')
sync = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = sync
spec.loader.exec_module(sync)


class ConditionalPatchTests(unittest.TestCase):
    def test_module_scoped_numbered_patches_follow_conditions_and_stop_at_gap(self):
        document = {
            'schemaVersion': 1, 'name': 'fixture', 'gameVersions': ['hl-8684', 'hl-10210'],
            'symbols': {'engine': {'baseline': 'global'}},
            'conditionalGroups': [{
                'when': ['hl-8684'], 'symbols': {'gameui': {'ctor': 'function'}},
                'numberedPatchSets': [{'module': 'gameui', 'prefix': 'size_callsite'}, 'legacy'],
            }],
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'manifest.json'
            path.write_text(json.dumps(document), encoding='utf-8')
            manifest = sync.load_manifest(path)
        records = [
            {'module': module, 'symbolName': name}
            for module, name in [('gameui', 'size_callsite_0'), ('gameui', 'size_callsite_1'),
                                 ('gameui', 'size_callsite_3'), ('engine', 'size_callsite_2'),
                                 ('engine', 'legacy_0')]
        ]
        self.assertEqual({('engine', 'baseline'), ('gameui', 'ctor'),
                          ('gameui', 'size_callsite_0'), ('gameui', 'size_callsite_1'),
                          ('engine', 'legacy_0')}, sync.manifest_keep_set(manifest, records, 'hl-8684'))
        self.assertEqual({('engine', 'baseline')}, sync.manifest_keep_set(manifest, records, 'hl-10210'))


if __name__ == '__main__':
    unittest.main()
