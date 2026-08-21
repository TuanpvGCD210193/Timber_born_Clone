// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Beavers/BeaverAgent.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "Timber_born_Clone/Public/Buildings/TimberDistrictCenter.h"
#include "Timber_born_Clone/Public/Buildings/TimberLumberjackFlag.h"
#include "Timber_born_Clone/Public/Buildings/TimberConstructionManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ABeaverAgent::ABeaverAgent()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Tạo Capsule Component làm Root, bật Overlap nhẹ nhàng chống kẹt xe
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(35.0f, 40.0f);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComponent->SetCollisionObjectType(ECC_Pawn);
	CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = CapsuleComponent;

	// 2. Mesh 3D thân hình của chú Hải ly
	BodyMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMeshComponent"));
	BodyMeshComponent->SetupAttachment(RootComponent);
	BodyMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
	BodyMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. Mesh 3D khúc gỗ hiển thị trên lưng khi đang vác tài nguyên (mặc định ẩn)
	CarriedWoodMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarriedWoodMeshComponent"));
	CarriedWoodMeshComponent->SetupAttachment(BodyMeshComponent);
	CarriedWoodMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	CarriedWoodMeshComponent->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.8f));
	CarriedWoodMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarriedWoodMeshComponent->SetVisibility(false);

	// Các thông số mặc định ban đầu
	CurrentState = EBeaverState::Idle;
	CurrentProfession = EBeaverProfession::Unemployed;
	CurrentStamina = AttributeConfig.MaxStamina;
	DebugLevel = 1;
}

void ABeaverAgent::BeginPlay()
{
	Super::BeginPlay();

	// Tạo tên Debug duy nhất nếu Blueprint vẫn dùng tên mặc định "Beaver".
	static int32 DebugBeaverCounter = 0;
	if (BeaverName.IsEmpty() || BeaverName == TEXT("Beaver"))
	{
		BeaverName = FString::Printf(TEXT("Beaver #%d"), ++DebugBeaverCounter);
	}

	// Tự động căn chỉnh tọa độ Grid ban đầu theo vị trí Spawn
	if (ATimberGridManager* Grid = GetGridManager())
	{
		CurrentGridCoord = Grid->WorldLocationToGridCoord(GetActorLocation());
		for (const TObjectPtr<ATimberBuildingBase>& Building : Grid->RegisteredBuildings)
		{
			if (ATimberDistrictCenter* DistrictCenter = Cast<ATimberDistrictCenter>(Building.Get()))
			{
				AssignedDistrictCenter = DistrictCenter;
				break;
			}
		}
		IdleRoamWaitRemaining = FMath::FRandRange(IdleRoamMinWait, IdleRoamMaxWait);
		UE_LOG(LogTemp, Warning, TEXT("🐾 [BEAVER SPAWN] '%s' World=%s Grid=%s"),
			*BeaverName, *GetActorLocation().ToString(), *CurrentGridCoord.ToString());
	}
}

void ABeaverAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Cập nhật di chuyển theo danh sách Waypoints A*
	UpdateMovement(DeltaTime);

	// 2. Cập nhật máy trạng thái FSM
	UpdateFSM(DeltaTime);

	// 3. Vẽ thông tin Debug 3D nếu DebugLevel > 0
	if (DebugLevel > 0)
	{
		DrawDebugVisuals();
	}
}

bool ABeaverAgent::MoveToGridCoord(const FIntVector& DestinationCoord)
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return false;
	}

	TargetGridCoord = DestinationCoord;

	// Gọi thuật toán A* tìm lộ trình từng bước chân
	TArray<FIntVector> GridPath;
	if (Grid->FindPath(CurrentGridCoord, TargetGridCoord, GridPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("🧭 [MOVE START] '%s' Phase=%s Current=%s Target=%s PathNodes=%d"),
			*BeaverName, *GetConstructionTaskDebugString(), *CurrentGridCoord.ToString(),
			*TargetGridCoord.ToString(), GridPath.Num());
		CurrentPathWaypoints.Empty();
		CurrentWaypointIndex = 0;

		// Chuyển đổi từng ô GridCoord thành tọa độ thế giới 3D (Tâm ô, nâng Z nhẹ 40cm cho chân tiếp xúc mặt đất)
		for (const FIntVector& Coord : GridPath)
		{
			const FVector WorldPos = Grid->GridCoordToWorldLocation(Coord, true) + FVector(0.0f, 0.0f, 40.0f);
			CurrentPathWaypoints.Add(WorldPos);
		}

		if (CurrentPathWaypoints.Num() > 0)
		{
			SetBeaverState(EBeaverState::MovingToTarget);
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("❌ [MOVE FAILED] '%s' Phase=%s Current=%s Target=%s"),
		*BeaverName, *GetConstructionTaskDebugString(), *CurrentGridCoord.ToString(), *TargetGridCoord.ToString());

	return false;
}

void ABeaverAgent::UpdateMovement(float DeltaTime)
{
	if (CurrentState != EBeaverState::MovingToTarget || CurrentPathWaypoints.Num() == 0)
	{
		return;
	}

	if (!CurrentPathWaypoints.IsValidIndex(CurrentWaypointIndex))
	{
		// Đã hoàn thành toàn bộ lộ trình Waypoints
		CurrentPathWaypoints.Empty();
		CurrentWaypointIndex = 0;
		SetBeaverState(EBeaverState::Idle);
		return;
	}

	const FVector TargetLocation = CurrentPathWaypoints[CurrentWaypointIndex];
	const FVector CurrentLocation = GetActorLocation();

	// Kiểm tra xem ô hiện tại có phải là Đường Đất DirtPath để áp dụng hệ số tăng tốc +50%
	float ActualSpeed = AttributeConfig.BaseMoveSpeed;
	if (ATimberGridManager* Grid = GetGridManager())
	{
		if (Grid->HasPathAt(CurrentGridCoord))
		{
			ActualSpeed *= AttributeConfig.RoadSpeedMultiplier;
		}
	}

	// 1. TÍNH TOÁN NỘI SUY VỊ TRÍ (Grid Linear Interpolation)
	const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaTime, ActualSpeed);
	SetActorLocation(NewLocation);

	// 2. NỘI SUY XOAY MÌNH HƯỚNG THEO HƯỚNG DI CHUYỂN
	FVector MoveDirection = (TargetLocation - CurrentLocation);
	MoveDirection.Z = 0.0f; // Khóa trục Z để không bị chúi đầu xuống đất

	if (!MoveDirection.IsNearlyZero())
	{
		const FRotator TargetRotation = MoveDirection.Rotation();
		const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 12.0f);
		SetActorRotation(NewRotation);
	}

	// 3. KIỂM TRA ĐÃ CHẠM ĐÍCH Ô TIẾP THEO CHƯA.
	// Bắt buộc dùng khoảng cách 3D: Dist2D có thể kết thúc waypoint sớm khi XY đã tới
	// nhưng Z vẫn nằm ở tầng khác, làm sai CurrentGridCoord tại Kho/Móng trên cao.
	if (FVector::Dist(NewLocation, TargetLocation) < 8.0f)
	{
		// Snap chính xác vào tâm waypoint để loại bỏ sai số tích lũy World <-> Grid.
		SetActorLocation(TargetLocation);

		if (ATimberGridManager* Grid = GetGridManager())
		{
			CurrentGridCoord = Grid->WorldLocationToGridCoord(TargetLocation);
		}

		CurrentWaypointIndex++;

		// Nếu vừa bước tới điểm cuối cùng của toàn bộ dải đường
		if (CurrentWaypointIndex >= CurrentPathWaypoints.Num())
		{
			CurrentPathWaypoints.Empty();
			CurrentWaypointIndex = 0;

			// Tọa độ grid được A* trả về mới là nguồn sự thật ở điểm cuối.
			// Không suy ngược từ vị trí mesh đã cộng offset chân (+40cm).
			CurrentGridCoord = TargetGridCoord;

			if (bIsIdleRoaming)
			{
				bIsIdleRoaming = false;
				IdleRoamWaitRemaining = FMath::FRandRange(IdleRoamMinWait, IdleRoamMaxWait);
				SetBeaverState(EBeaverState::Idle);
				return;
			}

			// Construction dùng job phase tường minh và source/target đã khóa từ lúc nhận việc.
			// Không quét lại building hoặc suy luận hành động từ CarriedWoodAmount.
			if (ConstructionTaskPhase != EConstructionTaskPhase::None)
			{
				HandleConstructionMovementCompleted();
				return;
			}

			// Nếu đang là thợ đốn gỗ và vừa bước tới ô cạnh cây gỗ
			if (CurrentProfession == EBeaverProfession::Lumberjack && TargetTreeCoord != FIntVector::ZeroValue && CarriedWoodAmount == 0)
			{
				// Xoay mặt nhìn thẳng vào thân cây gỗ
				if (ATimberGridManager* Grid = GetGridManager())
				{
					const FVector TreeWorldPos = Grid->GridCoordToWorldLocation(TargetTreeCoord, true);
					FVector LookDir = TreeWorldPos - GetActorLocation();
					LookDir.Z = 0.0f;
					if (!LookDir.IsNearlyZero())
					{
						SetActorRotation(LookDir.Rotation());
					}
				}

				CurrentChopProgressTimer = 0.0f;
				SetBeaverState(EBeaverState::Working);
			}
			// Nếu đang vác gỗ trên lưng và vừa bước tới đích (Móng công trình)
			else if (CarriedWoodAmount > 0)
			{
				if (ATimberGridManager* Grid = GetGridManager())
				{
					// 1. Kiểm tra xem có phải bước tới Móng công trình đang cần gỗ không (chấp nhận ô cửa hoặc ô tiếp giáp chu vi móng)
					if (TargetConstructionBuilding.IsValid() && TargetConstructionBuilding->BuildingState == EBuildingState::UnderConstruction)
					{
						if (IsBuildingAccessCoord(TargetConstructionBuilding.Get(), CurrentGridCoord))
						{
							const int32 Delivered = TargetConstructionBuilding->AddDeliveredWood(CarriedWoodAmount);
							if (Delivered > 0)
							{
								UE_LOG(LogTemp, Warning, TEXT("🪵 [SUPPLY WOOD] Hải ly '%s' đã nạp %d gỗ vào móng '%s'! (Móng hiện có: %d/%d)"),
									*BeaverName, Delivered, *TargetConstructionBuilding->BuildingName,
									TargetConstructionBuilding->CurrentWoodDelivered, TargetConstructionBuilding->WoodCost);

								CarriedWoodAmount -= Delivered;
								ReservedConstructionWoodAmount = FMath::Max(0, ReservedConstructionWoodAmount - Delivered);
							}
						}
					}
					// 2. Nếu không phải móng, kiểm tra xem có phải nạp vào Kho không
					else
					{
						for (const TObjectPtr<ATimberBuildingBase>& BuildingPtr : Grid->RegisteredBuildings)
						{
							if (IsValid(BuildingPtr) && BuildingPtr->IsStorageFacility())
							{
								if (IsBuildingAccessCoord(BuildingPtr.Get(), CurrentGridCoord))
								{
									const int32 Stored = BuildingPtr->StoreResource(CarriedWoodAmount);
									if (Stored > 0)
									{
										UE_LOG(LogTemp, Warning, TEXT("🪵 [DEPOSIT WOOD] Hải ly '%s' đã nạp thành công %d gỗ vào '%s'! (Kho hiện có: %d/%d)"),
											*BeaverName, Stored, *BuildingPtr->BuildingName,
											BuildingPtr->GetCurrentStoredAmount(), BuildingPtr->GetMaxStorageCapacity());

										CarriedWoodAmount -= Stored;
									}
									break;
								}
							}
						}
					}
				}

				// Cập nhật hiển thị khúc gỗ trên lưng
				if (CarriedWoodMeshComponent)
				{
					CarriedWoodMeshComponent->SetVisibility(CarriedWoodAmount > 0);
				}

				TargetTreeCoord = FIntVector::ZeroValue;
				SetBeaverState(EBeaverState::Idle);

				// Tự động tìm việc tiếp theo
				if (CurrentProfession == EBeaverProfession::Lumberjack)
				{
					StartLumberjackWorkLoop();
				}
				else
				{
					StartConstructionWorkLoop();
				}
			}
			// Nếu vừa bước tới kho để RÚT GỖ mang đi xây
			else if (TargetConstructionBuilding.IsValid() && TargetConstructionBuilding->BuildingState == EBuildingState::UnderConstruction && CarriedWoodAmount == 0)
			{
				if (ATimberGridManager* Grid = GetGridManager())
				{
					// Kiểm tra xem vị trí vừa bước tới có phải là Kho / Nhà Chính không
					for (const TObjectPtr<ATimberBuildingBase>& BuildingPtr : Grid->RegisteredBuildings)
					{
						if (IsValid(BuildingPtr) && BuildingPtr->IsStorageFacility())
						{
							if (IsBuildingAccessCoord(BuildingPtr.Get(), CurrentGridCoord))
							{
								const int32 Needed = TargetConstructionBuilding->GetRemainingWoodNeeded();
								const int32 Requested = FMath::Min(ReservedConstructionWoodAmount, AttributeConfig.MaxWoodCarryCapacity);
								const int32 Taken = BuildingPtr->WithdrawResource(FMath::Min(Needed + ReservedConstructionWoodAmount, Requested));

								if (Taken > 0)
								{
									if (Taken < ReservedConstructionWoodAmount)
									{
										const int32 Unfulfilled = ReservedConstructionWoodAmount - Taken;
										TargetConstructionBuilding->ReservedWoodDelivering = FMath::Max(
											0, TargetConstructionBuilding->ReservedWoodDelivering - Unfulfilled);
										ReservedConstructionWoodAmount = Taken;
									}
									AddCarriedWood(Taken);

									UE_LOG(LogTemp, Warning, TEXT("🪵 [TAKE WOOD] Hải ly '%s' đã rút %d gỗ từ '%s'! Đang mang tới móng '%s'..."),
										*BeaverName, Taken, *BuildingPtr->BuildingName, *TargetConstructionBuilding->BuildingName);

									// Chặng 2: Chọn ô DirtPath tiếp cận móng có đường A* ngắn nhất.
									FIntVector SiteAccessCoord;
									if (FindBestRoadAccessCoord(TargetConstructionBuilding.Get(), SiteAccessCoord) &&
										MoveToGridCoord(SiteAccessCoord))
									{
										return;
									}

									CancelConstructionAssignment();
									SetBeaverState(EBeaverState::Idle);
									return;
								}
								else
								{
									CancelConstructionAssignment();
								}
								break;
							}
						}
					}

					// Nếu là vừa bước tới móng để thi công gõ búa (móng đã đủ 100% gỗ)
					if (IsBuildingAccessCoord(TargetConstructionBuilding.Get(), CurrentGridCoord))
					{
						if (TargetConstructionBuilding->HasAllRequiredWood())
						{
							if (UTimberConstructionManager* CM = Grid->GetConstructionManager())
							{
								if (CM->AssignBuilderToSite(TargetConstructionBuilding.Get(), this))
								{
									SetBeaverState(EBeaverState::Working);
									return;
								}
							}
						}
					}
				}

				SetBeaverState(EBeaverState::Idle);
			}
			else
			{
				SetBeaverState(EBeaverState::Idle);
			}
		}
	}
}

void ABeaverAgent::UpdateFSM(float DeltaTime)
{
	switch (CurrentState)
	{
	case EBeaverState::Idle:
		// Nếu là thợ đốn gỗ đã assign và xưởng đã hoàn thành 100% -> Tự động tìm cây để chặt
		if (CurrentProfession == EBeaverProfession::Lumberjack && AssignedWorkplaceFlag.IsValid() && AssignedWorkplaceFlag->BuildingState == EBuildingState::Completed)
		{
			StartLumberjackWorkLoop();
		}
		// Nếu là Hải ly tự do (hoặc xưởng chưa xây xong) -> Tự động quét hàng đợi móng để xây dựng!
		else
		{
			if (!StartConstructionWorkLoop())
			{
				IdleRoamWaitRemaining -= DeltaTime;
				if (IdleRoamWaitRemaining <= 0.0f)
				{
					TryStartIdleRoam();
				}
			}
		}
		break;

	case EBeaverState::Working:
		// TH 1: Đang gõ búa xây dựng công trình móng
		if (TargetConstructionBuilding.IsValid() && TargetConstructionBuilding->BuildingState == EBuildingState::UnderConstruction)
		{
			if (!TargetConstructionBuilding->bIsConnectedToDistrict)
			{
				if (ATimberGridManager* Grid = GetGridManager())
				{
					if (UTimberConstructionManager* CM = Grid->GetConstructionManager())
					{
						CM->ReleaseBuilderFromSite(TargetConstructionBuilding.Get(), this);
					}
				}
				CancelConstructionAssignment();
				SetBeaverState(EBeaverState::Idle);
				break;
			}

			if (ATimberGridManager* Grid = GetGridManager())
			{
				if (UTimberConstructionManager* CM = Grid->GetConstructionManager())
				{
					CM->AdvanceCooperativeConstruction(TargetConstructionBuilding.Get(), DeltaTime);

					// Nếu công trình đã hoàn thành 100% -> Thôi gõ búa, quay về Idle tìm việc khác!
					if (TargetConstructionBuilding->BuildingState == EBuildingState::Completed)
					{
						CM->ReleaseBuilderFromSite(TargetConstructionBuilding.Get(), this);
						ConstructionTaskPhase = EConstructionTaskPhase::None;
						TargetConstructionBuilding = nullptr;
						SetBeaverState(EBeaverState::Idle);
					}
				}
			}
		}
		// Một builder khác vừa hoàn tất móng trong cùng frame: giải phóng toàn bộ builder còn lại.
		else if (ConstructionTaskPhase == EConstructionTaskPhase::Building)
		{
			if (ATimberGridManager* Grid = GetGridManager())
			{
				if (UTimberConstructionManager* CM = Grid->GetConstructionManager())
				{
					CM->ReleaseBuilderFromSite(TargetConstructionBuilding.Get(), this);
				}
			}
			ConstructionTaskPhase = EConstructionTaskPhase::None;
			TargetConstructionBuilding = nullptr;
			SetBeaverState(EBeaverState::Idle);
		}
		// TH 2: Đang đốn cây gỗ (Lumberjack)
		else if (TargetTreeCoord != FIntVector::ZeroValue)
		{
			// Đang đốn cây gỗ: Đếm ngược thời gian TreeChopDuration
			CurrentChopProgressTimer += DeltaTime;

			// Tiêu hao nhẹ thể lực khi làm việc
			CurrentStamina = FMath::Clamp(CurrentStamina - AttributeConfig.StaminaDrainRate * DeltaTime, 0.0f, AttributeConfig.MaxStamina);

			// Khi đã đốn đủ 3 giây (100%)
			if (CurrentChopProgressTimer >= AttributeConfig.TreeChopDuration)
			{
				CurrentChopProgressTimer = 0.0f;

				if (ATimberGridManager* Grid = GetGridManager())
				{
					// 1. Chặt cây gỗ thông qua API ChopTree của GridManager (Tự động biến thành TreeStump và cập nhật ISM)
					int32 WoodEarned = 0;
					Grid->ChopTree(TargetTreeCoord, WoodEarned);

					// 2. Nhận Khúc gỗ lên lưng
					AddCarriedWood(FMath::Max(1, WoodEarned));

					UE_LOG(LogTemp, Warning, TEXT("🪓 [CHOP SUCCESS] Hải ly '%s' đã đốn hạ thành công cây tại (%d, %d, %d)! Gỗ trên lưng: %d"),
						*BeaverName, TargetTreeCoord.X, TargetTreeCoord.Y, TargetTreeCoord.Z, CarriedWoodAmount);

					// 3. Tìm kho gần nhất còn chỗ chứa để vác gỗ về cất
					FIntVector StorageDoor;
					ATimberBuildingBase* StorageBuilding = nullptr;
					if (FindNearestAvailableStorage(StorageDoor, StorageBuilding))
					{
						MoveToGridCoord(StorageDoor);
					}
					else
					{
						SetBeaverState(EBeaverState::Idle);
					}
				}
			}
		}
		break;

	case EBeaverState::MovingToTarget:
		// Đi dạo là tác vụ nền: công việc thật luôn được quyền ngắt nó ngay lập tức.
		if (bIsIdleRoaming && CurrentProfession == EBeaverProfession::Unemployed)
		{
			if (StartConstructionWorkLoop())
			{
				bIsIdleRoaming = false;
			}
		}

		// Mất kết nối District giữa hành trình: hủy job và hoàn reservation ngay.
		if (TargetConstructionBuilding.IsValid() && !TargetConstructionBuilding->bIsConnectedToDistrict)
		{
			CurrentPathWaypoints.Empty();
			CurrentWaypointIndex = 0;
			CancelConstructionAssignment();
			SetBeaverState(EBeaverState::Idle);
		}
		break;

	case EBeaverState::CarryingResource:
		// Hiển thị khúc gỗ trên lưng
		if (CarriedWoodMeshComponent)
		{
			CarriedWoodMeshComponent->SetVisibility(CarriedWoodAmount > 0);
		}
		break;

	case EBeaverState::Resting:
		// Hồi phục thể lực
		CurrentStamina = FMath::Clamp(CurrentStamina + AttributeConfig.StaminaRecoveryRate * DeltaTime, 0.0f, AttributeConfig.MaxStamina);
		if (CurrentStamina >= AttributeConfig.MaxStamina)
		{
			SetBeaverState(EBeaverState::Idle);
		}
		break;
	}
}

void ABeaverAgent::AssignWorkplace(ATimberLumberjackFlag* InFlag)
{
	AssignedWorkplaceFlag = InFlag;
	CurrentProfession = EBeaverProfession::Lumberjack;
	StartLumberjackWorkLoop();
}

void ABeaverAgent::ClearWorkplace()
{
	AssignedWorkplaceFlag = nullptr;
	CurrentProfession = EBeaverProfession::Unemployed;
	TargetTreeCoord = FIntVector::ZeroValue;
	CurrentChopProgressTimer = 0.0f;
	SetBeaverState(EBeaverState::Idle);
}

void ABeaverAgent::StartLumberjackWorkLoop()
{
	if (!AssignedWorkplaceFlag.IsValid() || AssignedWorkplaceFlag->BuildingState != EBuildingState::Completed)
	{
		// Nếu xưởng chưa xây xong -> Tự động tham gia hỗ trợ chuỗi xây dựng
		StartConstructionWorkLoop();
		return;
	}

	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return;
	}

	// 1. Quét tìm cây trưởng thành gần nhất bên trong Vùng Xanh (WorkRadius) của Flag
	FIntVector FoundTreeCoord;
	if (AssignedWorkplaceFlag->FindNearestMatureTreeInWorkRadius(GetActorLocation(), FoundTreeCoord))
	{
		TargetTreeCoord = FoundTreeCoord;

		// 2. Tìm ô đất trống lân cận quanh gốc cây để bước tới (CHỐNG KẸT 100%)
		const FIntVector Offsets[4] = {
			FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
			FIntVector(0, 1, 0), FIntVector(0, -1, 0)
		};

		FIntVector BestAdjacentCoord = FIntVector::ZeroValue;
		int32 BestDist = MAX_int32;
		bool bFoundAdjacent = false;

		for (const FIntVector& Offset : Offsets)
		{
			const int32 AdjX = TargetTreeCoord.X + Offset.X;
			const int32 AdjY = TargetTreeCoord.Y + Offset.Y;

			FIntVector GroundCoord;
			if (Grid->GetTopSolidGridCoordAt(AdjX, AdjY, GroundCoord))
			{
				const FIntVector StandCoord = FIntVector(AdjX, AdjY, GroundCoord.Z + 1);

				// Ô đứng phải là ô trống và chênh lệch Z <= 1 so với cây
				if (Grid->IsCellEmptyForBuilding(StandCoord) && FMath::Abs(GroundCoord.Z - (TargetTreeCoord.Z - 1)) <= 1)
				{
					const int32 Dist = FMath::Abs(StandCoord.X - CurrentGridCoord.X) + FMath::Abs(StandCoord.Y - CurrentGridCoord.Y);
					if (Dist < BestDist)
					{
						BestDist = Dist;
						BestAdjacentCoord = StandCoord;
						bFoundAdjacent = true;
					}
				}
			}
		}

		if (bFoundAdjacent)
		{
			// Di chuyển tới ô đất cạnh cây
			if (MoveToGridCoord(BestAdjacentCoord))
			{
				UE_LOG(LogTemp, Log, TEXT("🦫 [LUMBERJACK] Hải ly '%s' đang di chuyển tới ô bên cạnh cây (%d, %d, %d)"),
					*BeaverName, BestAdjacentCoord.X, BestAdjacentCoord.Y, BestAdjacentCoord.Z);
			}
		}
	}
}

void ABeaverAgent::SetBeaverState(EBeaverState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;

	// Cập nhật hiển thị phụ trợ
	if (CarriedWoodMeshComponent)
	{
		CarriedWoodMeshComponent->SetVisibility(CarriedWoodAmount > 0);
	}
}

void ABeaverAgent::SetDebugLevel(int32 InLevel)
{
	DebugLevel = InLevel;
}

FString ABeaverAgent::GetStateDebugString(FColor& OutColor) const
{
	switch (CurrentState)
	{
	case EBeaverState::Idle:
		OutColor = FColor::Yellow;
		return TEXT("[IDLE]");

	case EBeaverState::MovingToTarget:
		if (bIsIdleRoaming)
		{
			OutColor = FColor::Yellow;
			return TEXT("[IDLE ROAM]");
		}
		OutColor = FColor::Cyan;
		return FString::Printf(TEXT("[MOVING -> (%d, %d, %d)]"), TargetGridCoord.X, TargetGridCoord.Y, TargetGridCoord.Z);

	case EBeaverState::Working:
		OutColor = FColor(255, 140, 0); // Màu Cam
		{
			const int32 Pct = FMath::Clamp(FMath::RoundToInt((CurrentChopProgressTimer / FMath::Max(0.1f, AttributeConfig.TreeChopDuration)) * 100.0f), 0, 100);
			return FString::Printf(TEXT("[WORKING]\n🪓 Chopping Tree: %d%%"), Pct);
		}

	case EBeaverState::CarryingResource:
		OutColor = FColor::Green;
		return FString::Printf(TEXT("[CARRYING WOOD: %d]"), CarriedWoodAmount);

	case EBeaverState::Resting:
		OutColor = FColor(186, 85, 211); // Màu Tím
		return TEXT("[RESTING / SLEEP]");
	}

	OutColor = FColor::White;
	return TEXT("[UNKNOWN]");
}

void ABeaverAgent::DrawDebugVisuals()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector BeaverHeadPos = GetActorLocation() + FVector(0.0f, 0.0f, 65.0f);

	// 1. MỨC 1: Hiện dòng chữ 3D Trạng thái FSM nổi trên đầu chú Hải ly
	if (DebugLevel >= 1)
	{
		FColor StateColor;
		const FString SourceName = ConstructionSourceBuilding.IsValid()
			? ConstructionSourceBuilding->BuildingName : TEXT("None");
		const FString SiteName = TargetConstructionBuilding.IsValid()
			? TargetConstructionBuilding->BuildingName : TEXT("None");
		const FString StateText = FString::Printf(
			TEXT("%s: %s\nC=%s -> T=%s | Task=%s"),
			*BeaverName, *GetStateDebugString(StateColor),
			*CurrentGridCoord.ToString(), *TargetGridCoord.ToString(),
			*GetConstructionTaskDebugString());
		DrawDebugString(World, BeaverHeadPos, StateText, nullptr, StateColor, 0.0f, true, 1.2f);

		if (DebugLevel >= 2)
		{
			const FString DetailText = FString::Printf(
				TEXT("Wood=%d Reserved=%d\nSource=%s | Site=%s"),
				CarriedWoodAmount, ReservedConstructionWoodAmount, *SourceName, *SiteName);
			DrawDebugString(World, BeaverHeadPos + FVector(0, 0, -45.0f), DetailText,
				nullptr, FColor::White, 0.0f, true, 1.0f);
		}
	}

	// 2. MỨC 2: Hiện Trạng thái FSM + Vẽ dải đường line A* mà Hải ly đang đi
	if (DebugLevel >= 2 && CurrentState == EBeaverState::MovingToTarget && CurrentPathWaypoints.Num() > 0)
	{
		// Vẽ tia nối từ Hải ly tới Waypoint kế tiếp
		if (CurrentPathWaypoints.IsValidIndex(CurrentWaypointIndex))
		{
			DrawDebugLine(World, GetActorLocation(), CurrentPathWaypoints[CurrentWaypointIndex], FColor::Green, false, 0.0f, 0, 2.5f);
		}

		// Vẽ dải đường nối toàn bộ các Waypoints còn lại trên mặt đất
		for (int32 i = CurrentWaypointIndex; i < CurrentPathWaypoints.Num() - 1; ++i)
		{
			DrawDebugLine(World, CurrentPathWaypoints[i], CurrentPathWaypoints[i + 1], FColor::Emerald, false, 0.0f, 0, 2.0f);
			DrawDebugSphere(World, CurrentPathWaypoints[i], 6.0f, 8, FColor::Cyan, false, 0.0f);
		}

		// Vẽ điểm đích cuối cùng
		if (CurrentPathWaypoints.Num() > 0)
		{
			DrawDebugSphere(World, CurrentPathWaypoints.Last(), 12.0f, 12, FColor::Yellow, false, 0.0f);
		}

		if (ATimberGridManager* Grid = GetGridManager())
		{
			const FVector CurrentCellPos = Grid->GridCoordToWorldLocation(CurrentGridCoord, true) + FVector(0, 0, 70.0f);
			const FVector TargetCellPos = Grid->GridCoordToWorldLocation(TargetGridCoord, true) + FVector(0, 0, 70.0f);
			DrawDebugSphere(World, CurrentCellPos, 18.0f, 12, FColor::Blue, false, 0.0f, 0, 3.0f);
			DrawDebugSphere(World, TargetCellPos, 22.0f, 12, FColor::Yellow, false, 0.0f, 0, 4.0f);

			if (ConstructionSourceBuilding.IsValid())
			{
				const FIntVector SourceDoor = ConstructionSourceBuilding->GetDoorGridCoord();
				const FVector SourceDoorPos = Grid->GridCoordToWorldLocation(SourceDoor, true) + FVector(0, 0, 90.0f);
				DrawDebugSphere(World, SourceDoorPos, 26.0f, 16, FColor::Magenta, false, 0.0f, 0, 4.0f);
				DrawDebugLine(World, GetActorLocation(), SourceDoorPos, FColor::Magenta, false, 0.0f, 0, 2.0f);
			}

			if (TargetConstructionBuilding.IsValid())
			{
				const FIntVector SiteDoor = TargetConstructionBuilding->GetDoorGridCoord();
				const FVector SiteDoorPos = Grid->GridCoordToWorldLocation(SiteDoor, true) + FVector(0, 0, 90.0f);
				DrawDebugSphere(World, SiteDoorPos, 26.0f, 16, FColor::Orange, false, 0.0f, 0, 4.0f);
			}
		}
	}
}

bool ABeaverAgent::FindNearestAvailableStorage(FIntVector& OutStorageDoorCoord, ATimberBuildingBase*& OutStorageBuilding) const
{
	OutStorageBuilding = nullptr;

	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return false;
	}

	int32 BestDist = MAX_int32;
	bool bFound = false;

	// Quét toàn bộ các công trình đã đăng ký trong thế giới
	for (const TObjectPtr<ATimberBuildingBase>& BuildingPtr : Grid->RegisteredBuildings)
	{
		if (IsValid(BuildingPtr) && BuildingPtr->IsStorageFacility() &&
			BuildingPtr->BuildingState == EBuildingState::Completed &&
			BuildingPtr->bIsConnectedToDistrict)
		{
			// Kiểm tra kho còn dung lượng chứa không
			if (BuildingPtr->GetCurrentStoredAmount() < BuildingPtr->GetMaxStorageCapacity())
			{
				FIntVector DoorCoord;
				if (!FindBestRoadAccessCoord(BuildingPtr.Get(), DoorCoord))
				{
					continue;
				}

				// Tính khoảng cách Manhattan tới cửa kho
				const int32 Dist = FMath::Abs(DoorCoord.X - CurrentGridCoord.X) +
				                   FMath::Abs(DoorCoord.Y - CurrentGridCoord.Y) +
				                   FMath::Abs(DoorCoord.Z - CurrentGridCoord.Z) * 2;

				if (Dist < BestDist)
				{
					BestDist = Dist;
					OutStorageDoorCoord = DoorCoord;
					OutStorageBuilding = BuildingPtr.Get();
					bFound = true;
				}
			}
		}
	}

	return bFound;
}

ATimberGridManager* ABeaverAgent::GetGridManager() const
{
	if (!CachedGridManager.IsValid())
	{
		if (const UWorld* World = GetWorld())
		{
			AActor* FoundGrid = UGameplayStatics::GetActorOfClass(World, ATimberGridManager::StaticClass());
			CachedGridManager = Cast<ATimberGridManager>(FoundGrid);
		}
	}

	return CachedGridManager.Get();
}

bool ABeaverAgent::TryStartIdleRoam()
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid || ConstructionTaskPhase != EConstructionTaskPhase::None || CarriedWoodAmount > 0)
	{
		return false;
	}

	if (!AssignedDistrictCenter.IsValid())
	{
		for (const TObjectPtr<ATimberBuildingBase>& Building : Grid->RegisteredBuildings)
		{
			if (ATimberDistrictCenter* DistrictCenter = Cast<ATimberDistrictCenter>(Building.Get()))
			{
				AssignedDistrictCenter = DistrictCenter;
				break;
			}
		}
	}

	if (!AssignedDistrictCenter.IsValid())
	{
		IdleRoamWaitRemaining = IdleRoamMaxWait;
		return false;
	}

	const FIntVector Center = AssignedDistrictCenter->GetDoorGridCoord();
	TArray<FIntVector> Candidates;
	for (int32 X = Center.X - IdleRoamRadius; X <= Center.X + IdleRoamRadius; ++X)
	{
		for (int32 Y = Center.Y - IdleRoamRadius; Y <= Center.Y + IdleRoamRadius; ++Y)
		{
			const int32 Distance = FMath::Abs(X - Center.X) + FMath::Abs(Y - Center.Y);
			if (Distance < 2 || Distance > IdleRoamRadius)
			{
				continue;
			}

			for (int32 ZOffset = -1; ZOffset <= 1; ++ZOffset)
			{
				const FIntVector Candidate(X, Y, Center.Z + ZOffset);
				if (Candidate != CurrentGridCoord && Grid->HasPathAt(Candidate))
				{
					Candidates.Add(Candidate);
				}
			}
		}
	}

	while (!Candidates.IsEmpty())
	{
		const int32 Index = FMath::RandRange(0, Candidates.Num() - 1);
		const FIntVector Destination = Candidates[Index];
		Candidates.RemoveAtSwap(Index);

		bIsIdleRoaming = true;
		if (MoveToGridCoord(Destination))
		{
			return true;
		}
		bIsIdleRoaming = false;
	}

	IdleRoamWaitRemaining = FMath::FRandRange(IdleRoamMinWait, IdleRoamMaxWait);
	return false;
}

int32 ABeaverAgent::AddCarriedWood(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const int32 OldWood = CarriedWoodAmount;
	CarriedWoodAmount = FMath::Clamp(CarriedWoodAmount + Amount, 0, AttributeConfig.MaxWoodCarryCapacity);
	const int32 Added = CarriedWoodAmount - OldWood;

	if (CarriedWoodMeshComponent)
	{
		CarriedWoodMeshComponent->SetVisibility(CarriedWoodAmount > 0);
	}

	return Added;
}

int32 ABeaverAgent::RemoveCarriedWood(int32 Amount)
{
	if (Amount <= 0 || CarriedWoodAmount <= 0)
	{
		return 0;
	}

	const int32 Removed = FMath::Min(Amount, CarriedWoodAmount);
	CarriedWoodAmount -= Removed;

	if (CarriedWoodMeshComponent)
	{
		CarriedWoodMeshComponent->SetVisibility(CarriedWoodAmount > 0);
	}

	return Removed;
}

bool ABeaverAgent::StartConstructionWorkLoop()
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return false;
	}

	UTimberConstructionManager* CM = Grid->GetConstructionManager();
	if (!CM)
	{
		return false;
	}

	if (TargetConstructionBuilding.IsValid() &&
		(!TargetConstructionBuilding->bIsConnectedToDistrict ||
		 TargetConstructionBuilding->BuildingState != EBuildingState::UnderConstruction))
	{
		CancelConstructionAssignment();
	}

	// ========================================================
	// TRƯỜNG HỢP 1: TRÊN VAI HẢI LY ĐÃ CÓ GỖ SẴN -> MANG GỖ TỚI MÓNG
	// ========================================================
	if (CarriedWoodAmount > 0)
	{
		// Tìm móng ưu tiên cao nhất đang cần nạp gỗ
		if (ATimberBuildingBase* SiteNeedingWood = CM->GetNextSiteNeedingWood())
		{
			TargetConstructionBuilding = SiteNeedingWood;

			FIntVector DropCoord;
			if (FindBestRoadAccessCoord(SiteNeedingWood, DropCoord))
			{
				ConstructionTaskPhase = EConstructionTaskPhase::MovingToSite;
				if (MoveToGridCoord(DropCoord))
				{
					return true;
				}
			}
			CancelConstructionAssignment();
		}

		// Nếu không có móng nào cần gỗ -> Vác trả về Kho
		FIntVector StorageDoor;
		ATimberBuildingBase* StorageBuilding = nullptr;
		if (FindNearestAvailableStorage(StorageDoor, StorageBuilding))
		{
			TargetConstructionBuilding = nullptr;
			return MoveToGridCoord(StorageDoor);
		}

		return false;
	}

	// ========================================================
	// TRƯỜNG HỢP 2: TRÊN VAI CHƯA CÓ GỖ -> BƯỚC 1: CHẠY TỚI KHO LẤY GỖ
	// ========================================================
	if (ATimberBuildingBase* SiteNeedingWood = CM->GetNextSiteNeedingWood())
	{
		const int32 RemainingNeeded = SiteNeedingWood->GetRemainingWoodNeeded();
		if (RemainingNeeded > 0)
		{
			// Quét tìm nguồn gỗ từ Kho/Nhà Chính (chỉ lấy gỗ có sẵn, không tự ý đi chặt cây nếu là dân tự do)
			FIntVector SourceCoord;
			ATimberBuildingBase* SourceBuilding = nullptr;
			if (CM->FindBestWoodSourceForSite(SiteNeedingWood, CurrentGridCoord, SourceCoord, SourceBuilding))
			{
				// Đặt chỗ đúng lượng gỗ thực tế cần lấy (tối đa sức vác của Agent).
				const int32 WoodToReserve = FMath::Clamp(RemainingNeeded, 1, AttributeConfig.MaxWoodCarryCapacity);
				SiteNeedingWood->ReservedWoodDelivering += WoodToReserve;
				ReservedConstructionWoodAmount = WoodToReserve;
				TargetConstructionBuilding = SiteNeedingWood;
				ConstructionSourceBuilding = SourceBuilding;
				ConstructionTaskPhase = EConstructionTaskPhase::MovingToSource;

				// CHẶNG 1: Di chuyển tới ô DirtPath tiếp cận Kho / Nhà Chính.
				if (MoveToGridCoord(SourceCoord))
				{
					UE_LOG(LogTemp, Warning, TEXT("🏃 [FETCH WOOD] Hải ly '%s' đã đặt chỗ %d gỗ, đang chạy tới kho tại (%d, %d, %d) cho '%s'..."),
						*BeaverName, WoodToReserve, SourceCoord.X, SourceCoord.Y, SourceCoord.Z, *SiteNeedingWood->BuildingName);
					return true;
				}
				CancelConstructionAssignment();
			}
		}
	}

	// ========================================================
	// TRƯỜNG HỢP 3: CÁC MÓNG ĐÃ ĐỦ GỖ -> ĐẾN GÕ BÚA THI CÔNG HỢP LỰC
	// ========================================================
	if (ATimberBuildingBase* SiteNeedingBuilders = CM->GetNextSiteNeedingBuilders())
	{
		if (CM->ReserveBuilderSlot(SiteNeedingBuilders))
		{
			TargetConstructionBuilding = SiteNeedingBuilders;

			FIntVector BuildCoord;
			if (FindBestRoadAccessCoord(SiteNeedingBuilders, BuildCoord))
			{
				ConstructionTaskPhase = EConstructionTaskPhase::MovingToBuild;
				if (MoveToGridCoord(BuildCoord))
				{
					return true;
				}
			}
			CancelConstructionAssignment(TEXT("Không tìm được đường tới móng sau khi giữ builder slot"));
		}
	}

	return false;
}

bool ABeaverAgent::FindBestRoadAccessCoord(const ATimberBuildingBase* Building, FIntVector& OutAccessCoord) const
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid || !IsValid(Building))
	{
		return false;
	}

	// Dùng đúng một access point logic: ưu tiên tuyệt đối Cửa.
	// Chỉ khi Cửa không có DirtPath mới xét chu vi để cứu map/asset cũ.
	TArray<FIntVector> BaseCandidates;
	const FIntVector DoorCoord = Building->GetDoorGridCoord();
	if (Grid->HasPathAt(DoorCoord) || Grid->HasPathAt(FIntVector(DoorCoord.X, DoorCoord.Y, DoorCoord.Z + 1)))
	{
		BaseCandidates.Add(DoorCoord);
	}
	else
	{
		BaseCandidates = Building->GetPerimeterAdjacentCoords();
	}

	int32 BestPathLength = MAX_int32;
	bool bFound = false;
	for (const FIntVector& BaseCoord : BaseCandidates)
	{
		for (int32 ZOffset = 0; ZOffset <= 1; ++ZOffset)
		{
			const FIntVector Candidate(BaseCoord.X, BaseCoord.Y, BaseCoord.Z + ZOffset);
			if (!Grid->HasPathAt(Candidate))
			{
				continue;
			}

			TArray<FIntVector> CandidatePath;
			if (Grid->FindPath(CurrentGridCoord, Candidate, CandidatePath, true) && CandidatePath.Num() < BestPathLength)
			{
				BestPathLength = CandidatePath.Num();
				OutAccessCoord = Candidate;
				bFound = true;
			}
		}
	}

	return bFound;
}

bool ABeaverAgent::IsBuildingAccessCoord(const ATimberBuildingBase* Building, const FIntVector& Coord) const
{
	if (!IsValid(Building))
	{
		return false;
	}

	TArray<FIntVector> BaseCandidates = Building->GetPerimeterAdjacentCoords();
	BaseCandidates.Insert(Building->GetDoorGridCoord(), 0);
	return BaseCandidates.ContainsByPredicate([&Coord](const FIntVector& BaseCoord)
	{
		return BaseCoord.X == Coord.X && BaseCoord.Y == Coord.Y &&
			(Coord.Z == BaseCoord.Z || Coord.Z == BaseCoord.Z + 1);
	});
}

void ABeaverAgent::CancelConstructionAssignment(const FString& Reason)
{
	UE_LOG(LogTemp, Warning,
		TEXT("🛑 [JOB CANCEL] '%s' Reason='%s' Phase=%s Current=%s Target=%s Wood=%d Reserved=%d Source='%s' Site='%s'"),
		*BeaverName, *Reason, *GetConstructionTaskDebugString(), *CurrentGridCoord.ToString(),
		*TargetGridCoord.ToString(), CarriedWoodAmount, ReservedConstructionWoodAmount,
		ConstructionSourceBuilding.IsValid() ? *ConstructionSourceBuilding->BuildingName : TEXT("None"),
		TargetConstructionBuilding.IsValid() ? *TargetConstructionBuilding->BuildingName : TEXT("None"));

	if (TargetConstructionBuilding.IsValid() && ReservedConstructionWoodAmount > 0)
	{
		TargetConstructionBuilding->ReservedWoodDelivering = FMath::Max(
			0, TargetConstructionBuilding->ReservedWoodDelivering - ReservedConstructionWoodAmount);
	}

	if (TargetConstructionBuilding.IsValid() && ConstructionTaskPhase == EConstructionTaskPhase::MovingToBuild)
	{
		if (ATimberGridManager* Grid = GetGridManager())
		{
			if (UTimberConstructionManager* CM = Grid->GetConstructionManager())
			{
				CM->ReleaseReservedBuilderSlot(TargetConstructionBuilding.Get());
			}
		}
	}

	ReservedConstructionWoodAmount = 0;
	ConstructionTaskPhase = EConstructionTaskPhase::None;
	ConstructionSourceBuilding = nullptr;
	TargetConstructionBuilding = nullptr;
}

FString ABeaverAgent::GetConstructionTaskDebugString() const
{
	switch (ConstructionTaskPhase)
	{
	case EConstructionTaskPhase::MovingToSource: return TEXT("FetchWood");
	case EConstructionTaskPhase::MovingToSite:   return TEXT("DeliverWood");
	case EConstructionTaskPhase::MovingToBuild:  return TEXT("MoveToBuild");
	case EConstructionTaskPhase::Building:       return TEXT("Building");
	case EConstructionTaskPhase::None:
	default:                                     return TEXT("None");
	}
}

void ABeaverAgent::HandleConstructionMovementCompleted()
{
	UE_LOG(LogTemp, Warning,
		TEXT("📍 [JOB ARRIVAL] '%s' Phase=%s Current=%s Target=%s Match=%s"),
		*BeaverName, *GetConstructionTaskDebugString(), *CurrentGridCoord.ToString(),
		*TargetGridCoord.ToString(), CurrentGridCoord == TargetGridCoord ? TEXT("YES") : TEXT("NO"));

	ATimberGridManager* Grid = GetGridManager();
	if (!Grid || !TargetConstructionBuilding.IsValid() ||
		TargetConstructionBuilding->BuildingState != EBuildingState::UnderConstruction ||
		!TargetConstructionBuilding->bIsConnectedToDistrict)
	{
		CancelConstructionAssignment(TEXT("Grid/Target không hợp lệ, site hoàn thành hoặc mất kết nối District"));
		SetBeaverState(EBeaverState::Idle);
		return;
	}

	switch (ConstructionTaskPhase)
	{
	case EConstructionTaskPhase::MovingToSource:
	{
		ATimberBuildingBase* Source = ConstructionSourceBuilding.Get();
		if (!IsValid(Source) || !Source->IsStorageFacility() ||
			!IsBuildingAccessCoord(Source, CurrentGridCoord))
		{
			CancelConstructionAssignment(FString::Printf(
				TEXT("Không nhận diện đúng Kho nguồn tại Current=%s"), *CurrentGridCoord.ToString()));
			SetBeaverState(EBeaverState::Idle);
			return;
		}

		const int32 Requested = FMath::Min(ReservedConstructionWoodAmount, AttributeConfig.MaxWoodCarryCapacity);
		const int32 Taken = Source->WithdrawResource(Requested);
		if (Taken <= 0)
		{
			CancelConstructionAssignment(TEXT("Kho nguồn không còn đủ gỗ khi Agent tới nơi"));
			SetBeaverState(EBeaverState::Idle);
			return;
		}

		if (Taken < ReservedConstructionWoodAmount)
		{
			const int32 Unfulfilled = ReservedConstructionWoodAmount - Taken;
			TargetConstructionBuilding->ReservedWoodDelivering = FMath::Max(
				0, TargetConstructionBuilding->ReservedWoodDelivering - Unfulfilled);
			ReservedConstructionWoodAmount = Taken;
		}
		AddCarriedWood(Taken);

		UE_LOG(LogTemp, Warning, TEXT("🪵 [TAKE WOOD] Hải ly '%s' rút %d gỗ từ '%s', chuyển thẳng sang Deliver cho '%s'."),
			*BeaverName, Taken, *Source->BuildingName, *TargetConstructionBuilding->BuildingName);

		FIntVector SiteAccessCoord;
		if (FindBestRoadAccessCoord(TargetConstructionBuilding.Get(), SiteAccessCoord))
		{
			ConstructionTaskPhase = EConstructionTaskPhase::MovingToSite;
			if (MoveToGridCoord(SiteAccessCoord))
			{
				return;
			}
		}

		CancelConstructionAssignment(TEXT("Không tìm được A* từ Kho nguồn tới access point của móng"));
		SetBeaverState(EBeaverState::Idle);
		return;
	}

	case EConstructionTaskPhase::MovingToSite:
	{
		if (!IsBuildingAccessCoord(TargetConstructionBuilding.Get(), CurrentGridCoord))
		{
			CancelConstructionAssignment(FString::Printf(
				TEXT("Current=%s không khớp access point của móng"), *CurrentGridCoord.ToString()));
			SetBeaverState(EBeaverState::Idle);
			return;
		}

		const int32 Delivered = TargetConstructionBuilding->AddDeliveredWood(CarriedWoodAmount);
		if (Delivered > 0)
		{
			CarriedWoodAmount -= Delivered;
			ReservedConstructionWoodAmount = FMath::Max(0, ReservedConstructionWoodAmount - Delivered);
			UE_LOG(LogTemp, Warning, TEXT("🪵 [SUPPLY WOOD] Hải ly '%s' giao %d gỗ cho '%s' (%d/%d)."),
				*BeaverName, Delivered, *TargetConstructionBuilding->BuildingName,
				TargetConstructionBuilding->CurrentWoodDelivered, TargetConstructionBuilding->WoodCost);
		}

		ConstructionTaskPhase = EConstructionTaskPhase::None;
		ConstructionSourceBuilding = nullptr;
		TargetConstructionBuilding = nullptr;
		SetBeaverState(EBeaverState::Idle);
		return;
	}

	case EConstructionTaskPhase::MovingToBuild:
	{
		if (IsBuildingAccessCoord(TargetConstructionBuilding.Get(), CurrentGridCoord))
		{
			if (UTimberConstructionManager* CM = Grid->GetConstructionManager())
			{
				if (CM->AssignBuilderToSite(TargetConstructionBuilding.Get(), this))
				{
					ConstructionTaskPhase = EConstructionTaskPhase::Building;
					SetBeaverState(EBeaverState::Working);
					return;
				}
			}
		}

		CancelConstructionAssignment(TEXT("Không nhận được builder slot hoặc không đứng đúng access point"));
		SetBeaverState(EBeaverState::Idle);
		return;
	}

	case EConstructionTaskPhase::Building:
	case EConstructionTaskPhase::None:
	default:
		return;
	}
}
