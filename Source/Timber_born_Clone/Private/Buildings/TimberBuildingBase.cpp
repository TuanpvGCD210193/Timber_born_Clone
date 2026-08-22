// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Buildings/TimberBuildingBase.h"
#include "Timber_born_Clone/Public/Buildings/TimberDistrictCenter.h"
#include "Timber_born_Clone/Public/Buildings/TimberConstructionManager.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

ATimberBuildingBase::ATimberBuildingBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Tạo Scene Root Component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Tạo Building Mesh Component (Mesh công trình hoàn thiện)
	BuildingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMeshComponent"));
	BuildingMeshComponent->SetupAttachment(RootComponent);
	BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BuildingMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	BuildingMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Tạo Scaffold Mesh Component (Mesh móng / giàn giáo khi đang xây dựng)
	ScaffoldMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScaffoldMeshComponent"));
	ScaffoldMeshComponent->SetupAttachment(RootComponent);
	ScaffoldMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScaffoldMeshComponent->SetVisibility(false);

	// Tạo Door Arrow Component (Mũi tên 3D chỉ hướng cửa)
	DoorArrowComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorArrowComponent"));
	DoorArrowComponent->SetupAttachment(RootComponent);
	DoorArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoorArrowComponent->SetVisibility(false);

	// Tạo Unconnected Icon Widget Component (Billboard icon cảnh báo trên đầu)
	UnconnectedIconWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("UnconnectedIconWidgetComponent"));
	UnconnectedIconWidgetComponent->SetupAttachment(RootComponent);
	UnconnectedIconWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	UnconnectedIconWidgetComponent->SetDrawAtDesiredSize(true);
	UnconnectedIconWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	UnconnectedIconWidgetComponent->SetVisibility(false);
}

void ATimberBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	// Nếu là bóng mờ Hologram xem trước -> Tuyệt đối không đăng ký vào GridManager
	if (bIsHologramPreview)
	{
		UpdateVisuals();
		return;
	}

	// Tự động tìm GridManager và đăng ký công trình nếu được kéo thả vào Level từ trước
	AActor* FoundGrid = UGameplayStatics::GetActorOfClass(GetWorld(), ATimberGridManager::StaticClass());
	if (ATimberGridManager* Grid = Cast<ATimberGridManager>(FoundGrid))
	{
		CachedGridManager = Grid;

		// Actor Location của building là TÂM footprint, còn OriginGridCoord là GÓC Min X/Y.
		// Luôn đồng bộ từ Transform để cả building đặt sẵn và building spawn runtime dùng một quy ước.
		int32 SizeX = FootprintSize.X;
		int32 SizeY = FootprintSize.Y;
		const int32 NormalizedYaw = (FMath::RoundToInt(GetActorRotation().Yaw) % 360 + 360) % 360;
		if (NormalizedYaw == 90 || NormalizedYaw == 270)
		{
			Swap(SizeX, SizeY);
		}

		const FVector FootprintMinWorld = GetActorLocation() - FVector(
			SizeX * Grid->CellSize * 0.5f,
			SizeY * Grid->CellSize * 0.5f,
			0.0f);
		OriginGridCoord = Grid->WorldLocationToGridCoord(FootprintMinWorld);

		UE_LOG(LogTemp, Warning,
			TEXT("[BUILDING ORIGIN] '%s' Actor=%s Footprint=%dx%d -> Origin=%s"),
			*BuildingName, *GetActorLocation().ToString(), SizeX, SizeY, *OriginGridCoord.ToString());
		Grid->RegisterBuilding(this);
	}

	UpdateVisuals();
}

void ATimberBuildingBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateVisuals();
}

#if WITH_EDITOR
void ATimberBuildingBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateVisuals();
}
#endif

void ATimberBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATimberBuildingBase::SetBuildingState(EBuildingState NewState)
{
	if (BuildingState == NewState)
	{
		return;
	}

	const EBuildingState OldState = BuildingState;
	BuildingState = NewState;

	// Cập nhật vào Hàng đợi Xây dựng
	if (ATimberGridManager* Grid = GetGridManager())
	{
		if (UTimberConstructionManager* CM = Grid->GetConstructionManager())
		{
			if (NewState == EBuildingState::UnderConstruction)
			{
				CM->RegisterConstructionSite(this);
			}
			else if (OldState == EBuildingState::UnderConstruction)
			{
				CM->UnregisterConstructionSite(this);
			}
		}

		// State công trình và trạng thái mạng đường phải đồng bộ trong cùng frame.
		Grid->UpdateAllBuildingsConnectionStatus();
	}

	UE_LOG(LogTemp, Log, TEXT("ATimberBuildingBase [%s]: Chuyển đổi trạng thái từ [%s] -> [%s]"),
		*BuildingName,
		*UEnum::GetValueAsString(OldState),
		*UEnum::GetValueAsString(NewState));

	UpdateVisuals();
}

void ATimberBuildingBase::UpdateVisuals()
{
	if (!BuildingMeshComponent || !ScaffoldMeshComponent)
	{
		return;
	}

	switch (BuildingState)
	{
	case EBuildingState::Ghost_Valid:
	case EBuildingState::Ghost_Invalid:
		// Hiện Mesh công trình với Material Hologram tương ứng (Xanh/Đỏ), ẩn giàn giáo, tắt va chạm
		BuildingMeshComponent->SetRelativeScale3D(FVector(FootprintSize.X, FootprintSize.Y, 1.0f));
		BuildingMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
		BuildingMeshComponent->SetVisibility(true);
		BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		ScaffoldMeshComponent->SetVisibility(false);
		ScaffoldMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (UnconnectedIconWidgetComponent)
		{
			UnconnectedIconWidgetComponent->SetVisibility(false);
		}
		if (DoorArrowComponent)
		{
			UpdateDoorArrowTransform();
			DoorArrowComponent->SetVisibility(true);
		}

		if (BuildingState == EBuildingState::Ghost_Valid && GhostValidMaterial)
		{
			BuildingMeshComponent->SetMaterial(0, GhostValidMaterial);
		}
		else if (BuildingState == EBuildingState::Ghost_Invalid && GhostInvalidMaterial)
		{
			BuildingMeshComponent->SetMaterial(0, GhostInvalidMaterial);
		}
		break;

	case EBuildingState::UnderConstruction:
		// Trạng thái móng giàn giáo: Tự động gán Mesh móng dẹp phẳng 20cm phủ kín trọn vẹn FootprintSize
		if (ScaffoldMeshComponent->GetStaticMesh() == nullptr && BuildingMeshComponent->GetStaticMesh() != nullptr)
		{
			ScaffoldMeshComponent->SetStaticMesh(BuildingMeshComponent->GetStaticMesh());
		}

		ScaffoldMeshComponent->SetRelativeScale3D(FVector(FootprintSize.X, FootprintSize.Y, 0.2f));
		ScaffoldMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
		ScaffoldMeshComponent->SetVisibility(true);
		ScaffoldMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		if (ScaffoldMaterial)
		{
			ScaffoldMeshComponent->SetMaterial(0, ScaffoldMaterial);
		}

		BuildingMeshComponent->SetVisibility(false);
		BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		UpdateDistrictConnectionVisuals(bIsConnectedToDistrict);
		break;

	case EBuildingState::Completed:
		// Trạng thái hoàn thiện: Hiện Mesh công trình cao đầy đủ, ẩn móng giàn giáo
		BuildingMeshComponent->SetRelativeScale3D(FVector(FootprintSize.X, FootprintSize.Y, 1.0f));
		BuildingMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
		BuildingMeshComponent->SetVisibility(true);
		BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		if (FinishedMaterial)
		{
			BuildingMeshComponent->SetMaterial(0, FinishedMaterial);
		}

		ScaffoldMeshComponent->SetVisibility(false);
		ScaffoldMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		UpdateDistrictConnectionVisuals(bIsConnectedToDistrict);
		break;
	}
}

int32 ATimberBuildingBase::AddDeliveredWood(int32 Amount)
{
	if (Amount <= 0 || HasAllRequiredWood())
	{
		return 0; // Đã đủ gỗ
	}

	const int32 OldWood = CurrentWoodDelivered;
	CurrentWoodDelivered = FMath::Clamp(CurrentWoodDelivered + Amount, 0, WoodCost);
	const int32 Delivered = CurrentWoodDelivered - OldWood;

	// Giảm số lượng gỗ đang đặt chỗ (Reserved) tương ứng
	ReservedWoodDelivering = FMath::Max(0, ReservedWoodDelivering - Delivered);

	UE_LOG(LogTemp, Log, TEXT("ATimberBuildingBase [%s]: Đã tiếp nhận %d Gỗ (Hiện có: %d / %d Gỗ, Reserved còn lại: %d)."),
		*BuildingName, Delivered, CurrentWoodDelivered, WoodCost, ReservedWoodDelivering);

	return Delivered;
}

void ATimberBuildingBase::AdvanceBuildProgress(float WorkDeltaTime)
{
	if (BuildingState != EBuildingState::UnderConstruction)
	{
		return;
	}

	// Bắt buộc phải tập kết đủ 100% gỗ mới được phép thi công gõ búa
	if (!HasAllRequiredWood())
	{
		return;
	}

	const float Rate = (BuildTimeSeconds > 0.0f) ? (1.0f / BuildTimeSeconds) : 1.0f;
	CurrentBuildProgress = FMath::Clamp(CurrentBuildProgress + WorkDeltaTime * Rate, 0.0f, 1.0f);

	if (CurrentBuildProgress >= 1.0f)
	{
		SetBuildingState(EBuildingState::Completed);
		UE_LOG(LogTemp, Log, TEXT("ATimberBuildingBase [%s]: ĐÃ XÂY DỰNG HOÀN TẤT 100%%! Bắt đầu vận hành."), *BuildingName);
	}
}

ATimberGridManager* ATimberBuildingBase::GetGridManager() const
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

FIntVector ATimberBuildingBase::GetDoorGridCoord() const
{
	int32 SizeX = FootprintSize.X;
	int32 SizeY = FootprintSize.Y;

	const int32 YawAngle = FMath::RoundToInt(GetActorRotation().Yaw) % 360;
	const int32 NormalizedYaw = (YawAngle < 0) ? (YawAngle + 360) : YawAngle;

	if (NormalizedYaw == 90 || NormalizedYaw == 270)
	{
		SizeX = FootprintSize.Y;
		SizeY = FootprintSize.X;
	}

	// Xác định hướng mặt tiền theo góc xoay chuẩn của Unreal Engine:
	// Yaw = 0   -> Nhìn theo +X (Đông) -> Cửa nằm ở mặt tiền +X (Origin.X + SizeX, Origin.Y + DoorRel.Y)
	// Yaw = 90  -> Nhìn theo +Y (Bắc)  -> Cửa nằm ở mặt tiền +Y (Origin.X + DoorRel.X, Origin.Y + SizeY)
	// Yaw = 180 -> Nhìn theo -X (Tây)  -> Cửa nằm ở mặt tiền -X (Origin.X - 1, Origin.Y + DoorRel.Y)
	// Yaw = 270 -> Nhìn theo -Y (Nam)  -> Cửa nằm ở mặt tiền -Y (Origin.X + DoorRel.X, Origin.Y - 1)

	FIntVector DoorCoord = OriginGridCoord;

	if (NormalizedYaw == 90)
	{
		DoorCoord.X = OriginGridCoord.X + FMath::Clamp(DoorRelativeCoord.X, 0, SizeX - 1);
		DoorCoord.Y = OriginGridCoord.Y + SizeY;
	}
	else if (NormalizedYaw == 180)
	{
		DoorCoord.X = OriginGridCoord.X - 1;
		DoorCoord.Y = OriginGridCoord.Y + FMath::Clamp(DoorRelativeCoord.Y, 0, SizeY - 1);
	}
	else if (NormalizedYaw == 270)
	{
		DoorCoord.X = OriginGridCoord.X + FMath::Clamp(DoorRelativeCoord.X, 0, SizeX - 1);
		DoorCoord.Y = OriginGridCoord.Y - 1;
	}
	else // Yaw == 0 hoặc mặc định
	{
		DoorCoord.X = OriginGridCoord.X + SizeX;
		DoorCoord.Y = OriginGridCoord.Y + FMath::Clamp(DoorRelativeCoord.Y, 0, SizeY - 1);
	}

	DoorCoord.Z = OriginGridCoord.Z;
	return DoorCoord;
}

FVector ATimberBuildingBase::GetDoorWorldLocation(const ATimberGridManager* GridManager) const
{
	if (DoorArrowComponent)
	{
		return DoorArrowComponent->GetComponentLocation();
	}

	if (GridManager)
	{
		return GridManager->GridCoordToWorldLocation(GetDoorGridCoord(), true);
	}

	return GetActorLocation();
}

bool ATimberBuildingBase::HasRequiredWood() const
{
	return CurrentWoodDelivered >= WoodCost;
}

bool ATimberBuildingBase::IsFullyBuilt() const
{
	return BuildingState == EBuildingState::Completed;
}

TArray<FIntVector> ATimberBuildingBase::GetOccupiedGridCoords() const
{
	TArray<FIntVector> Coords;

	int32 SizeX = FootprintSize.X;
	int32 SizeY = FootprintSize.Y;

	// Tính toán chiều dài/chiều rộng thực tế theo góc xoay Yaw của Actor
	const int32 YawAngle = FMath::RoundToInt(GetActorRotation().Yaw) % 360;
	const int32 NormalizedYaw = (YawAngle < 0) ? (YawAngle + 360) : YawAngle;

	if (NormalizedYaw == 90 || NormalizedYaw == 270)
	{
		SizeX = FootprintSize.Y;
		SizeY = FootprintSize.X;
	}

	Coords.Reserve(SizeX * SizeY);

	for (int32 Dx = 0; Dx < SizeX; ++Dx)
	{
		for (int32 Dy = 0; Dy < SizeY; ++Dy)
		{
			Coords.Add(FIntVector(OriginGridCoord.X + Dx, OriginGridCoord.Y + Dy, OriginGridCoord.Z));
		}
	}

	return Coords;
}

TArray<FIntVector> ATimberBuildingBase::GetPerimeterAdjacentCoords() const
{
	TArray<FIntVector> PerimeterCoords;
	const int32 BaseZ = OriginGridCoord.Z;

	int32 SizeX = FootprintSize.X;
	int32 SizeY = FootprintSize.Y;

	const int32 YawAngle = FMath::RoundToInt(GetActorRotation().Yaw) % 360;
	const int32 NormalizedYaw = (YawAngle < 0) ? (YawAngle + 360) : YawAngle;

	if (NormalizedYaw == 90 || NormalizedYaw == 270)
	{
		SizeX = FootprintSize.Y;
		SizeY = FootprintSize.X;
	}

	// Cạnh Nam (Y = MinY - 1) và Cạnh Bắc (Y = MaxY + 1)
	for (int32 Dx = 0; Dx < SizeX; ++Dx)
	{
		const int32 CurrentX = OriginGridCoord.X + Dx;
		PerimeterCoords.Add(FIntVector(CurrentX, OriginGridCoord.Y - 1, BaseZ));
		PerimeterCoords.Add(FIntVector(CurrentX, OriginGridCoord.Y + SizeY, BaseZ));
	}

	// Cạnh Tây (X = MinX - 1) và Cạnh Đông (X = MaxX + 1)
	for (int32 Dy = 0; Dy < SizeY; ++Dy)
	{
		const int32 CurrentY = OriginGridCoord.Y + Dy;
		PerimeterCoords.Add(FIntVector(OriginGridCoord.X - 1, CurrentY, BaseZ));
		PerimeterCoords.Add(FIntVector(OriginGridCoord.X + SizeX, CurrentY, BaseZ));
	}

	return PerimeterCoords;
}

float ATimberBuildingBase::GetBuildProgressPercent() const
{
	return CurrentBuildProgress * 100.0f;
}

bool ATimberBuildingBase::HasPathAtDoor() const
{
	if (!CachedGridManager.IsValid())
	{
		if (const UWorld* World = GetWorld())
		{
			AActor* FoundGrid = UGameplayStatics::GetActorOfClass(World, ATimberGridManager::StaticClass());
			CachedGridManager = Cast<ATimberGridManager>(FoundGrid);
		}
	}

	if (CachedGridManager.IsValid())
	{
		const FIntVector DoorCoord = GetDoorGridCoord();
		// Kiểm tra cả tầng Z của móng và tầng Z+1 (độ cao của DirtPath)
		if (CachedGridManager->HasPathAt(DoorCoord) || CachedGridManager->HasPathAt(FIntVector(DoorCoord.X, DoorCoord.Y, DoorCoord.Z + 1)))
		{
			return true;
		}
	}

	return false;
}

void ATimberBuildingBase::SetDoorArrowVisible(bool bVisible)
{
	if (!DoorArrowComponent)
	{
		return;
	}

	// 1. Nếu công trình đang ở dạng Hologram xem trước -> Tuyệt đối ẨN
	if (BuildingState == EBuildingState::Ghost_Valid || BuildingState == EBuildingState::Ghost_Invalid)
	{
		DoorArrowComponent->SetVisibility(false);
		return;
	}

	// 2. Nếu là Nhà Chính (District Center) -> Luôn hiện Mũi Tên khi đang bật chế độ Lát Đường (bVisible)
	if (IsA(ATimberDistrictCenter::StaticClass()))
	{
		if (bVisible)
		{
			UpdateDoorArrowTransform();
			DoorArrowComponent->SetVisibility(true);
		}
		else
		{
			DoorArrowComponent->SetVisibility(false);
		}
		return;
	}

	// 3. Nếu đã có đường đè lên ô cửa HOẶC công trình đã kết nối hoàn toàn về Nhà Chính -> ẨN
	if (HasPathAtDoor() || bIsConnectedToDistrict)
	{
		DoorArrowComponent->SetVisibility(false);
		return;
	}

	// 4. Nếu ở chế độ Lát Đường (bVisible == true) và chưa bị đường đè -> HIỆN MŨI TÊN
	if (bVisible)
	{
		// Căn chỉnh vị trí Mũi tên 3D nằm chính xác tại ô đường ngay phía trước cửa (bên ngoài móng)
		UpdateDoorArrowTransform();
		DoorArrowComponent->SetVisibility(true);
	}
	else
	{
		DoorArrowComponent->SetVisibility(false);
	}
}

FVector ATimberBuildingBase::CalcDoorArrowLocalOffset() const
{
	// Component dùng local space nên KHÔNG tráo X/Y theo Actor Yaw.
	// Actor rotation tự xoay toàn bộ mặt tiền local +X sang hướng thế giới tương ứng.
	const int32 SizeX = FootprintSize.X;
	const int32 SizeY = FootprintSize.Y;

	// Đẩy mũi tên ra mặt tiền local +X đúng 1 ô.
	const float LocalX = (SizeX * 0.5f) * 100.0f + 50.0f;
	const float LocalY = (DoorRelativeCoord.Y - (SizeY - 1) * 0.5f) * 100.0f;
	const float LocalZ = 15.0f;

	return FVector(LocalX, LocalY, LocalZ);
}

void ATimberBuildingBase::UpdateDoorArrowTransform()
{
	if (!DoorArrowComponent)
	{
		return;
	}

	DoorArrowComponent->SetRelativeLocation(CalcDoorArrowLocalOffset());
	DoorArrowComponent->SetRelativeRotation(FRotator(0.0f, DoorArrowMeshYawOffset, 0.0f));
}

void ATimberBuildingBase::UpdateDistrictConnectionVisuals(bool bConnected)
{
	bIsConnectedToDistrict = bConnected;

	// District Center là gốc nguồn nên KHÔNG BAO GIỜ hiện icon No Road
	if (IsA(ATimberDistrictCenter::StaticClass()))
	{
		bIsConnectedToDistrict = true;
		if (UnconnectedIconWidgetComponent)
		{
			UnconnectedIconWidgetComponent->SetVisibility(false);
		}
		if (DoorArrowComponent)
		{
			DoorArrowComponent->SetVisibility(false);
		}
		return;
	}

	if (UnconnectedIconWidgetComponent)
	{
		// Nếu đang ở trạng thái Hologram xem trước (chưa đặt móng) thì TUYỆT ĐỐI KHÔNG HIỆN
		if (BuildingState == EBuildingState::Ghost_Valid || BuildingState == EBuildingState::Ghost_Invalid)
		{
			UnconnectedIconWidgetComponent->SetVisibility(false);
		}
		else
		{
			// Nếu đã kết nối đường về District Center thì ẩn, nếu đứt đường thì hiện
			UnconnectedIconWidgetComponent->SetVisibility(!bConnected);
		}
	}

	// Nếu đã kết nối đường thành công HOẶC ô cửa đã có đường đè lên -> tự động tắt Mũi tên chỉ hướng
	if ((bConnected || HasPathAtDoor()) && DoorArrowComponent)
	{
		DoorArrowComponent->SetVisibility(false);
	}
}

void ATimberBuildingBase::Editor_DeliverWoodStep()
{
	if (BuildingState != EBuildingState::UnderConstruction && BuildingState != EBuildingState::Completed)
	{
		SetBuildingState(EBuildingState::UnderConstruction);
	}

	DeliverWood(2);
	UE_LOG(LogTemp, Warning, TEXT("[CALL-IN-EDITOR] [%s]: Giao 2 Gỗ. Tiến độ vật liệu: %d / %d Gỗ (Còn thiếu: %d Gỗ)."),
		*BuildingName, CurrentWoodDelivered, WoodCost, GetRemainingWoodNeeded());
}

void ATimberBuildingBase::Editor_AdvanceBuildStep()
{
	if (BuildingState != EBuildingState::UnderConstruction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CALL-IN-EDITOR] [%s]: Công trình chưa ở trạng thái Đang xây dựng!"), *BuildingName);
		return;
	}

	if (!HasRequiredWood())
	{
		UE_LOG(LogTemp, Error, TEXT("[CALL-IN-EDITOR] [%s]: Chưa thể thi công! Cần thêm %d Gỗ trước khi thợ gõ búa."),
			*BuildingName, GetRemainingWoodNeeded());
		return;
	}

	AdvanceBuildProgress(2.5f);
	UE_LOG(LogTemp, Warning, TEXT("[CALL-IN-EDITOR] [%s]: Thợ gõ búa thi công 2.5s. Tiến độ hiện tại: %.1f%%."),
		*BuildingName, GetBuildProgressPercent());
}

void ATimberBuildingBase::Editor_InstantComplete()
{
	CurrentWoodDelivered = WoodCost;
	CurrentBuildProgress = 1.0f;
	SetBuildingState(EBuildingState::Completed);
	UE_LOG(LogTemp, Warning, TEXT("[CALL-IN-EDITOR] [%s]: HOÀN THÀNH TỨC THÌ 100%%!"), *BuildingName);
}

void ATimberBuildingBase::Editor_ResetConstruction()
{
	CurrentWoodDelivered = 0;
	ReservedWoodDelivering = 0;
	CurrentActiveBuilders = 0;
	ReservedBuildersEnRoute = 0;
	CurrentBuildProgress = 0.0f;
	SetBuildingState(EBuildingState::UnderConstruction);
	UE_LOG(LogTemp, Warning, TEXT("[CALL-IN-EDITOR] [%s]: Đã reset móng công trình về 0%%."), *BuildingName);
}
