// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Buildings/TimberDistrictCenter.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"

ATimberDistrictCenter::ATimberDistrictCenter()
{
	BuildingName = TEXT("District Center");
	FootprintSize = FIntPoint(2, 2);
	DoorRelativeCoord = FIntVector(0, 1, 0);
	WoodCost = 0; // Công trình trung tâm được dựng sẵn ban đầu
	BuildingState = EBuildingState::Completed;
	bCanBeDemolished = false; // BẢO VỆ NHÀ CHÍNH: Tuyệt đối không cho phép phá hủy!

	MaxWoodStorage = 50;
	CurrentWoodStock = 20; // 20 gỗ khởi nghiệp
	MaxBeaverCapacity = 10;
	StartingBeaverCount = 3;
	MaxDistrictRangeSteps = 70;
}

int32 ATimberDistrictCenter::AddWood(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const int32 OldStock = CurrentWoodStock;
	CurrentWoodStock = FMath::Clamp(CurrentWoodStock + Amount, 0, MaxWoodStorage);
	const int32 Added = CurrentWoodStock - OldStock;

	UE_LOG(LogTemp, Log, TEXT("ATimberDistrictCenter: Đã nhập +%d Gỗ vào kho (Hiện có: %d / %d Gỗ)."),
		Added, CurrentWoodStock, MaxWoodStorage);

	return Added;
}

int32 ATimberDistrictCenter::RemoveWood(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const int32 OldStock = CurrentWoodStock;
	CurrentWoodStock = FMath::Clamp(CurrentWoodStock - Amount, 0, MaxWoodStorage);
	const int32 Taken = OldStock - CurrentWoodStock;

	UE_LOG(LogTemp, Log, TEXT("ATimberDistrictCenter: Đã xuất -%d Gỗ từ kho (Còn lại: %d / %d Gỗ)."),
		Taken, CurrentWoodStock, MaxWoodStorage);

	return Taken;
}

bool ATimberDistrictCenter::IsStorageFull() const
{
	return CurrentWoodStock >= MaxWoodStorage;
}

float ATimberDistrictCenter::GetStorageFillRatio() const
{
	return (MaxWoodStorage > 0) ? ((float)CurrentWoodStock / (float)MaxWoodStorage) : 0.0f;
}
