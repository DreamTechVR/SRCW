#include "Features.h"

void HookGame()
{
	bool bHooked = false;

	while (!bHooked)
	{
		auto World = SDK::UWorld::GetWorld();
		auto Engine = SDK::UEngine::GetEngine();

		if (!World || !Engine || !Engine->GameViewport)
		{
			continue;
		}

		Orig_AActor_ProcessEvent = reinterpret_cast<AActor_ProcessEvent_t>(PatternScan("48 89 5C 24 10 48 89 6C 24 18 57 48 83 EC ? F7 82 B0 00 00 00 ? ? ? ?"));

		if (!Orig_AActor_ProcessEvent)
		{
			continue;
		}

		Orig_AActor_ProcessEvent = reinterpret_cast<AActor_ProcessEvent_t>(TrampHook64((BYTE*)Orig_AActor_ProcessEvent, (BYTE*)hk_AActor_ProcessEvent, 15));

		bHooked = true;

		if (cfg.AutoRun)
			std::cout << "[SRCW] Hooked! Will unlock automatically on menu load.\n";
		else if (cfg.HotkeyEnabled)
			std::cout << "[SRCW] Hooked! Press hotkey (0x" << std::hex << cfg.UnlockKey << std::dec << ") to unlock.\n";
	}
}

void Clear()
{
	for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
	{
		SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);

		if (!Obj)
			continue;

		if (Obj->IsDefaultObject())
			continue;

		if (Obj->IsA(SDK::UContentDataAsset::StaticClass()))
		{
			auto Content = static_cast<SDK::UContentDataAsset*>(Obj);

			if (cfg.ClearCharaDLC)
				for (auto& cm : Content->CharaDLCMap)
					cm.Value().DriverIds.Clear();

			if (cfg.ClearMachineDLC)
				for (auto& mm : Content->MachineDLCMap)
					mm.Value().MachineIds.Clear();

			if (cfg.ClearHonorDLC)
				for (auto& hm : Content->HonorOtherTitleDLCMap)
					hm.Value().HonorTitleIds.Clear();

			if (cfg.ClearAlbumDLC)
				for (auto& am : Content->AlbumDLCMap)
					am.Value().AlbumIds.Clear();

			if (cfg.ClearStickerDLC)
				for (auto& sm : Content->StickerDLCMap)
					sm.Value().StickerIds.Clear();

			std::cout << "Cleared DLCs!\n";
			bCleared = true;
		}
	}
}

bool RunUnlockPhase(int phase)
{
	switch (phase)
	{

	// Phase 0: Honor Titles
	case 0:
	{
		if (cfg.HonorTitles)
		{
			for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
			{
				SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
				if (!Obj || Obj->IsDefaultObject()) continue;
				if (Obj->IsA(SDK::UHonorTitleListDataAsset::StaticClass()))
				{
					auto Title = static_cast<SDK::UHonorTitleListDataAsset*>(Obj);
					for (const auto& t : Title->HonorTitleTableDataMap)
						SDK::UAppSaveGameHelper::UnlockHonorTitle(t.Key());
				}
			}
			std::cout << "[Phase 0] Honor titles unlocked\n";
		}
		return true;
	}

	// Phase 1: Drivers
	case 1:
	{
		if (cfg.Drivers)
		{
			for (int i = 0; i < (uint8_t)SDK::EDriverId::Num; i++)
			{
				SDK::UAppSaveGameHelper::SetDriverSelectable(SDK::EDriverId(i));
				SDK::UAppSaveGameHelper::ClearDriverNew(SDK::EDriverId(i));
			}
			std::cout << "[Phase 1] Drivers unlocked\n";
		}
		return true;
	}

	// Phase 2: Machine customization
	case 2:
	{
		if (cfg.MachineCustomize)
		{
			SDK::UMachineCustomizeUtilityLibrary::StoreAllAura();
			SDK::UMachineCustomizeUtilityLibrary::StoreAllHorn();
			SDK::UMachineCustomizeUtilityLibrary::StoreAllMachineAssembly();
			SDK::UMachineCustomizeUtilityLibrary::StoreAllMachineParts();
			SDK::UMachineCustomizeUtilityLibrary::StoreAllSticker();
			SDK::UMachineCustomizeUtilityLibrary::UnlockGadgetAll();
			std::cout << "[Phase 2] Machine customization unlocked\n";
		}
		return true;
	}

	// Phase 3: Machine color presets
	case 3:
	{
		if (cfg.ColorPresets)
		{
			for (int i = 0; i < (uint8_t)SDK::EMachineId::Num; i++)
				SDK::UMachineCustomizeUtilityLibrary::UnlockMachinePresetColor(SDK::EMachineId(i));
			std::cout << "[Phase 3] Color presets unlocked\n";
		}
		return true;
	}

	// Phase 4: Mirror Mode & Super Sonic Speed
	case 4:
	{
		if (cfg.MirrorSpeed)
		{
			SDK::UAppSaveGameHelper::SetOpenMirror(true);
			SDK::UAppSaveGameHelper::SetOpenSuperSonicSpeed(true);
			std::cout << "[Phase 4] Mirror & Super Sonic unlocked\n";
		}
		return true;
	}

	// Phase 5: Music albums & tracks
	case 5:
	{
		if (cfg.Music)
		{
			for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
			{
				SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
				if (!Obj || Obj->IsDefaultObject()) continue;
				if (Obj->IsA(SDK::UJukeboxDataAsset::StaticClass()))
				{
					auto Jukebox = static_cast<SDK::UJukeboxDataAsset*>(Obj);
					for (const auto& album : Jukebox->AlbumDataMap)
					{
						SDK::UAppSaveGameHelper::SetAlbumAvailable(album.Key());
						for (const auto& trackId : album.Value().TrackIDList)
							SDK::UAppSaveGameHelper::SetTrackAvailable(trackId);
					}
				}
			}
			std::cout << "[Phase 5] Albums & tracks unlocked\n";
		}
		return true;
	}

	// Phase 6: DLC Stages
	case 6:
	{
		if (cfg.StagesDLC)
		{
			SDK::UContentDataAsset* ContentData = nullptr;
			SDK::UUnionContentManager* ContentMgr = nullptr;

			for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
			{
				SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
				if (!Obj || Obj->IsDefaultObject()) continue;
				if (!ContentData && Obj->IsA(SDK::UContentDataAsset::StaticClass()))
					ContentData = static_cast<SDK::UContentDataAsset*>(Obj);
				if (!ContentMgr && Obj->IsA(SDK::UUnionContentManager::StaticClass()))
					ContentMgr = static_cast<SDK::UUnionContentManager*>(Obj);
				if (ContentData && ContentMgr) break;
			}

			if (ContentData && ContentMgr)
			{
				std::vector<int32_t> allIds;
				for (int j = 0; j < ContentMgr->m_packageIds.Num(); j++)
					allIds.push_back(ContentMgr->m_packageIds[j]);

				int32_t added = 0;
				for (const auto& entry : ContentData->StageDLCMap)
				{
					int32_t pkgId = entry.Key();
					bool alreadyOwned = false;
					for (const auto& id : allIds)
						if (id == pkgId) { alreadyOwned = true; break; }
					if (!alreadyOwned) { allIds.push_back(pkgId); added++; }
				}

				if (added > 0)
				{
					bool addSucceeded = true;
					int32_t originalNum = ContentMgr->m_packageIds.Num();

					for (int32_t j = originalNum; j < (int32_t)allIds.size(); j++)
					{
						if (!ContentMgr->m_packageIds.Add(allIds[j]))
						{ addSucceeded = false; break; }
					}

					if (!addSucceeded)
					{
						struct FTArrayRaw { int32_t* Data; int32_t NumElements; int32_t MaxElements; };
						auto* arr = reinterpret_cast<FTArrayRaw*>(&ContentMgr->m_packageIds);
						arr->NumElements = originalNum;
						int32_t newCount = (int32_t)allIds.size();
						int32_t* newData = static_cast<int32_t*>(
							VirtualAlloc(nullptr, newCount * sizeof(int32_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
						if (newData)
						{
							memcpy(newData, allIds.data(), newCount * sizeof(int32_t));
							arr->Data = newData;
							arr->NumElements = newCount;
							arr->MaxElements = newCount;
						}
					}

					SDK::UUnionContentUtils::RequestCheckContent(true);
					std::cout << "[Phase 6] Injected " << added << " stage DLC package IDs\n";
				}
				else
					std::cout << "[Phase 6] All stage packages already owned\n";
			}
		}
		return true;
	}

	// Phase 7: Open all Grand Prix
	case 7:
	{
		if (cfg.StagesGPOpen)
		{
			auto CheatGP = SDK::UCheatGrandPrix::GetDefaultObj();
			if (CheatGP)
			{
				CheatGP->SetGrandPrixOpenedAll(true);
				CheatGP->SetSetGrandPrixLeast1Play(true);
			}

			for (int gp = (uint8_t)SDK::EGrandPrixId::Purple_01; gp < (uint8_t)SDK::EGrandPrixId::Num; gp++)
				SDK::UAppSaveGameHelper::SetGrandPrixLeast1PlayEachGrandPrix(SDK::EGrandPrixId(gp), true);

			for (int ev = 0; ev < (uint8_t)SDK::EGrandPrixEventFlag::Num; ev++)
				SDK::UAppSaveGameHelper::SetDodonpaEventCompleteFlag(SDK::EGrandPrixEventFlag(ev), true);

			SDK::UAppSaveGameHelper::SetGrandPrixLeast1Play(true);

			std::cout << "[Phase 7] All Grand Prix opened\n";
		}
		return true;
	}

	// Phase 8: Secret/Another stages + endings
	case 8:
	{
		if (cfg.StagesSecret)
		{
			auto CheatGP = SDK::UCheatGrandPrix::GetDefaultObj();
			if (CheatGP)
			{
				CheatGP->SetPlayedAnotherStageAll();
				CheatGP->SetClearedGrandPrixEnding(SDK::EGrandPrixEndingId::OrbotCubotEnding, true);
				CheatGP->SetClearedGrandPrixEnding(SDK::EGrandPrixEndingId::MemoryEnding, true);
			}
			std::cout << "[Phase 8] Secret stages + endings unlocked\n";
		}
		return true;
	}

	// Phase 9: Gadget plate — set to max rank
	case 9:
	{
		if (cfg.GadgetPlate)
		{
			SDK::UMachineCustomizeUtilityLibrary::SetCurrentGadgetPlateIdUseId(SDK::EGadgetPlateId::Rank7);
			SDK::UMachineCustomizeUtilityLibrary::UpdateGadgetSlotNumInUserData();
			std::cout << "[Phase 9] Gadget plate set to Rank7 (max)\n";
		}
		return true;
	}

	// Phase 10: Complete all challenges
	case 10:
	{
		if (cfg.Challenges)
		{
			// Force every challenge to Verified (completed + claimed)
			for (int i = 0; i < (uint8_t)SDK::EChallengeId::Num; i++)
			{
				SDK::UChallengeStatsUtility::SetChallengeProgressState(
					SDK::EChallengeId(i), SDK::EChallengeProgressState::Verified);
			}

			SDK::UAppSaveGameHelper::SetCompleteMainChallenge(true);
			SDK::UAppSaveGameHelper::SetCompleteSpecialChallenge(true);

			// Sync progress counter so the game knows everything is seen
			int32_t progressCount = SDK::UChallengeStatsUtility::GetChallengeProgressCount();
			SDK::UAppSaveGameHelper::SetChallengeShowProgress(progressCount);
			SDK::UAppSaveGameHelper::SetChallengeLastShowProgress(1.0f);

			std::cout << "[Phase 10] All " << (uint8_t)SDK::EChallengeId::Num << " challenges completed\n";
		}
		return true;
	}

	// Phase 11: New flags — single machine customize calls
	case 11:
	{
		if (cfg.NF_CompleteMachine)
			SDK::UMachineCustomizeUtilityLibrary::DisableCompleteMachineNewFlags();
		if (cfg.NF_Sticker)
			SDK::UMachineCustomizeUtilityLibrary::DisableStickerNewFlags();
		if (cfg.NF_ColorPreset)
			SDK::UMachineCustomizeUtilityLibrary::DisableMachineColorPresetNewFlags();
		if (cfg.NF_Gadget)
			SDK::UMachineCustomizeUtilityLibrary::DisableDisplayedGadgetNewFlags();
		std::cout << "[Phase 11] Machine new flags cleared\n";
		return true;
	}

	// Phases 12-16: Parts new flags (one per tick)
	case 12:
	{
		if (cfg.NF_PartsSpeed)
			SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusSpeed);
		std::cout << "[Phase 12] Parts Speed done\n";
		return true;
	}
	case 13:
	{
		if (cfg.NF_PartsAccel)
			SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusAccele);
		std::cout << "[Phase 13] Parts Accel done\n";
		return true;
	}
	case 14:
	{
		if (cfg.NF_PartsHandle)
			SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusHandle);
		std::cout << "[Phase 14] Parts Handle done\n";
		return true;
	}
	case 15:
	{
		if (cfg.NF_PartsPower)
			SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusPower);
		std::cout << "[Phase 15] Parts Power done\n";
		return true;
	}
	case 16:
	{
		if (cfg.NF_PartsDash)
			SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusDash);
		std::cout << "[Phase 16] Parts Dash done\n";
		return true;
	}

	// Phase 17: Horn new flags
	case 17:
	{
		if (cfg.NF_Horn)
		{
			for (int i = 0; i < (uint8_t)SDK::EMachineHornType::Num; i++)
				SDK::UMachineCustomizeUtilityLibrary::SetCustomMachineHornNew(SDK::EMachineHornType(i), false);
		}
		std::cout << "[Phase 17] Horn new flags cleared\n";
		return true;
	}

	// Phase 18: Honor title new flags
	case 18:
	{
		if (cfg.NF_HonorTitles)
		{
			for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
			{
				SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
				if (!Obj || Obj->IsDefaultObject()) continue;
				if (Obj->IsA(SDK::UHonorTitleListDataAsset::StaticClass()))
				{
					auto Title = static_cast<SDK::UHonorTitleListDataAsset*>(Obj);
					for (const auto& t : Title->HonorTitleTableDataMap)
						SDK::UAppSaveGameHelper::ResetNewHonorTitle(t.Key());
				}
			}
			std::cout << "[Phase 18] Honor title new flags cleared\n";
		}
		return true;
	}

	// Phase 19: Jukebox new flags — via save game
	case 19:
	{
		if (cfg.NF_Jukebox)
		{
			for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
			{
				SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
				if (!Obj || Obj->IsDefaultObject()) continue;
				if (Obj->IsA(SDK::UAppSaveGame::StaticClass()))
				{
					auto SaveGame = static_cast<SDK::UAppSaveGame*>(Obj);
					int cleared = 0;
					for (auto& entry : SaveGame->_UserJukeboxData.AlbumCondition)
					{ entry.Value().bNew = false; cleared++; }
					for (auto& entry : SaveGame->_UserJukeboxData.TrackCondition)
					{ entry.Value().bNew = false; }
					std::cout << "[Phase 19] Jukebox new flags cleared (" << cleared << " albums)\n";
					break;
				}
			}
		}
		return true;
	}

	// Phase 20: Challenge new flags + reward notifications
	case 20:
	{
		if (cfg.NF_Challenges)
		{
			int32_t progressCount = SDK::UChallengeStatsUtility::GetChallengeProgressCount();
			SDK::UAppSaveGameHelper::SetChallengeShowProgress(progressCount);
			SDK::UAppSaveGameHelper::SetChallengeLastShowProgress(1.0f);

			for (int i = 0; i < (uint8_t)SDK::EChallengeId::Num; i++)
			{
				auto state = SDK::UChallengeStatsUtility::GetChallengeProgressState(SDK::EChallengeId(i));
				if (state == SDK::EChallengeProgressState::Acquired)
					SDK::UChallengeStatsUtility::SetChallengeProgressState(
						SDK::EChallengeId(i), SDK::EChallengeProgressState::Verified);
			}
			std::cout << "[Phase 20] Challenge new flags cleared\n";
		}

		if (cfg.NF_Rewards)
		{
			SDK::UAppSaveGameHelper::ClearRewardGetDisplayRequestDataAll();
			std::cout << "[Phase 20] Reward notifications cleared\n";
		}
		return true;
	}

	default:
		return false;
	}
}

void __fastcall hk_AActor_ProcessEvent(SDK::AActor* Class, SDK::UFunction* Function, void* Parms)
{
	if (!bCleared)
	{
		Clear();
	}

	// --- AutoRun: trigger when main menu loads ---
	if (cfg.AutoRun && !bUnlockStarted && !bUnlockDone)
	{
		if (!bAutoRunReady)
		{
			if (!Function->GetName().compare("OnInitStateSelectPlayMode"))
				bAutoRunReady = true;
		}
		else
		{
			bUnlockStarted = true;
			unlockPhase = 0;
			lastPhaseTime = GetTickCount64();
			std::cout << "[SRCW] AutoRun — starting phased unlock...\n";
		}
	}

	// --- Hotkey: trigger on key press ---
	if (cfg.HotkeyEnabled && !bUnlockStarted && !bUnlockDone)
	{
		bool keyDown = (GetAsyncKeyState(cfg.UnlockKey) & 0x8000) != 0;

		if (keyDown && !bKeyWasDown)
		{
			bUnlockStarted = true;
			unlockPhase = 0;
			lastPhaseTime = GetTickCount64();
			std::cout << "[SRCW] Hotkey pressed — starting phased unlock...\n";
		}

		bKeyWasDown = keyDown;
	}

	// --- Run one phase every PhaseDelayMs ---
	if (bUnlockStarted && !bUnlockDone)
	{
		ULONGLONG now = GetTickCount64();
		if (now - lastPhaseTime >= (ULONGLONG)cfg.PhaseDelayMs)
		{
			if (RunUnlockPhase(unlockPhase))
			{
				unlockPhase++;
				lastPhaseTime = now;
			}
			else
			{
				bUnlockDone = true;
				std::cout << "All phases complete — Unlocked Everything!\n";
			}
		}
	}

	Orig_AActor_ProcessEvent(Class, Function, Parms);
}
