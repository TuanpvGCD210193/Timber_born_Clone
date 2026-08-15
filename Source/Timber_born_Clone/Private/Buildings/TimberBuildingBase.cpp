// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Buildings/TimberBuildingBase.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "Components/StaticMeshComponent.h"

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
}

void ATimberBuildingBase::BeginPlay()
{
	Super::BeginPlay();
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
		// Hiện Mesh công trình với Material Hologram Xanh, ẩn giàn giáo, tắt va chạm
		BuildingMeshComponent->SetVisibility(true);
		ScaffoldMeshComponent->SetVisibility(false);
		BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (GhostValidMaterial)
		{
			BuildingMeshComponent->SetMaterial(0, GhostValidMaterial);
		}
		break;

	case EBuildingState::Ghost_Invalid:
		// Hiện Mesh công trình với Material Hologram Đỏ, ẩn giàn giáo, tắt va chạm
		BuildingMeshComponent->SetVisibility(true);
		ScaffoldMeshComponent->SetVisibility(false);
		BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (GhostInvalidMaterial)
		{
			BuildingMeshComponent->SetMaterial(0, GhostInvalidMaterial);
		}
		break;

	case EBuildingState::UnderConstruction:
		// Ẩn Mesh hoàn thiện, hiện giàn giáo móng xây dựng
		BuildingMeshComponent->SetVisibility(false);
		ScaffoldMeshComponent->SetVisibility(true);
		ScaffoldMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		if (ScaffoldMaterial)
		{
			ScaffoldMeshComponent->SetMaterial(0, ScaffoldMaterial);
		}
		break;

	case EBuildingState::Completed:
		// Hiện Mesh công trình hoàn thiện với Material chính thức, ẩn giàn giáo
		BuildingMeshComponent->SetVisibility(true);
		ScaffoldMeshComponent->SetVisibility(false);
		BuildingMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		if (FinishedMaterial)
		{
			BuildingMeshComponent->SetMaterial(0, FinishedMaterial);
		}
		break;
	}
}

bool ATimberBuildingBase::DeliverWood(int32 Amount)
{
	if (BuildingState != EBuildingState::UnderConstruction)
	{
		return false;
	}

	if (CurrentWoodDelivered >= WoodCost)
	{
		return false; // Đã đủ gỗ
	}

	const int32 OldWood = CurrentWoodDelivered;
	CurrentWoodDelivered = FMath::Clamp(CurrentWoodDelivered + Amount, 0, WoodCost);
	const int32 Delivered = CurrentWoodDelivered - OldWood;

	UE_LOG(LogTemp, Log, TEXT("ATimberBuildingBase [%s]: Đã tiếp nhận %d Gỗ (Hiện có: %d / %d Gỗ)."),
		*BuildingName, Delivered, CurrentWoodDelivered, WoodCost);

	return Delivered > 0;
}

void ATimberBuildingBase::AdvanceBuildProgress(float WorkDeltaTime)
{
	if (BuildingState != EBuildingState::UnderConstruction)
	{
		return;
	}

	// Bắt buộc phải tập kết đủ 100% gỗ mới được phép thi công gõ búa
	if (!HasRequiredWood())
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

FIntVector ATimberBuildingBase::GetDoorGridCoord() const
{
	return OriginGridCoord + DoorRelativeCoord;
}

FVector ATimberBuildingBase::GetDoorWorldLocation(const ATimberGridManager* GridManager) const
{
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
	Coords.Reserve(FootprintSize.X * FootprintSize.Y);

	for (int32 Dx = 0; Dx < FootprintSize.X; ++Dx)
	{
		for (int32 Dy = 0; Dy < FootprintSize.Y; ++Dy)
		{
			Coords.Add(FIntVector(OriginGridCoord.X + Dx, OriginGridCoord.Y + Dy, OriginGridCoord.Z));
		}
	}

	return Coords;
}

int32 ATimberBuildingBase::GetRemainingWoodNeeded() const
{
	return FMath::Max(0, WoodCost - CurrentWoodDelivered);
}

float ATimberBuildingBase::GetBuildProgressPercent() const
{
	return CurrentBuildProgress * 100.0f;
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
	CurrentBuildProgress = 0.0f;
	SetBuildingState(EBuildingState::UnderConstruction);
	UE_LOG(LogTemp, Warning, TEXT("[CALL-IN-EDITOR] [%s]: Đã reset móng công trình về 0%%."), *BuildingName);
}
