#include "extension.h"

#include "CDetour/detours.h"

#define GAMEDATA_FILE "l4d2_weaponspawncontrol"
#define MELEE_LIST "fireaxe;frying_pan;machete;baseball_bat;crowbar;cricket_bat;tonfa;katana;electric_guitar;knife;golfclub"

#if defined PLATFORM_WINDOWS
typedef  void (__thiscall *tGame_KeyValues__SetString)(KeyValues* kv, const char*keyName, const char* value);
tGame_KeyValues__SetString Game_KeyValues__SetString;
#endif

CDetour *Detour_CTerrorGameRules__GetMissionInfo = NULL;
CDetour *Detour_CDirectorItemManager__IsMeleeWeaponAllowedToExist = NULL;
CDetour *Detour_CDirectorItemManager__IsWeaponAllowedToExist = NULL;

IGameConfig *g_pGameConf = NULL;

MeleeSpawnControl g_MeleeControl;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_MeleeControl);

DETOUR_DECL_STATIC0(CTerrorGameRules__GetMissionInfo, KeyValues*)
{
	KeyValues* result = DETOUR_STATIC_CALL(CTerrorGameRules__GetMissionInfo)();
	if (result){
#if !defined PLATFORM_WINDOWS
		result->SetString("meleeweapons", MELEE_LIST);
		result->SetInt("no_cs_weapons", 0);
#else
		Game_KeyValues__SetString(result, "meleeweapons", MELEE_LIST);
		result->SetInt("no_cs_weapons", 0);
#endif
	}
	return result;
}

DETOUR_DECL_MEMBER1(CDirectorItemManager__IsMeleeWeaponAllowedToExist, bool, char*, wscript_name)
{
	if (!strcmp(wscript_name, "knife")) return true;
	return DETOUR_MEMBER_CALL(CDirectorItemManager__IsMeleeWeaponAllowedToExist)(wscript_name);
}

DETOUR_DECL_MEMBER1(CDirectorItemManager__IsWeaponAllowedToExist, bool, char*, csweapon_name)
{
	return true;
}

bool MeleeSpawnControl::SDK_OnMetamodLoad( ISmmAPI *ismm, char *error, size_t maxlength, bool late )
{
	return true;
}

bool MeleeSpawnControl::SDK_OnLoad( char *error, size_t maxlength, bool late )
{
	char conf_error[255];
	if (!gameconfs->LoadGameConfigFile(GAMEDATA_FILE, &g_pGameConf, conf_error, sizeof(conf_error))){
		if (!strlen(conf_error)){
			snprintf(error, maxlength, "Could not read %s.txt: %s", GAMEDATA_FILE, conf_error);
		}  else {
			snprintf(error, maxlength, "Could not read %s.txt.", GAMEDATA_FILE);
		}
		return false;
	}
	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);	

#if defined PLATFORM_WINDOWS
	if (!g_pGameConf->GetMemSig("KeyValues::SetString",(void **)&Game_KeyValues__SetString)) {
		snprintf(error, maxlength, "Cannot get signature for KeyValues::SetString", GAMEDATA_FILE);
		g_pSM->LogError(myself, error);
		return false;
	}
#endif

	if (!SetupHooks(error, maxlength)) return false;


	return true;
}

void MeleeSpawnControl::SDK_OnUnload()
{
	RemoveHooks();
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool MeleeSpawnControl::SetupHooks(char *error, size_t maxlength)
{
	Detour_CTerrorGameRules__GetMissionInfo = DETOUR_CREATE_STATIC(CTerrorGameRules__GetMissionInfo, "CTerrorGameRules::GetMissionInfo");
	if (Detour_CTerrorGameRules__GetMissionInfo) {
		Detour_CTerrorGameRules__GetMissionInfo->EnableDetour();
	}else{
		snprintf(error, maxlength, "Cannot get signature for CTerrorGameRules::GetMissionInfo");
		g_pSM->LogError(myself, error);
		RemoveHooks();
		return false;
	}

	Detour_CDirectorItemManager__IsMeleeWeaponAllowedToExist = DETOUR_CREATE_MEMBER(CDirectorItemManager__IsMeleeWeaponAllowedToExist, "CDirectorItemManager::IsMeleeWeaponAllowedToExist");
	if (Detour_CDirectorItemManager__IsMeleeWeaponAllowedToExist) {
		Detour_CDirectorItemManager__IsMeleeWeaponAllowedToExist->EnableDetour();
	}else{
		snprintf(error, maxlength, "Cannot get signature for CDirectorItemManager::IsMeleeWeaponAllowedToExist");
		g_pSM->LogError(myself, error);
		RemoveHooks();
		return false;
	}

	Detour_CDirectorItemManager__IsWeaponAllowedToExist = DETOUR_CREATE_MEMBER(CDirectorItemManager__IsWeaponAllowedToExist, "CDirectorItemManager::IsWeaponAllowedToExist");
	if (Detour_CDirectorItemManager__IsWeaponAllowedToExist) {
		Detour_CDirectorItemManager__IsWeaponAllowedToExist->EnableDetour();
	}else{
		snprintf(error, maxlength, "Cannot get signature for CDirectorItemManager::IsWeaponAllowedToExist");
		g_pSM->LogError(myself, error);
		RemoveHooks();
		return false;
	}
	return true;
}

void MeleeSpawnControl::RemoveHooks()
{
	if (Detour_CTerrorGameRules__GetMissionInfo) {
		Detour_CTerrorGameRules__GetMissionInfo->DisableDetour();
	}

	if (Detour_CDirectorItemManager__IsMeleeWeaponAllowedToExist) {
		Detour_CDirectorItemManager__IsMeleeWeaponAllowedToExist->DisableDetour();
	}
	
	if (Detour_CDirectorItemManager__IsWeaponAllowedToExist) {
		Detour_CDirectorItemManager__IsWeaponAllowedToExist->DisableDetour();
	}
}
