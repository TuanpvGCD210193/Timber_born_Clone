// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Buildings/TimberLumberjackFlag.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"

ATimberLumberjackFlag::ATimberLumberjackFlag()
{
	BuildingName = TEXT("Lumberjack Flag");
	FootprintSize = FIntPoint(1, 1);
	DoorRelativeCoord = FIntVector(0, 0, 0);
	WoodCost = 3;
	BuildingState = EBuildingState::Ghost_Valid;

	WorkRadius = 10;
	MaxWorkers = 1;
	CurrentAssignedWorkers = 0;
	LocalWoodBufferCapacity = 5;
	CurrentLocalWood = 0;
}

bool ATimberLumberjackFlag::AssignWorker()
{
	if (CurrentAssignedWorkers < MaxWorkers)
	{
		CurrentAssignedWorkers++;
		UE_LOG(LogTemp, Log, TEXT("ATimberLumberjackFlag: Đã tiếp nhận 1 thợ đốn gỗ Hải ly (Công nhân: %d / %d)."),
			CurrentAssignedWorkers, MaxWorkers);
		return true;
	}
	return false;
}

bool ATimberLumberjackFlag::UnassignWorker()
{
	if (CurrentAssignedWorkers > 0)
	{
		CurrentAssignedWorkers--;
		UE_LOG(LogTemp, Log, TEXT("ATimberLumberjackFlag: Đã hủy 1 thợ đốn gỗ Hải ly (Công nhân: %d / %d)."),
			CurrentAssignedWorkers, MaxWorkers);
		return true;
	}
	return false;
}

bool ATimberLumberjackFlag::HasWorker() const
{
	return CurrentAssignedWorkers > 0;
}

int32 ATimberLumberjackFlag::AddHarvestedWood(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const int32 OldWood = CurrentLocalWood;
	CurrentLocalWood = FMath::Clamp(CurrentLocalWood + Amount, 0, LocalWoodBufferCapacity);
	const int32 Added = CurrentLocalWood - OldWood;

	UE_LOG(LogTemp, Log, TEXT("ATimberLumberjackFlag: Đã gom +%d Gỗ vào Flag (Tồn đệm: %d / %d Gỗ)."),
		Added, CurrentLocalWood, LocalWoodBufferCapacity);

	return Added;
}

int32 ATimberLumberjackFlag::TakeWoodFromBuffer(int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const int32 OldWood = CurrentLocalWood;
	CurrentLocalWood = FMath::Clamp(CurrentLocalWood - Amount, 0, LocalWoodBufferCapacity);
	const int32 Taken = OldWood - CurrentLocalWood;

	return Taken;
}

bool ATimberLumberjackFlag::FindNearestHarvestableTree(const ATimberGridManager* GridManager, FIntVector& OutTreeCoord) const
{
	if (!GridManager || !IsFullyBuilt())
	{
		return false;
	}

	const FIntVector FlagCoord = OriginGridCoord;
	float BestDistSq = MAX_flt;
	bool bFound = false;
	FIntVector ClosestTree = FIntVector::ZeroValue;

	const int32 MinX = FMath::Max(0, FlagCoord.X - WorkRadius);
	const int32 MaxX = FMath::Min(GridManager->GridSizeX - 1, FlagCoord.X + WorkRadius);
	const int32 MinY = FMath::Max(0, FlagCoord.Y - WorkRadius);
	const int32 MaxY = FMath::Min(GridManager->GridSizeY - 1, FlagCoord.Y + WorkRadius);

	for (int32 X = MinX; X <= MaxX; ++X)
	{
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			// Kiểm tra khoảng cách 2D trong bán kính hình tròn WorkRadius
			const float Dist2DSq = FMath::Square((float)(X - FlagCoord.X)) + FMath::Square((float)(Y - FlagCoord.Y));
			if (Dist2DSq > FMath::Square((float)WorkRadius))
			{
				continue;
			}

			// Quét các tầng Z từ mặt đất lên đỉnh
			for (int32 Z = 0; Z < GridManager->GridSizeZ; ++Z)
			{
				FTimberCell Cell;
				if (GridManager->GetCell(FIntVector(X, Y, Z), Cell))
				{
					if (Cell.BlockType == ETimberBlockType::TreeMature || Cell.TreeStage == ETreeGrowthStage::Mature)
					{
						const float TotalDistSq = Dist2DSq + FMath::Square((float)(Z - FlagCoord.Z));
						if (TotalDistSq < BestDistSq)
						{
							BestDistSq = TotalDistSq;
							ClosestTree = FIntVector(X, Y, Z);
							bFound = true;
						}
						break; // Mỗi cột (X, Y) chỉ có tối đa 1 cây
					}
				}
			}
		}
	}

	if (bFound)
	{
		OutTreeCoord = ClosestTree;
		UE_LOG(LogTemp, Log, TEXT("ATimberLumberjackFlag: Đã tìm thấy Cây Trưởng Thành gần nhất tại [%d, %d, %d] (Khoảng cách: %.1f ô)."),
			OutTreeCoord.X, OutTreeCoord.Y, OutTreeCoord.Z, FMath::Sqrt(BestDistSq));
	}

	return bFound;
}
