// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Beavers/BeaverAgent.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "Timber_born_Clone/Public/Buildings/TimberDistrictCenter.h"
#include "Timber_born_Clone/Public/Buildings/TimberLumberjackFlag.h"
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

	// Tự động căn chỉnh tọa độ Grid ban đầu theo vị trí Spawn
	if (ATimberGridManager* Grid = GetGridManager())
	{
		CurrentGridCoord = Grid->WorldLocationToGridCoord(GetActorLocation());
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

	// 3. KIỂM TRA ĐÃ CHẠM ĐÍCH Ô TIẾP THEO CHƯA (Khoảng cách < 8cm)
	if (FVector::Dist2D(NewLocation, TargetLocation) < 8.0f)
	{
		if (ATimberGridManager* Grid = GetGridManager())
		{
			CurrentGridCoord = Grid->WorldLocationToGridCoord(NewLocation);
		}

		CurrentWaypointIndex++;

		// Nếu vừa bước tới điểm cuối cùng của toàn bộ dải đường
		if (CurrentWaypointIndex >= CurrentPathWaypoints.Num())
		{
			CurrentPathWaypoints.Empty();
			CurrentWaypointIndex = 0;

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
			// Nếu đang vác gỗ trên lưng và vừa bước tới cửa kho
			else if (CarriedWoodAmount > 0)
			{
				if (ATimberGridManager* Grid = GetGridManager())
				{
					// Tìm công trình kho tại vị trí ô cửa vừa bước tới
					for (const TObjectPtr<ATimberBuildingBase>& BuildingPtr : Grid->RegisteredBuildings)
					{
						if (IsValid(BuildingPtr) && BuildingPtr->IsStorageFacility())
						{
							if (BuildingPtr->GetDoorGridCoord() == CurrentGridCoord)
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

				// Ẩn khúc gỗ trên lưng
				if (CarriedWoodMeshComponent)
				{
					CarriedWoodMeshComponent->SetVisibility(CarriedWoodAmount > 0);
				}

				TargetTreeCoord = FIntVector::ZeroValue;
				SetBeaverState(EBeaverState::Idle);

				// Tự động kích hoạt lại chu trình tìm cây tiếp theo
				if (CurrentProfession == EBeaverProfession::Lumberjack)
				{
					StartLumberjackWorkLoop();
				}
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
		// Nếu là thợ đốn gỗ đang rảnh rỗi -> Tự động tìm cây tiếp theo để chặt
		if (CurrentProfession == EBeaverProfession::Lumberjack && AssignedWorkplaceFlag.IsValid())
		{
			StartLumberjackWorkLoop();
		}
		break;

	case EBeaverState::MovingToTarget:
		// Đang di chuyển trong UpdateMovement()
		break;

	case EBeaverState::Working:
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

				// 2. Nhận +1 Khúc gỗ lên lưng
				CarriedWoodAmount = FMath::Max(1, WoodEarned);
				SetBeaverState(EBeaverState::CarryingResource);

				UE_LOG(LogTemp, Warning, TEXT("🪓 [CHOPPING] Hải ly '%s' đã đốn hạ thành công cây tại (%d, %d, %d)! (Thu được: %d gỗ)"),
					*BeaverName, TargetTreeCoord.X, TargetTreeCoord.Y, TargetTreeCoord.Z, CarriedWoodAmount);

				// 3. Tìm Kho gần nhất còn chỗ chứa để vác gỗ về nạp
				FIntVector StorageDoorCoord;
				ATimberBuildingBase* TargetStorage = nullptr;

				if (FindNearestAvailableStorage(StorageDoorCoord, TargetStorage))
				{
					if (MoveToGridCoord(StorageDoorCoord))
					{
						UE_LOG(LogTemp, Log, TEXT("📦 [HAULING] Hải ly '%s' đang vác gỗ về kho '%s' tại cửa (%d, %d, %d)!"),
							*BeaverName, *TargetStorage->BuildingName, StorageDoorCoord.X, StorageDoorCoord.Y, StorageDoorCoord.Z);
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("⚠️ [STORAGE FULL] Toàn bộ Kho và Nhà Chính đều đã đầy gỗ! Hải ly '%s' đứng chờ."), *BeaverName);
				}
			}
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
	if (!AssignedWorkplaceFlag.IsValid())
	{
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
		CarriedWoodMeshComponent->SetVisibility(CurrentState == EBeaverState::CarryingResource && CarriedWoodAmount > 0);
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
		const FString StateText = FString::Printf(TEXT("%s: %s"), *BeaverName, *GetStateDebugString(StateColor));
		DrawDebugString(World, BeaverHeadPos, StateText, nullptr, StateColor, 0.0f, true, 1.2f);
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
		if (IsValid(BuildingPtr) && BuildingPtr->IsStorageFacility())
		{
			// Kiểm tra kho còn dung lượng chứa không
			if (BuildingPtr->GetCurrentStoredAmount() < BuildingPtr->GetMaxStorageCapacity())
			{
				const FIntVector DoorCoord = BuildingPtr->GetDoorGridCoord();

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
