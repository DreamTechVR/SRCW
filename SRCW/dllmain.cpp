#include "Features.h"

#include "CppSDK/SDK/Engine_classes.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

inline HMODULE CurrentModule = nullptr;

void LoadConfig()
{
    std::ifstream file(ConfigFileName);

    if (!file.is_open())
    {
        WriteDefaultConfig();
        file.open(ConfigFileName);
        if (!file.is_open()) return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string key;
        int value;

        if (!(iss >> key >> value))
            continue;

        bool b = (value != 0);

        // General
        if (key == "Console")           cfg.Console = b;
        else if (key == "ClearOnly")    cfg.ClearOnly = b;
        else if (key == "PhaseDelayMs") cfg.PhaseDelayMs = value;
        else if (key == "AutoRun")      cfg.AutoRun = b;
        else if (key == "HotkeyEnabled") cfg.HotkeyEnabled = b;
        else if (key == "UnlockKey")    cfg.UnlockKey = value;

        // DLC Gate Removal
        else if (key == "ClearCharaDLC")    cfg.ClearCharaDLC = b;
        else if (key == "ClearMachineDLC")  cfg.ClearMachineDLC = b;
        else if (key == "ClearHonorDLC")    cfg.ClearHonorDLC = b;
        else if (key == "ClearAlbumDLC")    cfg.ClearAlbumDLC = b;
        else if (key == "ClearStickerDLC")  cfg.ClearStickerDLC = b;

        // Save Data Unlocks
        else if (key == "HonorTitles")      cfg.HonorTitles = b;
        else if (key == "Drivers")          cfg.Drivers = b;
        else if (key == "MachineCustomize") cfg.MachineCustomize = b;
        else if (key == "ColorPresets")     cfg.ColorPresets = b;
        else if (key == "MirrorSpeed")      cfg.MirrorSpeed = b;
        else if (key == "Music")            cfg.Music = b;
        else if (key == "GadgetPlate")      cfg.GadgetPlate = b;
        else if (key == "Challenges")       cfg.Challenges = b;

        // Stage Unlocks
        else if (key == "StagesDLC")        cfg.StagesDLC = b;
        else if (key == "StagesGPOpen")     cfg.StagesGPOpen = b;
        else if (key == "StagesSecret")     cfg.StagesSecret = b;

        // New Flag Clearing
        else if (key == "NF_CompleteMachine") cfg.NF_CompleteMachine = b;
        else if (key == "NF_Sticker")       cfg.NF_Sticker = b;
        else if (key == "NF_ColorPreset")   cfg.NF_ColorPreset = b;
        else if (key == "NF_Gadget")        cfg.NF_Gadget = b;
        else if (key == "NF_PartsSpeed")    cfg.NF_PartsSpeed = b;
        else if (key == "NF_PartsAccel")    cfg.NF_PartsAccel = b;
        else if (key == "NF_PartsHandle")   cfg.NF_PartsHandle = b;
        else if (key == "NF_PartsPower")    cfg.NF_PartsPower = b;
        else if (key == "NF_PartsDash")     cfg.NF_PartsDash = b;
        else if (key == "NF_Horn")          cfg.NF_Horn = b;
        else if (key == "NF_HonorTitles")   cfg.NF_HonorTitles = b;
        else if (key == "NF_Jukebox")       cfg.NF_Jukebox = b;
        else if (key == "NF_Challenges")    cfg.NF_Challenges = b;
        else if (key == "NF_Rewards")       cfg.NF_Rewards = b;
    }

    file.close();

    if (cfg.ClearOnly) bUnlockDone = true;
}

void WriteDefaultConfig()
{
    std::ofstream file(ConfigFileName, std::ofstream::out | std::ofstream::trunc);
    if (!file.is_open()) return;

    file << "; =============================================================\n";
    file << "; SRCW Unlocker Config\n";
    file << "; Set to 1 to enable, 0 to disable\n";
    file << "; Changes take effect on next game launch (no recompile needed)\n";
    file << "; =============================================================\n\n";

    file << "; --- General ---\n";
    file << "Console 0\n";
    file << "ClearOnly 0\n";
    file << "PhaseDelayMs 500\n\n";

    file << "; --- Trigger Mode ---\n";
    file << "; AutoRun 1 = unlock automatically when main menu loads\n";
    file << "; AutoRun 0 = wait for hotkey press (default)\n";
    file << "AutoRun 0\n\n";

    file << "; HotkeyEnabled 1 = press hotkey to trigger unlock\n";
    file << "; HotkeyEnabled 0 = disable hotkey (use with AutoRun 1)\n";
    file << "HotkeyEnabled 1\n\n";

    file << "; UnlockKey = virtual key code (decimal)\n";
    file << "; 192 = ~ (tilde), 120 = F9, 45 = Insert\n";
    file << "; Full list: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes\n";
    file << "UnlockKey 192\n\n";

    file << "; --- DLC Gate Removal ---\n";
    file << "ClearCharaDLC 1\n";
    file << "ClearMachineDLC 1\n";
    file << "ClearHonorDLC 1\n";
    file << "ClearAlbumDLC 1\n";
    file << "ClearStickerDLC 1\n\n";

    file << "; --- Save Data Unlocks ---\n";
    file << "HonorTitles 1\n";
    file << "Drivers 1\n";
    file << "MachineCustomize 1\n";
    file << "ColorPresets 1\n";
    file << "MirrorSpeed 1\n";
    file << "Music 1\n";
    file << "GadgetPlate 1\n";
    file << "; Complete all challenges (main + special)\n";
    file << "Challenges 1\n\n";

    file << "; --- Stage Unlocks ---\n";
    file << "StagesDLC 1\n";
    file << "StagesGPOpen 1\n";
    file << "StagesSecret 1\n\n";

    file << "; --- New Flag Clearing (red exclamation marks) ---\n";
    file << "NF_CompleteMachine 1\n";
    file << "NF_Sticker 1\n";
    file << "NF_ColorPreset 1\n";
    file << "NF_Gadget 1\n";
    file << "NF_PartsSpeed 1\n";
    file << "NF_PartsAccel 1\n";
    file << "NF_PartsHandle 1\n";
    file << "NF_PartsPower 1\n";
    file << "NF_PartsDash 1\n";
    file << "NF_Horn 1\n";
    file << "NF_HonorTitles 1\n";
    file << "NF_Jukebox 1\n";
    file << "NF_Challenges 1\n";
    file << "NF_Rewards 1\n";

    file.close();
}

void Init()
{
    LoadConfig();

    auto CreateConsole = [](const char* name) {
        FILE* ConsoleIO;
        if (!AllocConsole())
            return;

        freopen_s(&ConsoleIO, "CONIN$", "r", stdin);
        freopen_s(&ConsoleIO, "CONOUT$", "w", stderr);
        freopen_s(&ConsoleIO, "CONOUT$", "w", stdout);

        SetConsoleTitleA(name);
    };

    if (cfg.Console) CreateConsole("SRCW");

    std::cout << "[*] Sonic Racing Crossworlds Unlocker\n";
    std::cout << "[+] Config loaded from SRCW.ini\n";
    std::cout << "[+] Phase delay: " << cfg.PhaseDelayMs << "ms\n";

    if (cfg.AutoRun)
        std::cout << "[+] Mode: AutoRun (triggers on menu load)\n";
    else if (cfg.HotkeyEnabled)
        std::cout << "[+] Mode: Hotkey (press key 0x" << std::hex << cfg.UnlockKey << std::dec << " to unlock)\n";
    else
        std::cout << "[!] Warning: Both AutoRun and HotkeyEnabled are off — nothing will trigger!\n";

    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HookGame, CurrentModule, 0, nullptr);
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CurrentModule = hModule;
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Init, CurrentModule, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
