// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Buildings/TimberStorage.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"

ATimberStorage::ATimberStorage()
{
	BuildingName = TEXT("Wood Storage Pile");
	FootprintSize = FIntPoint(2, 2);
	DoorRelativeCoord = FIntVector(0, 1, 0);
	WoodCost = 10;
	BuildingState = EBuildingState::Ghost_Valid;

	MaxCapacity = 100;
	StoredWood = 0;
}

int32 ATimberStorage::DepositWood(int32 Amount)
{
	if (Amount <= 0 || !IsFullyBuilt())
	{
		return 0;
	}

	const int32 OldStored = StoredWood;
	StoredWood = FMath::Clamp(StoredWood + Amount, 0, MaxCapacity);
	const int32 Deposited = StoredWood - OldStored;

	UE_LOG(LogTemp, Log, TEXT("ATimberStorage: Nhập +%d Gỗ vào kho (Hiện có: %d / %d Gỗ)."),
		Deposited, StoredWood, MaxCapacity);

	return Deposited;
}

int32 ATimberStorage::WithdrawWood(int32 Amount)
{
	if (Amount <= 0 || !IsFullyBuilt())
	{
		return 0;
	}

	const int32 OldStored = StoredWood;
	StoredWood = FMath::Clamp(StoredWood - Amount, 0, MaxCapacity);
	const int32 Withdrawn = OldStored - StoredWood;

	UE_LOG(LogTemp, Log, TEXT("ATimberStorage: Xuất -%d Gỗ từ kho (Còn lại: %d / %d Gỗ)."),
		Withdrawn, StoredWood, MaxCapacity);

	return Withdrawn;
}

bool ATimberStorage::IsFull() const
{
	return StoredWood >= MaxCapacity;
}

bool ATimberStorage::IsEmpty() const
{
	return StoredWood <= 0;
}

int32 ATimberStorage::GetRemainingCapacity() const
{
	return FMath::Max(0, MaxCapacity - StoredWood);
}

float ATimberStorage::GetStorageRatio() const
{
	return (MaxCapacity > 0) ? ((float)StoredWood / (float)MaxCapacity) : 0.0f;
}
