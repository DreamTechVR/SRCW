#include "Features.h"
#include <iostream>

typedef void* (*SteamUserStats_t)();
typedef bool  (*RequestCurrentStats_t)(void*);
typedef uint32_t (*GetNumAchievements_t)(void*);
typedef const char* (*GetAchievementName_t)(void*, uint32_t);
typedef bool  (*SetAchievement_t)(void*, const char*);
typedef bool  (*StoreStats_t)(void*);

bool UnlockSteamAchievements()
{
    HMODULE steamApi = GetModuleHandleA("steam_api64.dll");
    if (!steamApi) { std::cout << "[Achievements] steam_api64.dll not loaded\n"; return false; }
    auto fnGetStats = (SteamUserStats_t)GetProcAddress(steamApi, "SteamAPI_SteamUserStats_v012");
    if (!fnGetStats) fnGetStats = (SteamUserStats_t)GetProcAddress(steamApi, "SteamAPI_SteamUserStats_v011");
    if (!fnGetStats) { std::cout << "[Achievements] SteamUserStats not found\n"; return false; }
    void* userStats = fnGetStats();
    if (!userStats) { std::cout << "[Achievements] SteamUserStats null\n"; return false; }
    auto fnReq  = (RequestCurrentStats_t)GetProcAddress(steamApi, "SteamAPI_ISteamUserStats_RequestCurrentStats");
    auto fnNum  = (GetNumAchievements_t) GetProcAddress(steamApi, "SteamAPI_ISteamUserStats_GetNumAchievements");
    auto fnName = (GetAchievementName_t) GetProcAddress(steamApi, "SteamAPI_ISteamUserStats_GetAchievementName");
    auto fnSet  = (SetAchievement_t)     GetProcAddress(steamApi, "SteamAPI_ISteamUserStats_SetAchievement");
    auto fnStore= (StoreStats_t)         GetProcAddress(steamApi, "SteamAPI_ISteamUserStats_StoreStats");
    if (!fnReq || !fnNum || !fnName || !fnSet || !fnStore) { std::cout << "[Achievements] Missing API funcs\n"; return false; }
    fnReq(userStats); Sleep(500);
    uint32_t numAch = fnNum(userStats);
    if (!numAch) { std::cout << "[Achievements] 0 achievements found\n"; return false; }
    int unlocked = 0;
    for (uint32_t i = 0; i < numAch; i++) {
        const char* n = fnName(userStats, i);
        if (n && n[0] && fnSet(userStats, n)) unlocked++;
    }
    fnStore(userStats);
    std::cout << "[Achievements] Unlocked " << unlocked << "/" << numAch << "\n";
    return true;
}

void HookGame()
{
    bool bHooked = false;
    while (!bHooked) {
        auto World  = SDK::UWorld::GetWorld();
        auto Engine = SDK::UEngine::GetEngine();
        if (!World || !Engine || !Engine->GameViewport) continue;
        Orig_AActor_ProcessEvent = reinterpret_cast<AActor_ProcessEvent_t>(
            PatternScan("48 89 5C 24 10 48 89 6C 24 18 57 48 83 EC ? F7 82 B0 00 00 00 ? ? ? ?"));
        if (!Orig_AActor_ProcessEvent) continue;
        Orig_AActor_ProcessEvent = reinterpret_cast<AActor_ProcessEvent_t>(
            TrampHook64((BYTE*)Orig_AActor_ProcessEvent, (BYTE*)hk_AActor_ProcessEvent, 15));
        bHooked = true;
        if (cfg.HotkeyEnabled)
            std::cout << "[SRCW] Hooked! Press hotkey (0x" << std::hex << cfg.UnlockKey << std::dec << ") to unlock.\n";
        else
            std::cout << "[SRCW] Hooked! Will unlock on menu load.\n";
    }
}

void Clear()
{
    auto* contentData = Reflect::FindInstance("ContentDataAsset");
    auto* contentMgr  = Reflect::FindInstance("UnionContentManager");
    if (!contentData || !contentMgr) { bCleared = true; return; }

    SDK::FProperty* pkgListProp = Reflect::FindProperty(contentData->Class, "PackageDataList");
    if (!pkgListProp) { bCleared = true; return; }

    auto* arrProp    = static_cast<SDK::FArrayProperty*>(pkgListProp);
    auto* structProp = static_cast<SDK::FStructProperty*>(arrProp->InnerProperty);
    int32_t pidOffset = Reflect::FindPropertyOffset(structProp->Struct, "P_Id");
    int32_t elemSize  = arrProp->InnerProperty->ElementSize;
    if (pidOffset < 0 || elemSize <= 0) { bCleared = true; return; }

    void*    pkgListAddr = reinterpret_cast<uint8_t*>(contentData) + pkgListProp->Offset;
    int32_t  pkgCount    = ReflectRaw::TArrayNum(pkgListAddr);

    std::vector<int32_t> allPkgIds;
    for (int i = 0; i < pkgCount; i++)
        allPkgIds.push_back(ReflectRaw::ReadInt32(ReflectRaw::TArrayAt(pkgListAddr, i, elemSize), pidOffset));

    SDK::FProperty* mPkgProp = Reflect::FindProperty(contentMgr->Class, "m_packageIds");
    if (!mPkgProp || allPkgIds.empty()) { bCleared = true; return; }

    void*   mPkgAddr = reinterpret_cast<uint8_t*>(contentMgr) + mPkgProp->Offset;
    auto*   mPkgArr  = reinterpret_cast<ReflectRaw::RawTArray*>(mPkgAddr);

    std::vector<int32_t> existing;
    for (int i = 0; i < mPkgArr->NumElements; i++)
        existing.push_back(reinterpret_cast<int32_t*>(mPkgArr->Data)[i]);

    int32_t added = 0;
    for (auto pid : allPkgIds) {
        bool found = false;
        for (auto eid : existing) if (eid == pid) { found = true; break; }
        if (!found) { existing.push_back(pid); added++; }
    }

    if (added > 0) {
        int32_t nc = (int32_t)existing.size();
        if (nc > mPkgArr->MaxElements) {
            int32_t* nd = (int32_t*)VirtualAlloc(nullptr, nc * sizeof(int32_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (nd) { memcpy(nd, existing.data(), nc * sizeof(int32_t)); mPkgArr->Data = nd; mPkgArr->NumElements = nc; mPkgArr->MaxElements = nc; }
        } else {
            for (int32_t j = mPkgArr->NumElements; j < nc; j++)
                reinterpret_cast<int32_t*>(mPkgArr->Data)[j] = existing[j];
            mPkgArr->NumElements = nc;
        }
        Reflect::CallStaticBool("UnionContentUtils", "RequestCheckContent", true);
        std::cout << "[Clear] Injected " << added << " package IDs\n";
    }
    bCleared = true;
}

bool RunUnlockPhase(int phase)
{
    switch (phase) {
    case 0:  { if (cfg.HonorTitles)      { for (int i = 0; i < 500; i++) Reflect::CallStaticInt32("AppSaveGameHelper", "UnlockHonorTitle", i); std::cout << "[Phase 0] Honor titles\n"; } return true; }
    case 1:  { if (cfg.Drivers)          { int n = Reflect::GetEnumNum("EDriverId"); for (int i = 0; i < n; i++) { Reflect::CallStaticUInt8("AppSaveGameHelper", "SetDriverSelectable", (uint8_t)i); Reflect::CallStaticUInt8("AppSaveGameHelper", "ClearDriverNew", (uint8_t)i); } std::cout << "[Phase 1] Drivers (" << n << ")\n"; } return true; }
    case 2:  { if (cfg.MachineCustomize) { Reflect::CallStatic("MachineCustomizeUtilityLibrary", "StoreAllAura"); Reflect::CallStatic("MachineCustomizeUtilityLibrary", "StoreAllHorn"); Reflect::CallStatic("MachineCustomizeUtilityLibrary", "StoreAllMachineAssembly"); Reflect::CallStatic("MachineCustomizeUtilityLibrary", "StoreAllMachineParts"); Reflect::CallStatic("MachineCustomizeUtilityLibrary", "StoreAllSticker"); Reflect::CallStatic("MachineCustomizeUtilityLibrary", "UnlockGadgetAll"); std::cout << "[Phase 2] Machine customize\n"; } return true; }
    case 3:  { if (cfg.ColorPresets)     { int n = Reflect::GetEnumNum("EMachineId"); for (int i = 0; i < n; i++) Reflect::CallStaticUInt8("MachineCustomizeUtilityLibrary", "UnlockMachinePresetColor", (uint8_t)i); std::cout << "[Phase 3] Color presets\n"; } return true; }
    case 4:  { if (cfg.MirrorSpeed)      { Reflect::CallStaticBool("AppSaveGameHelper", "SetOpenMirror", true); Reflect::CallStaticBool("AppSaveGameHelper", "SetOpenSuperSonicSpeed", true); std::cout << "[Phase 4] Mirror & SSS\n"; } return true; }
    case 5:  { if (cfg.Music)            { for (int i = 0; i < 200; i++) Reflect::CallStaticInt32("AppSaveGameHelper", "SetAlbumAvailable", i); for (int i = 0; i < 500; i++) Reflect::CallStaticInt32("AppSaveGameHelper", "SetTrackAvailable", i); std::cout << "[Phase 5] Music\n"; } return true; }
    case 6:  { if (cfg.StagesDLC)        { Reflect::CallStaticBool("UnionContentUtils", "RequestCheckContent", true); std::cout << "[Phase 6] DLC stages\n"; } return true; }
    case 7:  {
        if (cfg.StagesGPOpen) {
            auto* gp = Reflect::FindCDO("CheatGrandPrix");
            if (gp) { Reflect::CallInstanceBool(gp, "CheatGrandPrix", "SetGrandPrixOpenedAll", true); Reflect::CallInstanceBool(gp, "CheatGrandPrix", "SetSetGrandPrixLeast1Play", true); }
            int n = Reflect::GetEnumNum("EGrandPrixId"); for (int i = 1; i < n; i++) Reflect::CallStaticUInt8Bool("AppSaveGameHelper", "SetGrandPrixLeast1PlayEachGrandPrix", (uint8_t)i, true);
            int ne = Reflect::GetEnumNum("EGrandPrixEventFlag"); for (int i = 0; i < ne; i++) Reflect::CallStaticUInt8Bool("AppSaveGameHelper", "SetDodonpaEventCompleteFlag", (uint8_t)i, true);
            Reflect::CallStaticBool("AppSaveGameHelper", "SetGrandPrixLeast1Play", true);
            std::cout << "[Phase 7] GP opened\n"; }
        return true; }
    case 8:  {
        if (cfg.StagesSecret) {
            auto* gp = Reflect::FindCDO("CheatGrandPrix");
            if (gp) { Reflect::CallInstance(gp, "CheatGrandPrix", "SetPlayedAnotherStageAll"); Reflect::CallInstanceUInt8Bool(gp, "CheatGrandPrix", "SetClearedGrandPrixEnding", 0, true); Reflect::CallInstanceUInt8Bool(gp, "CheatGrandPrix", "SetClearedGrandPrixEnding", 1, true); }
            std::cout << "[Phase 8] Secret stages\n"; }
        return true; }
    case 9:  { if (cfg.GadgetPlate)  { Reflect::CallStaticUInt8("MachineCustomizeUtilityLibrary", "SetCurrentGadgetPlateIdUseId", 7); Reflect::CallStatic("MachineCustomizeUtilityLibrary", "UpdateGadgetSlotNumInUserData"); std::cout << "[Phase 9] Gadget plate\n"; } return true; }
    case 10: { if (cfg.Challenges)   { Reflect::CallStatic("CheatChallenge", "AllChallengeClear"); Reflect::CallStaticBool("AppSaveGameHelper", "SetCompleteMainChallenge", true); Reflect::CallStaticBool("AppSaveGameHelper", "SetCompleteSpecialChallenge", true); int32_t pc = Reflect::CallStaticRetInt32("ChallengeStatsUtility", "GetChallengeProgressCount"); Reflect::CallStaticInt32("AppSaveGameHelper", "SetChallengeShowProgress", pc); Reflect::CallStaticFloat("AppSaveGameHelper", "SetChallengeLastShowProgress", 1.0f); std::cout << "[Phase 10] Challenges\n"; } return true; }
    case 11: {
        if (cfg.SuperSonicAll) {
            Reflect::CallStaticUInt8("AppSaveGameHelper", "SetDriverSelectable", 46);
            Reflect::CallStaticBool("AppSaveGameHelper", "SetOpenSuperSonicSpeed", true);
            auto* gp = Reflect::FindCDO("CheatGrandPrix");
            if (gp) Reflect::CallInstanceBool(gp, "CheatGrandPrix", "SetOpenFever", true);
            std::cout << "[Phase 11] Super Sonic all modes\n"; }
        return true; }
    case 12: { if (cfg.NF_CompleteMachine) Reflect::CallStatic("MachineCustomizeUtilityLibrary", "DisableCompleteMachineNewFlags"); if (cfg.NF_Sticker) Reflect::CallStatic("MachineCustomizeUtilityLibrary", "DisableStickerNewFlags"); if (cfg.NF_ColorPreset) Reflect::CallStatic("MachineCustomizeUtilityLibrary", "DisableMachineColorPresetNewFlags"); if (cfg.NF_Gadget) Reflect::CallStatic("MachineCustomizeUtilityLibrary", "DisableDisplayedGadgetNewFlags"); std::cout << "[Phase 12] Machine flags\n"; return true; }
    case 13: { if (cfg.NF_PartsSpeed)  Reflect::CallStaticUInt8("MachineCustomizeUtilityLibrary", "DisablePartsListNewFlagByType", 0); return true; }
    case 14: { if (cfg.NF_PartsAccel)  Reflect::CallStaticUInt8("MachineCustomizeUtilityLibrary", "DisablePartsListNewFlagByType", 1); return true; }
    case 15: { if (cfg.NF_PartsHandle) Reflect::CallStaticUInt8("MachineCustomizeUtilityLibrary", "DisablePartsListNewFlagByType", 2); return true; }
    case 16: { if (cfg.NF_PartsPower)  Reflect::CallStaticUInt8("MachineCustomizeUtilityLibrary", "DisablePartsListNewFlagByType", 3); return true; }
    case 17: { if (cfg.NF_PartsDash)   Reflect::CallStaticUInt8("MachineCustomizeUtilityLibrary", "DisablePartsListNewFlagByType", 4); return true; }
    case 18: { if (cfg.NF_Horn) { int n = Reflect::GetEnumNum("EMachineHornType"); for (int i = 0; i < n; i++) Reflect::CallStaticUInt8Bool("MachineCustomizeUtilityLibrary", "SetCustomMachineHornNew", (uint8_t)i, false); std::cout << "[Phase 18] Horns\n"; } return true; }
    case 19: { if (cfg.NF_HonorTitles) { for (int i = 0; i < 500; i++) Reflect::CallStaticInt32("AppSaveGameHelper", "ResetNewHonorTitle", i); std::cout << "[Phase 19] Title flags\n"; } return true; }
    case 20: { if (cfg.NF_Challenges) { int32_t pc = Reflect::CallStaticRetInt32("ChallengeStatsUtility", "GetChallengeProgressCount"); Reflect::CallStaticInt32("AppSaveGameHelper", "SetChallengeShowProgress", pc); Reflect::CallStaticFloat("AppSaveGameHelper", "SetChallengeLastShowProgress", 1.0f); } if (cfg.NF_Rewards) Reflect::CallStatic("AppSaveGameHelper", "ClearRewardGetDisplayRequestDataAll"); std::cout << "[Phase 20] Challenge/reward flags\n"; return true; }
    case 21: { if (cfg.Achievements) { UnlockSteamAchievements(); std::cout << "[Phase 21] Achievements\n"; } return true; }
    default: return false;
    }
}

// ---------------------------------------------------------------------------
// Super Sonic All-Modes: Character Select Screen Patch
//
// The screenshots confirm that Super Sonic's slot is ABSENT from the CSS
// grid in Time Trial / Race Park — the widget that builds the driver list
// simply doesn't include driver 46 in non-Grand Prix modes.
//
// The fix has three layers, all inside hk_AActor_ProcessEvent:
//
// LAYER 1 — IsDriverSelectable post-call patch
//   Forces return value to true for driver 46 so clicks are accepted.
//
// LAYER 2 — Driver list injection (post-call on SetDriverIdList / equivalent)
//   WBP_CharaSelect_Sub_Window_C builds its icon list by calling a function
//   that takes or sets a TArray<uint8> of driver IDs (e.g. SetDriverIdList,
//   InitDriverList, or BuildCharaList). We intercept this post-call and,
//   if driver 46 is not in the output array property, append it.
//
// LAYER 3 — CSS window init event intercept (post-call)
//   On any Init/Construct event fired by a WBP_CharaSelect_Sub_Window_C
//   object, we call AddDriverIcon(46) or equivalent to force the slot to
//   be created. This is a fallback if layer 2 misses the exact function.
// ---------------------------------------------------------------------------

static SDK::UFunction* s_IsDriverSelectableFunc  = nullptr;

// Candidate function names the CSS window uses to build its driver list.
// We search for each at runtime and cache the first one found.
static SDK::UFunction* s_SetDriverIdListFunc     = nullptr;
static SDK::UFunction* s_InitDriverListFunc      = nullptr;
static SDK::UFunction* s_BuildCharaListFunc      = nullptr;
static SDK::UFunction* s_AddDriverIconFunc       = nullptr;

// Candidate CSS window event names that trigger a roster rebuild
static const char* kCSSWindowInitEvents[] = {
    "OnInitStateSelectPlayMode",
    "InitCharaSelectWindow",
    "BuildCharaSelectList",
    "OnConstruct",
    nullptr
};

// Lazy-resolve all driver-list related functions from the CSS window class.
// Called once after the first CSS window event fires.
static bool s_DriverListFuncsResolved = false;
static void ResolveDriverListFuncs()
{
    if (s_DriverListFuncsResolved) return;
    s_DriverListFuncsResolved = true;

    // Try several plausible class/function name combinations.
    // We try both the _C (Blueprint) and plain variants.
    static const char* classes[] = {
        "WBP_CharaSelect_Sub_Window_C",
        "WBP_CharaSelect_Sub_Window",
        "BPC_CharaSelectState_C",
        "BPC_CharaSelectState",
        "CharaSelectUtilityLibrary",
        nullptr
    };
    static const char* listFuncs[] = {
        "SetDriverIdList", "InitDriverList", "SetDisplayDriverIds",
        "SetCharaIdList",  "BuildDriverList","SetSelectDriverList",
        "SetShowDriverList","SetDispDriverList",
        nullptr
    };
    static const char* addFuncs[] = {
        "AddDriverIcon", "AddCharaIcon", "AddDriverSlot",
        "CreateDriverIcon", "SpawnCharaIcon",
        nullptr
    };

    for (int ci = 0; classes[ci]; ci++) {
        for (int fi = 0; listFuncs[fi]; fi++) {
            if (!s_SetDriverIdListFunc)
                s_SetDriverIdListFunc = Reflect::FindFunction(classes[ci], listFuncs[fi]);
        }
        for (int fi = 0; addFuncs[fi]; fi++) {
            if (!s_AddDriverIconFunc)
                s_AddDriverIconFunc = Reflect::FindFunction(classes[ci], addFuncs[fi]);
        }
    }

    if (s_SetDriverIdListFunc)
        std::cout << "[SuperSonic] Resolved driver list func: " << s_SetDriverIdListFunc->GetName() << "\n";
    if (s_AddDriverIconFunc)
        std::cout << "[SuperSonic] Resolved add icon func:    " << s_AddDriverIconFunc->GetName() << "\n";
    if (!s_SetDriverIdListFunc && !s_AddDriverIconFunc)
        std::cout << "[SuperSonic] Warning: no driver list func found — using TArray scan fallback\n";
}

// Layer 1: patch IsDriverSelectable return value post-call
static void PatchIsDriverSelectable(SDK::UFunction* Function, void* Parms)
{
    if (!s_IsDriverSelectableFunc)
        s_IsDriverSelectableFunc = Reflect::FindFunction("CharaSelectUtilityLibrary", "IsDriverSelectable");
    if (!s_IsDriverSelectableFunc || Function != s_IsDriverSelectableFunc) return;

    int32_t driverIdOffset  = -1;
    int32_t returnValOffset = -1;
    for (SDK::FField* field = Function->ChildProperties; field; field = field->Next) {
        auto* prop    = static_cast<SDK::FProperty*>(field);
        bool isReturn = (prop->PropertyFlags & static_cast<uint64_t>(SDK::EPropertyFlags::ReturnParm)) != 0;
        if (isReturn && returnValOffset < 0)                            returnValOffset = prop->Offset;
        else if (!isReturn && driverIdOffset < 0 && prop->ElementSize <= 4) driverIdOffset  = prop->Offset;
    }
    if (driverIdOffset < 0 || returnValOffset < 0) return;

    uint8_t* bytes    = static_cast<uint8_t*>(Parms);
    int32_t  driverId = static_cast<int32_t>(*reinterpret_cast<uint8_t*>(bytes + driverIdOffset));
    if (driverId == 46)
        *reinterpret_cast<bool*>(bytes + returnValOffset) = true;
}

// Layer 2: after SetDriverIdList (or equivalent) returns, scan the output
// TArray<uint8> property on the object and append driver 46 if absent.
static void PatchDriverIdListOutput(SDK::AActor* Caller, SDK::UFunction* Function, void* Parms)
{
    if (!s_SetDriverIdListFunc || Function != s_SetDriverIdListFunc) return;

    SDK::UObject* obj = reinterpret_cast<SDK::UObject*>(Caller);
    if (!obj || !obj->Class) return;

    // Find a TArray property that looks like a driver ID list on this object.
    // It will be a TArray whose InnerProperty is uint8 or int32 sized.
    for (SDK::FField* field = obj->Class->ChildProperties; field; field = field->Next) {
        auto* prop = static_cast<SDK::FProperty*>(field);
        // Look for array properties
        if (prop->ElementSize < 16) continue; // TArray is 16 bytes
        std::string pname = prop->Name.ToString();
        // Heuristic: name contains "Driver", "Chara", "Id", "List", "Ids"
        bool looksLikeList = (pname.find("Driver") != std::string::npos ||
                              pname.find("Chara")  != std::string::npos ||
                              pname.find("Id")     != std::string::npos ||
                              pname.find("List")   != std::string::npos);
        if (!looksLikeList) continue;

        void* arrAddr = reinterpret_cast<uint8_t*>(obj) + prop->Offset;
        auto* arr     = reinterpret_cast<ReflectRaw::RawTArray*>(arrAddr);
        if (!arr->Data || arr->NumElements <= 0 || arr->NumElements > 512) continue;

        // Check if all values fit in uint8 range (driver IDs are 0-255)
        uint8_t* data = reinterpret_cast<uint8_t*>(arr->Data);
        bool allByte  = true;
        for (int i = 0; i < arr->NumElements; i++)
            if (data[i] > 200) { allByte = false; break; }
        if (!allByte) continue;

        // Check driver 46 isn't already in the list
        bool has46 = false;
        for (int i = 0; i < arr->NumElements; i++)
            if (data[i] == 46) { has46 = true; break; }
        if (has46) continue;

        // Append driver 46
        int32_t nc = arr->NumElements + 1;
        if (nc <= arr->MaxElements) {
            data[arr->NumElements] = 46;
            arr->NumElements = nc;
        } else {
            uint8_t* nd = (uint8_t*)VirtualAlloc(nullptr, nc, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (nd) {
                memcpy(nd, data, arr->NumElements);
                nd[arr->NumElements] = 46;
                arr->Data        = nd;
                arr->NumElements = nc;
                arr->MaxElements = nc;
            }
        }
        std::cout << "[SuperSonic] Injected driver 46 into " << pname << " list\n";
    }
}

// Layer 3: when a CSS window init event fires, call AddDriverIcon(46) directly
// and also trigger a rebuild of the icon list via known refresh functions.
static bool s_SuperSonicIconAdded = false;
static void InjectSuperSonicIcon(SDK::AActor* Caller, SDK::UFunction* Function)
{
    if (s_SuperSonicIconAdded) return;

    // Check if this event is one of the known CSS window init events
    std::string fname = Function->GetName();
    bool isCSSEvent   = false;
    for (int i = 0; kCSSWindowInitEvents[i]; i++)
        if (fname == kCSSWindowInitEvents[i]) { isCSSEvent = true; break; }
    if (!isCSSEvent) return;

    SDK::UObject* obj = reinterpret_cast<SDK::UObject*>(Caller);
    if (!obj || !obj->Class) return;

    // Only act on objects that look like the CSS window widget
    std::string cname = obj->Class->GetName();
    bool isCSSWindow  = (cname.find("CharaSelect") != std::string::npos ||
                         cname.find("CharaSel")    != std::string::npos ||
                         cname.find("BPC_Chara")   != std::string::npos);
    if (!isCSSWindow) return;

    ResolveDriverListFuncs();

    // Try AddDriverIcon(46) directly on the widget
    if (s_AddDriverIconFunc) {
        uint8_t params[512] = {};
        SDK::FProperty* p = nullptr;
        for (SDK::FField* f = s_AddDriverIconFunc->ChildProperties; f; f = f->Next) {
            auto* prop = static_cast<SDK::FProperty*>(f);
            if (!(prop->PropertyFlags & static_cast<uint64_t>(SDK::EPropertyFlags::ReturnParm))) {
                p = prop; break;
            }
        }
        if (p) {
            *(uint8_t*)(params + p->Offset) = 46;
            obj->ProcessEvent(s_AddDriverIconFunc, params);
            std::cout << "[SuperSonic] Called AddDriverIcon(46) on " << cname << "\n";
            s_SuperSonicIconAdded = true;
        }
    }

    // Also try known refresh/rebuild function names as a belt-and-suspenders
    static const char* rebuildFuncs[] = {
        "RebuildCharaList", "RefreshCharaList", "UpdateCharaList",
        "ResetCharaSelect", "RefreshDriverList",
        nullptr
    };
    for (int i = 0; rebuildFuncs[i]; i++) {
        SDK::UFunction* rf = Reflect::FindFunction(cname.c_str(), rebuildFuncs[i]);
        if (rf) { uint8_t p[64] = {}; obj->ProcessEvent(rf, p); }
    }
}

// ---------------------------------------------------------------------------
// Main ProcessEvent hook
// ---------------------------------------------------------------------------
void __fastcall hk_AActor_ProcessEvent(SDK::AActor* Class, SDK::UFunction* Function, void* Parms)
{
    if (!bCleared) Clear();

    if (!cfg.HotkeyEnabled && !bUnlockStarted && !bUnlockDone) {
        if (!bAutoRunReady) { if (!Function->GetName().compare("OnInitStateSelectPlayMode")) bAutoRunReady = true; }
        else { bUnlockStarted = true; unlockPhase = 0; lastPhaseTime = GetTickCount64(); std::cout << "[SRCW] AutoRun starting...\n"; }
    }

    if (cfg.HotkeyEnabled && !bUnlockStarted && !bUnlockDone) {
        bool kd = (GetAsyncKeyState(cfg.UnlockKey) & 0x8000) != 0;
        if (kd && !bKeyWasDown) { bUnlockStarted = true; unlockPhase = 0; lastPhaseTime = GetTickCount64(); std::cout << "[SRCW] Hotkey pressed...\n"; }
        bKeyWasDown = kd;
    }

    if (bUnlockStarted && !bUnlockDone) {
        ULONGLONG now = GetTickCount64();
        if (now - lastPhaseTime >= (ULONGLONG)cfg.PhaseDelayMs) {
            if (RunUnlockPhase(unlockPhase)) { unlockPhase++; lastPhaseTime = now; }
            else { bUnlockDone = true; std::cout << "All phases complete!\n"; }
        }
    }

    Orig_AActor_ProcessEvent(Class, Function, Parms);

    if (cfg.SuperSonicAll) {
        PatchIsDriverSelectable(Function, Parms);        // Layer 1
        PatchDriverIdListOutput(Class, Function, Parms); // Layer 2
        InjectSuperSonicIcon(Class, Function);           // Layer 3
    }
}
