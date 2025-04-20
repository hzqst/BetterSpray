#include <metahook.h>
#include <capstone.h>
#include <vector>
#include <set>
#include "plugins.h"
#include "privatehook.h"

//xref string "Failed to load custom decal for player"
#define DRAW_DECALTEXTURE_SIG_BLOB      "\x8B\x44\x24\x04\x56\x85\xC0\x57\x7D\x7B\x83\xC9\xFF\x2B\xC8\x8D\x04\x49"
#define DRAW_DECALTEXTURE_SIG_BLOB2     "\x8B\x4C\x24\x04\x56\x85\xC9\x57\x2A\x2A\x83\xC8\xFF\x2B\xC1"
#define DRAW_DECALTEXTURE_SIG_NEW       "\x55\x8B\xEC\x8B\x4D\x08\x56\x85\xC9\x57\x7D\x2A\x83\xC8\xFF\x2B\xC1\x8D\x0C\xC0"
#define DRAW_DECALTEXTURE_SIG_HL25      "\x55\x8B\xEC\x8B\x4D\x08\x85\xC9\x2A\x2A\xF7\xD1\x69\xD1\x50\x02\x00\x00"
#define DRAW_DECALTEXTURE_SIG_SVENGINE "\x8B\x4C\x24\x2A\x0F\xAE\xE8\x85\xC9\x2A\x2A\xF7\xD1"

private_funcs_t gPrivateFuncs = { 0 };

static hook_t* g_phook_Draw_DecalTexture = nullptr;

void Engine_FillAddress_Draw_DecalTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_DecalTexture)
		return;

	PVOID Draw_DecalTexture_VA = 0;

	{
		const char sigs[] = "Failed to load custom decal for player";
		auto Draw_DecalTexture_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Draw_DecalTexture_String)
			Draw_DecalTexture_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (Draw_DecalTexture_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C";
			*(DWORD*)(pattern + 1) = (DWORD)Draw_DecalTexture_String;
			auto Draw_DecalTexture_Call = Search_Pattern(pattern, DllInfo);
			if (Draw_DecalTexture_Call)
			{
				Draw_DecalTexture_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Draw_DecalTexture_Call, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					if (Candidate[0] == 0x81 &&
						Candidate[1] == 0xEC &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0x00 &&
						Candidate[6] == 0xA1)
						return TRUE;

					if (Candidate[0] == 0x8B &&
						Candidate[1] == 0x44 &&
						Candidate[2] == 0x24)
						return TRUE;

					if (Candidate[0] == 0x8B &&
						Candidate[1] == 0x4C &&
						Candidate[2] == 0x24)
						return TRUE;

					return FALSE;
					});

				gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.Draw_DecalTexture)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Draw_DecalTexture_VA = Search_Pattern(DRAW_DECALTEXTURE_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Draw_DecalTexture_VA = Search_Pattern(DRAW_DECALTEXTURE_SIG_HL25, DllInfo);
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			Draw_DecalTexture_VA = Search_Pattern(DRAW_DECALTEXTURE_SIG_NEW, DllInfo);
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			Draw_DecalTexture_VA = Search_Pattern(DRAW_DECALTEXTURE_SIG_BLOB, DllInfo);
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(Draw_DecalTexture);
}

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	
}

void Engine_InstallHooks()
{
	Install_InlineHook(Draw_DecalTexture);
}

void Engine_UninstallHooks()
{
	Uninstall_Hook(Draw_DecalTexture);
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{

}

void Client_InstallHooks()
{

}

void Client_UninstallHooks()
{

}

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo)
{
	if ((ULONG_PTR)addr > (ULONG_PTR)SrcDllInfo.ImageBase && (ULONG_PTR)addr < (ULONG_PTR)SrcDllInfo.ImageBase + SrcDllInfo.ImageSize)
	{
		auto addr_VA = (ULONG_PTR)addr;
		auto addr_RVA = RVA_from_VA(addr, SrcDllInfo);

		return (PVOID)VA_from_RVA(addr, TargetDllInfo);
	}

	return nullptr;
}

PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo)
{
	if ((ULONG_PTR)vftable > (ULONG_PTR)RealDllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)RealDllInfo.ImageBase + RealDllInfo.ImageSize)
	{
		ULONG_PTR vftable_VA = (ULONG_PTR)vftable;
		ULONG vftable_RVA = RVA_from_VA(vftable, RealDllInfo);
		auto vftable_DllInfo = (decltype(vftable))VA_from_RVA(vftable, DllInfo);

		auto vf_VA = (ULONG_PTR)vftable_DllInfo[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}
	else if ((ULONG_PTR)vftable > (ULONG_PTR)DllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)DllInfo.ImageBase + DllInfo.ImageSize)
	{
		auto vf_VA = (ULONG_PTR)vftable[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}

	return vftable[index];
}