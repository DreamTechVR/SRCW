#pragma once
#include <random>
#include <vector>

#include "Helpers.h"
#include "CppSDK/SDK/Engine_classes.hpp"
#include "CppSDK/SDK/BP_UnionGameInstance_classes.hpp"
#include "CppSDK/SDK/BP_MenuPlayerController_classes.hpp"
#include "CppSDK/SDK/UnionSystem_classes.hpp"
#include "CppSDK/SDK/UnionSystem_structs.hpp"
#include "CppSDK/SDK/UnionRun_structs.hpp"
#include "CppSDK/SDK/UNION_parameters.hpp"
#include "CppSDK/SDK/UNION_classes.hpp"
#include "CppSDK/SDK/BP_MenuRacerLobby_classes.hpp"

// =============================================================================
// FEATURE TOGGLES — set to 1 to enable, 0 to disable before compiling
// =============================================================================

// --- DLC Gate Removal ---
#define ENABLE_CLEAR_CHARA_DLC         1   // Unlock DLC-gated characters
#define ENABLE_CLEAR_MACHINE_DLC       1   // Unlock DLC-gated machines
#define ENABLE_CLEAR_HONOR_TITLE_DLC   1   // Unlock DLC-gated honor titles
#define ENABLE_CLEAR_ALBUM_DLC         1   // Unlock DLC-gated albums
#define ENABLE_CLEAR_STICKER_DLC       1   // Unlock DLC-gated stickers

// --- Save Data Unlocks ---
#define ENABLE_HONOR_TITLES            1   // Unlock all honor titles in save data
#define ENABLE_DRIVERS                 1   // Make all drivers selectable
#define ENABLE_MACHINE_CUSTOMIZE       1   // Auras, horns, assembly, parts, stickers, gadgets
#define ENABLE_COLOR_PRESETS           1   // Unlock machine color presets for all machines
#define ENABLE_MIRROR_SPEED            1   // Unlock Mirror Mode & Super Sonic Speed
#define ENABLE_MUSIC                   1   // Unlock all jukebox albums & tracks

// --- Stage Unlocks ---
#define ENABLE_STAGES_DLC              1   // Unlock DLC stages (SpongeBob, Minecraft, Pac-Man, etc.)
#define ENABLE_STAGES_GP_OPEN          1   // Open all Grand Prix + mark events complete
#define ENABLE_STAGES_SECRET           1   // Unlock Another/Secret stages + mark endings cleared

// --- New Flag Clearing (red exclamation marks) ---
#define ENABLE_NF_COMPLETE_MACHINE     1   // Clear general machine new badges
#define ENABLE_NF_STICKER              1   // Clear sticker new indicators
#define ENABLE_NF_COLOR_PRESET         1   // Clear color preset new indicators
#define ENABLE_NF_GADGET               1   // Clear gadget new indicators
#define ENABLE_NF_PARTS_SPEED          1   // Clear Speed parts new flags
#define ENABLE_NF_PARTS_ACCEL          1   // Clear Accel parts new flags
#define ENABLE_NF_PARTS_HANDLE         1   // Clear Handle parts new flags
#define ENABLE_NF_PARTS_POWER          1   // Clear Power parts new flags
#define ENABLE_NF_PARTS_DASH           1   // Clear Dash parts new flags
#define ENABLE_NF_HORN                 1   // Clear horn new indicators
#define ENABLE_NF_HONOR_TITLES         1   // Clear honor title new indicators
#define ENABLE_NF_JUKEBOX              1   // Clear jukebox/album new indicators
#define ENABLE_NF_CHALLENGES           1   // Clear challenge new indicators
#define ENABLE_NF_REWARDS              1   // Clear pending reward display notifications

// --- Timing ---
#define PHASE_DELAY_MS                 750  // Milliseconds between each unlock phase

// --- Hotkey ---
#define UNLOCK_KEY                     VK_OEM_3  // ~ (tilde) key to trigger unlock

// =============================================================================

inline const char ConfigFileName[] = ".\\UNION\\Binaries\\Win64\\SRCW.ini";

inline bool     bCleared = false;
inline int      unlockPhase = -1;
inline bool     bUnlockStarted = false;
inline bool     bUnlockDone = false;
inline bool     bKeyWasDown = false;
inline ULONGLONG lastPhaseTime = 0;

void HookGame();
void Clear();
bool RunUnlockPhase(int phase);

void __fastcall hk_AActor_ProcessEvent(SDK::AActor* Class, SDK::UFunction* Function, void* Parms);
typedef void(__fastcall* AActor_ProcessEvent_t)(SDK::AActor* Class, SDK::UFunction* Function, void* Parms);
inline AActor_ProcessEvent_t Orig_AActor_ProcessEvent;
