#include "Features.h"

// Helper to find the live UAppSaveGame instance
static SDK::UAppSaveGame* FindSaveGame()
{
	for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
	{
		SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
		if (!Obj || Obj->IsDefaultObject())
			continue;

		if (Obj->IsA(SDK::UAppSaveGame::StaticClass()))
			return static_cast<SDK::UAppSaveGame*>(Obj);
	}
	return nullptr;
}

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

		std::cout << "[SRCW] Hooked! Press ~ (tilde) to unlock everything.\n";
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

#if ENABLE_CLEAR_CHARA_DLC
			for (auto& cm : Content->CharaDLCMap)
				cm.Value().DriverIds.Clear();
#endif

#if ENABLE_CLEAR_MACHINE_DLC
			for (auto& mm : Content->MachineDLCMap)
				mm.Value().MachineIds.Clear();
#endif

#if ENABLE_CLEAR_HONOR_TITLE_DLC
			for (auto& hm : Content->HonorOtherTitleDLCMap)
				hm.Value().HonorTitleIds.Clear();
#endif

#if ENABLE_CLEAR_ALBUM_DLC
			for (auto& am : Content->AlbumDLCMap)
				am.Value().AlbumIds.Clear();
#endif

#if ENABLE_CLEAR_STICKER_DLC
			for (auto& sm : Content->StickerDLCMap)
				sm.Value().StickerIds.Clear();
#endif

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
#if ENABLE_HONOR_TITLES
		for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
		{
			SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
			if (!Obj || Obj->IsDefaultObject())
				continue;

			if (Obj->IsA(SDK::UHonorTitleListDataAsset::StaticClass()))
			{
				auto Title = static_cast<SDK::UHonorTitleListDataAsset*>(Obj);
				for (const auto& t : Title->HonorTitleTableDataMap)
					SDK::UAppSaveGameHelper::UnlockHonorTitle(t.Key());
			}
		}
		std::cout << "[Phase 0] Honor titles unlocked\n";
#endif
		return true;
	}

	// Phase 1: Drivers
	case 1:
	{
#if ENABLE_DRIVERS
		for (int i = 0; i < (uint8_t)SDK::EDriverId::Num; i++)
		{
			SDK::UAppSaveGameHelper::SetDriverSelectable(SDK::EDriverId(i));
			SDK::UAppSaveGameHelper::ClearDriverNew(SDK::EDriverId(i));
		}
		std::cout << "[Phase 1] Drivers unlocked\n";
#endif
		return true;
	}

	// Phase 2: Machine customization
	case 2:
	{
#if ENABLE_MACHINE_CUSTOMIZE
		SDK::UMachineCustomizeUtilityLibrary::StoreAllAura();
		SDK::UMachineCustomizeUtilityLibrary::StoreAllHorn();
		SDK::UMachineCustomizeUtilityLibrary::StoreAllMachineAssembly();
		SDK::UMachineCustomizeUtilityLibrary::StoreAllMachineParts();
		SDK::UMachineCustomizeUtilityLibrary::StoreAllSticker();
		SDK::UMachineCustomizeUtilityLibrary::UnlockGadgetAll();
		std::cout << "[Phase 2] Machine customization unlocked\n";
#endif
		return true;
	}

	// Phase 3: Machine color presets
	case 3:
	{
#if ENABLE_COLOR_PRESETS
		for (int i = 0; i < (uint8_t)SDK::EMachineId::Num; i++)
			SDK::UMachineCustomizeUtilityLibrary::UnlockMachinePresetColor(SDK::EMachineId(i));
		std::cout << "[Phase 3] Color presets unlocked\n";
#endif
		return true;
	}

	// Phase 4: Mirror Mode & Super Sonic Speed
	case 4:
	{
#if ENABLE_MIRROR_SPEED
		SDK::UAppSaveGameHelper::SetOpenMirror(true);
		SDK::UAppSaveGameHelper::SetOpenSuperSonicSpeed(true);
		std::cout << "[Phase 4] Mirror & Super Sonic unlocked\n";
#endif
		return true;
	}

	// Phase 5: Music albums & tracks
	case 5:
	{
#if ENABLE_MUSIC
		for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
		{
			SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
			if (!Obj || Obj->IsDefaultObject())
				continue;

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
#endif
		return true;
	}

	// Phase 6: DLC Stages — inject package IDs + refresh content
	case 6:
	{
#if ENABLE_STAGES_DLC
		SDK::UContentDataAsset* ContentData = nullptr;
		SDK::UUnionContentManager* ContentMgr = nullptr;

		for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
		{
			SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
			if (!Obj || Obj->IsDefaultObject())
				continue;

			if (!ContentData && Obj->IsA(SDK::UContentDataAsset::StaticClass()))
				ContentData = static_cast<SDK::UContentDataAsset*>(Obj);

			if (!ContentMgr && Obj->IsA(SDK::UUnionContentManager::StaticClass()))
				ContentMgr = static_cast<SDK::UUnionContentManager*>(Obj);

			if (ContentData && ContentMgr)
				break;
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
				{
					if (id == pkgId)
					{
						alreadyOwned = true;
						break;
					}
				}
				if (!alreadyOwned)
				{
					allIds.push_back(pkgId);
					added++;
				}
			}

			if (added > 0)
			{
				bool addSucceeded = true;
				int32_t originalNum = ContentMgr->m_packageIds.Num();

				for (int32_t j = originalNum; j < (int32_t)allIds.size(); j++)
				{
					if (!ContentMgr->m_packageIds.Add(allIds[j]))
					{
						addSucceeded = false;
						break;
					}
				}

				if (!addSucceeded)
				{
					struct FTArrayRaw
					{
						int32_t* Data;
						int32_t  NumElements;
						int32_t  MaxElements;
					};

					auto* arr = reinterpret_cast<FTArrayRaw*>(&ContentMgr->m_packageIds);
					arr->NumElements = originalNum;

					int32_t newCount = (int32_t)allIds.size();
					int32_t* newData = static_cast<int32_t*>(
						VirtualAlloc(nullptr, newCount * sizeof(int32_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
					);

					if (newData)
					{
						memcpy(newData, allIds.data(), newCount * sizeof(int32_t));
						arr->Data = newData;
						arr->NumElements = newCount;
						arr->MaxElements = newCount;
					}
					else
					{
						std::cout << "[Phase 6] VirtualAlloc failed!\n";
					}
				}

				SDK::UUnionContentUtils::RequestCheckContent(true);
				std::cout << "[Phase 6] Injected " << added << " stage DLC package IDs + refreshed content\n";
			}
			else
			{
				std::cout << "[Phase 6] All stage packages already owned\n";
			}
		}
		else
		{
			std::cout << "[Phase 6] Could not find ContentDataAsset or UnionContentManager\n";
		}
#endif
		return true;
	}

	// Phase 7: Open all Grand Prix directly in save data
	case 7:
	{
#if ENABLE_STAGES_GP_OPEN
		auto SaveGame = FindSaveGame();
		if (SaveGame)
		{
			auto& gpData = SaveGame->_UserGrandPrixData;

			// Open all GP entries and mark as played
			int gpCount = gpData.GrandPrixCompletionStatus.Num();
			for (int i = 0; i < gpCount; i++)
			{
				gpData.GrandPrixCompletionStatus[i].bOpened = true;
				gpData.GrandPrixCompletionStatus[i].bLeast1Play = true;
			}

			gpData.GrandPrixLeast1Play = true;
			gpData.GrandPrixWinCount = 100;
			gpData.GrandPrixFinishesCount = 100;
			gpData.GrandPrixFirstIrregularAppeared = true;

			// Mark all events as completed
			for (int i = 0; i < gpData.EventCompleted.Num(); i++)
				gpData.EventCompleted[i] = true;

			// Also use the helper functions for redundancy
			for (int gp = (uint8_t)SDK::EGrandPrixId::Purple_01; gp < (uint8_t)SDK::EGrandPrixId::Num; gp++)
				SDK::UAppSaveGameHelper::SetGrandPrixLeast1PlayEachGrandPrix(SDK::EGrandPrixId(gp), true);

			for (int ev = 0; ev < (uint8_t)SDK::EGrandPrixEventFlag::Num; ev++)
				SDK::UAppSaveGameHelper::SetDodonpaEventCompleteFlag(SDK::EGrandPrixEventFlag(ev), true);

			SDK::UAppSaveGameHelper::SetGrandPrixLeast1Play(true);
			SDK::UAppSaveGameHelper::SetCompleteMainChallenge(true);
			SDK::UAppSaveGameHelper::SetCompleteSpecialChallenge(true);

			std::cout << "[Phase 7] All " << gpCount << " Grand Prix opened directly in save data\n";
		}
		else
		{
			std::cout << "[Phase 7] Could not find UAppSaveGame instance!\n";
		}
#endif
		return true;
	}

	// Phase 8: Another/Secret stages + endings — direct save data
	case 8:
	{
#if ENABLE_STAGES_SECRET
		auto SaveGame = FindSaveGame();
		if (SaveGame)
		{
			auto& gpData = SaveGame->_UserGrandPrixData;

			// Add all Another stage IDs (STG2001-STG2020) to PlayedAnotherStageIds
			for (int s = (uint8_t)SDK::EStageId::AnotherBegin; s <= (uint8_t)SDK::EStageId::AnotherEnd; s++)
			{
				SDK::EStageId stageId = SDK::EStageId(s);

				// Check if already present
				bool found = false;
				for (int j = 0; j < gpData.PlayedAnotherStageIds.Num(); j++)
				{
					if (gpData.PlayedAnotherStageIds[j] == stageId)
					{
						found = true;
						break;
					}
				}

				if (!found)
					gpData.PlayedAnotherStageIds.Add(stageId);
			}

			// Add both endings to ClearedEndingIds
			auto addEnding = [&](SDK::EGrandPrixEndingId id) {
				bool found = false;
				for (int j = 0; j < gpData.ClearedEndingIds.Num(); j++)
				{
					if (gpData.ClearedEndingIds[j] == id)
					{
						found = true;
						break;
					}
				}
				if (!found)
					gpData.ClearedEndingIds.Add(id);
			};

			addEnding(SDK::EGrandPrixEndingId::OrbotCubotEnding);
			addEnding(SDK::EGrandPrixEndingId::MemoryEnding);

			std::cout << "[Phase 8] Another/Secret stages + endings set in save data\n";
			std::cout << "  PlayedAnotherStageIds count: " << gpData.PlayedAnotherStageIds.Num() << "\n";
			std::cout << "  ClearedEndingIds count: " << gpData.ClearedEndingIds.Num() << "\n";
		}
		else
		{
			std::cout << "[Phase 8] Could not find UAppSaveGame instance!\n";
		}
#endif
		return true;
	}

	// Phase 9: New flags — single machine customize calls
	case 9:
	{
#if ENABLE_NF_COMPLETE_MACHINE
		SDK::UMachineCustomizeUtilityLibrary::DisableCompleteMachineNewFlags();
#endif
#if ENABLE_NF_STICKER
		SDK::UMachineCustomizeUtilityLibrary::DisableStickerNewFlags();
#endif
#if ENABLE_NF_COLOR_PRESET
		SDK::UMachineCustomizeUtilityLibrary::DisableMachineColorPresetNewFlags();
#endif
#if ENABLE_NF_GADGET
		SDK::UMachineCustomizeUtilityLibrary::DisableDisplayedGadgetNewFlags();
#endif
		std::cout << "[Phase 9] Machine new flags cleared\n";
		return true;
	}

	case 10:
	{
#if ENABLE_NF_PARTS_SPEED
		SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusSpeed);
		std::cout << "[Phase 10] Parts Speed new flags cleared\n";
#endif
		return true;
	}

	case 11:
	{
#if ENABLE_NF_PARTS_ACCEL
		SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusAccele);
		std::cout << "[Phase 11] Parts Accel new flags cleared\n";
#endif
		return true;
	}

	case 12:
	{
#if ENABLE_NF_PARTS_HANDLE
		SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusHandle);
		std::cout << "[Phase 12] Parts Handle new flags cleared\n";
#endif
		return true;
	}

	case 13:
	{
#if ENABLE_NF_PARTS_POWER
		SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusPower);
		std::cout << "[Phase 13] Parts Power new flags cleared\n";
#endif
		return true;
	}

	case 14:
	{
#if ENABLE_NF_PARTS_DASH
		SDK::UMachineCustomizeUtilityLibrary::DisablePartsListNewFlagByType(SDK::ECarStatusType::CarStatusDash);
		std::cout << "[Phase 14] Parts Dash new flags cleared\n";
#endif
		return true;
	}

	case 15:
	{
#if ENABLE_NF_HORN
		for (int i = 0; i < (uint8_t)SDK::EMachineHornType::Num; i++)
			SDK::UMachineCustomizeUtilityLibrary::SetCustomMachineHornNew(SDK::EMachineHornType(i), false);
		std::cout << "[Phase 15] Horn new flags cleared\n";
#endif
		return true;
	}

	case 16:
	{
#if ENABLE_NF_HONOR_TITLES
		for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
		{
			SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
			if (!Obj || Obj->IsDefaultObject())
				continue;

			if (Obj->IsA(SDK::UHonorTitleListDataAsset::StaticClass()))
			{
				auto Title = static_cast<SDK::UHonorTitleListDataAsset*>(Obj);
				for (const auto& t : Title->HonorTitleTableDataMap)
					SDK::UAppSaveGameHelper::ResetNewHonorTitle(t.Key());
			}
		}
		std::cout << "[Phase 16] Honor title new flags cleared\n";
#endif
		return true;
	}

	// Phase 17: Jukebox new flags — clear via save game directly
	case 17:
	{
#if ENABLE_NF_JUKEBOX
		for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
		{
			SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);
			if (!Obj || Obj->IsDefaultObject())
				continue;

			if (Obj->IsA(SDK::UAppSaveGame::StaticClass()))
			{
				auto SaveGame = static_cast<SDK::UAppSaveGame*>(Obj);

				int cleared = 0;
				for (auto& entry : SaveGame->_UserJukeboxData.AlbumCondition)
				{
					entry.Value().bNew = false;
					cleared++;
				}
				for (auto& entry : SaveGame->_UserJukeboxData.TrackCondition)
				{
					entry.Value().bNew = false;
				}

				std::cout << "[Phase 17] Jukebox new flags cleared (" << cleared << " albums)\n";
				break;
			}
		}
#endif
		return true;
	}

	case 18:
	{
#if ENABLE_NF_CHALLENGES
		int32_t progressCount = SDK::UChallengeStatsUtility::GetChallengeProgressCount();
		SDK::UAppSaveGameHelper::SetChallengeShowProgress(progressCount);
		SDK::UAppSaveGameHelper::SetChallengeLastShowProgress(1.0f);

		for (int i = 0; i < (uint8_t)SDK::EChallengeId::Num; i++)
		{
			auto state = SDK::UChallengeStatsUtility::GetChallengeProgressState(SDK::EChallengeId(i));
			if (state == SDK::EChallengeProgressState::Acquired)
			{
				SDK::UChallengeStatsUtility::SetChallengeProgressState(
					SDK::EChallengeId(i), SDK::EChallengeProgressState::Verified);
			}
		}
		std::cout << "[Phase 18] Challenge new flags cleared\n";
#endif

#if ENABLE_NF_REWARDS
		SDK::UAppSaveGameHelper::ClearRewardGetDisplayRequestDataAll();
		std::cout << "[Phase 18] Reward notifications cleared\n";
#endif
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

	if (!bUnlockStarted && !bUnlockDone)
	{
		bool keyDown = (GetAsyncKeyState(UNLOCK_KEY) & 0x8000) != 0;

		if (keyDown && !bKeyWasDown)
		{
			bUnlockStarted = true;
			unlockPhase = 0;
			lastPhaseTime = GetTickCount64();
			std::cout << "[SRCW] ~ pressed — starting phased unlock...\n";
		}

		bKeyWasDown = keyDown;
	}

	if (bUnlockStarted && !bUnlockDone)
	{
		ULONGLONG now = GetTickCount64();
		if (now - lastPhaseTime >= PHASE_DELAY_MS)
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
