#include "Timber_born_Clone/Public/Buildings/TimberConstructionManager.h"
#include "Timber_born_Clone/Public/Buildings/TimberBuildingBase.h"
#include "Timber_born_Clone/Public/Beavers/BeaverAgent.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"

bool FBuildingQueuePredicate::operator()(const TWeakObjectPtr<ATimberBuildingBase>& A, const TWeakObjectPtr<ATimberBuildingBase>& B) const
{
	if (!A.IsValid()) return false;
	if (!B.IsValid()) return true;

	const ATimberBuildingBase* BuildingA = A.Get();
	const ATimberBuildingBase* BuildingB = B.Get();

	// 1. TẦNG 1: Mức độ ưu tiên (Priority) cao hơn -> ĐỨNG TRƯỚC!
	if (BuildingA->ConstructionPriority != BuildingB->ConstructionPriority)
	{
		return BuildingA->ConstructionPriority > BuildingB->ConstructionPriority;
	}

	// 2. TẦNG 1.5: Ưu tiên công trình đã có kết nối đường hoàn chỉnh trước
	if (BuildingA->bIsConnectedToDistrict != BuildingB->bIsConnectedToDistrict)
	{
		return BuildingA->bIsConnectedToDistrict;
	}

	// 3. TẦNG 2 (FIFO): Cùng Priority -> Đặt móng trước (PlacementOrderIndex nhỏ hơn) -> ĐỨNG TRƯỚC!
	return BuildingA->PlacementOrderIndex < BuildingB->PlacementOrderIndex;
}

UTimberConstructionManager::UTimberConstructionManager()
{
	GlobalPlacementCounter = 0;
}

void UTimberConstructionManager::Initialize(ATimberGridManager* InGridManager)
{
	GridManager = InGridManager;
	GlobalPlacementCounter = 0;
	ConstructionQueue.Empty();
	UE_LOG(LogTemp, Log, TEXT("UTimberConstructionManager: Khởi tạo thành công Sub-Class Quản lý Xây dựng!"));
}

void UTimberConstructionManager::RegisterConstructionSite(ATimberBuildingBase* Building)
{
	if (!Building || Building->BuildingState != EBuildingState::UnderConstruction)
	{
		return;
	}

	// Cấp phát số thứ tự thời gian FIFO duy nhất
	Building->PlacementOrderIndex = ++GlobalPlacementCounter;

	// Thêm vào hàng đợi và sắp xếp lại
	ConstructionQueue.AddUnique(Building);
	SortConstructionQueue();

	UE_LOG(LogTemp, Log, TEXT("UTimberConstructionManager: Đã đăng ký móng [%s] (Priority: %d, FIFO ID: %lld). Tổng hàng đợi: %d"),
		*Building->BuildingName, Building->ConstructionPriority, Building->PlacementOrderIndex, ConstructionQueue.Num());
}

void UTimberConstructionManager::UnregisterConstructionSite(ATimberBuildingBase* Building)
{
	if (!Building)
	{
		return;
	}

	const int32 Removed = ConstructionQueue.Remove(Building);
	if (Removed > 0)
	{
		SortConstructionQueue();
		UE_LOG(LogTemp, Log, TEXT("UTimberConstructionManager: Đã loại bỏ công trình [%s] khỏi hàng đợi. Còn lại: %d"),
			*Building->BuildingName, ConstructionQueue.Num());
	}
}

void UTimberConstructionManager::SortConstructionQueue()
{
	// Dọn dẹp các con trỏ rác trước khi sắp xếp
	ConstructionQueue.RemoveAll([](const TWeakObjectPtr<ATimberBuildingBase>& Ptr) {
		return !Ptr.IsValid() || Ptr->BuildingState == EBuildingState::Completed;
	});

	// Sắp xếp 2 tầng theo đúng tiêu chuẩn Priority + FIFO
	ConstructionQueue.Sort(FBuildingQueuePredicate());
}

ATimberBuildingBase* UTimberConstructionManager::GetNextSiteNeedingWood() const
{
	for (const TWeakObjectPtr<ATimberBuildingBase>& SitePtr : ConstructionQueue)
	{
		if (SitePtr.IsValid())
		{
			ATimberBuildingBase* Site = SitePtr.Get();
			
			// Móng chỉ được phát việc khi đã nối mạng đường về District Center.
			if (Site->BuildingState == EBuildingState::UnderConstruction &&
				Site->bIsConnectedToDistrict && Site->NeedsMoreWoodDelivery())
			{
				return Site;
			}
		}
	}

	return nullptr;
}

ATimberBuildingBase* UTimberConstructionManager::GetNextSiteNeedingBuilders() const
{
	for (const TWeakObjectPtr<ATimberBuildingBase>& SitePtr : ConstructionQueue)
	{
		if (SitePtr.IsValid())
		{
			ATimberBuildingBase* Site = SitePtr.Get();
			
			// Chỉ điều thợ tới móng đã đủ gỗ và còn kết nối District.
			if (Site->BuildingState == EBuildingState::UnderConstruction &&
				Site->bIsConnectedToDistrict && Site->HasAllRequiredWood())
			{
				// Kiểm tra công trình này còn slot cho thợ vào gõ búa không (tối đa theo Priority: 1-4 thợ)
				if (Site->CurrentActiveBuilders + Site->ReservedBuildersEnRoute < Site->GetMaxAllowedBuilders())
				{
					return Site;
				}
			}
		}
	}

	return nullptr;
}

bool UTimberConstructionManager::ReserveBuilderSlot(ATimberBuildingBase* TargetSite)
{
	if (!TargetSite || TargetSite->BuildingState != EBuildingState::UnderConstruction ||
		!TargetSite->bIsConnectedToDistrict || !TargetSite->HasAllRequiredWood())
	{
		return false;
	}

	if (TargetSite->CurrentActiveBuilders + TargetSite->ReservedBuildersEnRoute >= TargetSite->GetMaxAllowedBuilders())
	{
		return false;
	}

	++TargetSite->ReservedBuildersEnRoute;
	UE_LOG(LogTemp, Warning, TEXT("[BUILDER RESERVE] Site='%s' Active=%d EnRoute=%d Max=%d"),
		*TargetSite->BuildingName, TargetSite->CurrentActiveBuilders,
		TargetSite->ReservedBuildersEnRoute, TargetSite->GetMaxAllowedBuilders());
	return true;
}

void UTimberConstructionManager::ReleaseReservedBuilderSlot(ATimberBuildingBase* TargetSite)
{
	if (TargetSite)
	{
		TargetSite->ReservedBuildersEnRoute = FMath::Max(0, TargetSite->ReservedBuildersEnRoute - 1);
	}
}

bool UTimberConstructionManager::FindBestWoodSourceForSite(ATimberBuildingBase* TargetSite, const FIntVector& BeaverLocation,
	FIntVector& OutSourceCoord, ATimberBuildingBase*& OutSourceBuilding)
{
	OutSourceCoord = FIntVector::ZeroValue;
	OutSourceBuilding = nullptr;

	if (!TargetSite || !GridManager.IsValid())
	{
		return false;
	}

	ATimberGridManager* Grid = GridManager.Get();

	// ========================================================
	// BƯỚC 1: QUÉT TOÀN BỘ KHO VÀ NHÀ CHÍNH TRONG THẾ GIỚI
	// ========================================================
	int32 BestStorageDist = MAX_int32;
	bool bFoundStorageWithWood = false;

	for (const TObjectPtr<ATimberBuildingBase>& BuildingPtr : Grid->RegisteredBuildings)
	{
		if (IsValid(BuildingPtr) && BuildingPtr->IsStorageFacility() &&
			BuildingPtr->BuildingState == EBuildingState::Completed &&
			BuildingPtr->bIsConnectedToDistrict)
		{
			// Kiểm tra kho hoặc nhà chính có gỗ không (> 0)
			if (BuildingPtr->GetCurrentStoredAmount() > 0)
			{
				// Cửa là access point chuẩn. Chỉ fallback chu vi nếu Cửa không có DirtPath.
				TArray<FIntVector> AccessibleCoords;
				const FIntVector DoorCoord = BuildingPtr->GetDoorGridCoord();
				if (Grid->HasPathAt(DoorCoord))
				{
					AccessibleCoords.Add(DoorCoord);
				}
				if (Grid->HasPathAt(FIntVector(DoorCoord.X, DoorCoord.Y, DoorCoord.Z + 1)))
				{
					AccessibleCoords.AddUnique(FIntVector(DoorCoord.X, DoorCoord.Y, DoorCoord.Z + 1));
				}
				if (AccessibleCoords.IsEmpty())
				{
					AccessibleCoords = BuildingPtr->GetPerimeterAdjacentCoords();
				}

				for (const FIntVector& BaseCandidateCoord : AccessibleCoords)
				{
					FIntVector CandidateCoord = BaseCandidateCoord;
					if (!Grid->HasPathAt(CandidateCoord))
					{
						CandidateCoord.Z += 1;
					}
					if (!Grid->HasPathAt(CandidateCoord))
					{
						continue;
					}

					TArray<FIntVector> CandidatePath;
					if (!Grid->FindPath(BeaverLocation, CandidateCoord, CandidatePath, true))
					{
						continue;
					}

					const int32 ManhattanDist = FMath::Abs(CandidateCoord.X - BeaverLocation.X) +
					                            FMath::Abs(CandidateCoord.Y - BeaverLocation.Y) +
					                            FMath::Abs(CandidateCoord.Z - BeaverLocation.Z) * 2;
					const int32 Score = CandidatePath.Num() * 1000 + ManhattanDist;

					if (Score < BestStorageDist)
					{
						BestStorageDist = Score;
						OutSourceCoord = CandidateCoord;
						OutSourceBuilding = BuildingPtr.Get();
						bFoundStorageWithWood = true;
					}
				}
			}
		}
	}

	if (bFoundStorageWithWood)
	{
		return true; // Ưu tiên 1: Rút gỗ từ Kho / Nhà Chính gần nhất!
	}

	// Kho hết gỗ: không phát việc. Chặt cây chỉ thuộc Lumberjack đã được assign.
	return false;
}

bool UTimberConstructionManager::AssignBuilderToSite(ATimberBuildingBase* TargetSite, ABeaverAgent* Beaver)
{
	if (!TargetSite || !Beaver || TargetSite->BuildingState != EBuildingState::UnderConstruction)
	{
		return false;
	}

	// Agent phải sở hữu một slot đã giữ từ trước khi bắt đầu di chuyển.
	if (TargetSite->ReservedBuildersEnRoute <= 0)
	{
		return false;
	}

	--TargetSite->ReservedBuildersEnRoute;
	TargetSite->CurrentActiveBuilders++;
	UE_LOG(LogTemp, Warning, TEXT("[BUILDER START] Beaver='%s' Site='%s' Active=%d EnRoute=%d Max=%d"),
		*Beaver->BeaverName, *TargetSite->BuildingName, TargetSite->CurrentActiveBuilders,
		TargetSite->ReservedBuildersEnRoute, TargetSite->GetMaxAllowedBuilders());

	return true;
}

void UTimberConstructionManager::ReleaseBuilderFromSite(ATimberBuildingBase* TargetSite, ABeaverAgent* Beaver)
{
	if (!TargetSite)
	{
		return;
	}

	TargetSite->CurrentActiveBuilders = FMath::Max(0, TargetSite->CurrentActiveBuilders - 1);
	UE_LOG(LogTemp, Log, TEXT("UTimberConstructionManager: Đã giải phóng thợ khỏi móng [%s] (Thợ còn lại: %d)."),
		*TargetSite->BuildingName, TargetSite->CurrentActiveBuilders);
}

void UTimberConstructionManager::AdvanceCooperativeConstruction(ATimberBuildingBase* TargetSite, float DeltaTime)
{
	if (!TargetSite || TargetSite->BuildingState != EBuildingState::UnderConstruction)
	{
		return;
	}

	if (!TargetSite->HasAllRequiredWood())
	{
		return; // Chưa đủ 100% gỗ, chưa cho phép xây
	}

	if (TargetSite->CurrentActiveBuilders <= 0)
	{
		return; // Không có thợ nào đang gõ búa
	}

	// Hàm được gọi một lần mỗi frame bởi từng builder đang Working.
	// Mỗi lần gọi chỉ cộng phần việc của chính builder đó; tổng tốc độ tự tăng tuyến tính theo số thợ.
	const float BaseDuration = (TargetSite->BuildTimeSeconds > 0.0f) ? TargetSite->BuildTimeSeconds : 10.0f;
	const float SpeedPerBuilder = 1.0f / BaseDuration;

	TargetSite->CurrentBuildProgress = FMath::Clamp(TargetSite->CurrentBuildProgress + (DeltaTime * SpeedPerBuilder), 0.0f, 1.0f);

	if (TargetSite->CurrentBuildProgress >= 1.0f)
	{
		TargetSite->SetBuildingState(EBuildingState::Completed);
		UnregisterConstructionSite(TargetSite);

		UE_LOG(LogTemp, Log, TEXT("UTimberConstructionManager: 🎉 CÔNG TRÌNH [%s] ĐÃ ĐƯỢC XÂY DỰNG HOÀN THÀNH 100%%! Bắt đầu vận hành."),
			*TargetSite->BuildingName);
	}
}
