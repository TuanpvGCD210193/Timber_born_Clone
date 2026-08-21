// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Buildings/TimberLumberjackFlag.h"
#include "Timber_born_Clone/Public/Beavers/BeaverAgent.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "DrawDebugHelpers.h"

ATimberLumberjackFlag::ATimberLumberjackFlag()
{
	PrimaryActorTick.bCanEverTick = true;

	BuildingName = TEXT("Lumberjack Flag");
	BuildingDescription = TEXT("Employs a lumberjack that cuts trees in the surrounding area.");
	FootprintSize = FIntPoint(1, 1);
	DoorRelativeCoord = FIntVector(0, 0, 0);
	WoodCost = 3; // 3 Gỗ để xây trại đốn gỗ
	BuildingState = EBuildingState::UnderConstruction;

	WorkRadius = 10;
	MaxWorkers = 1;
	bIsWorkAreaVisible = false;
}

void ATimberLumberjackFlag::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Vẽ dải đường viền xanh bao quanh khu vực làm việc nếu đang được người chơi chọn
	if (bIsWorkAreaVisible)
	{
		DrawWorkAreaBounds();
	}
}

bool ATimberLumberjackFlag::AddWorker(ABeaverAgent* Beaver)
{
	if (!Beaver)
	{
		return false;
	}

	// CHẶN: Chỉ cho phép tuyển thợ khi công trình đã xây xong 100%
	if (BuildingState != EBuildingState::Completed)
	{
		UE_LOG(LogTemp, Warning, TEXT("🌲 [LUMBERJACK FLAG] Không thể tuyển thợ vì công trình '%s' chưa xây xong!"), *BuildingName);
		return false;
	}

	// Đảm bảo không vượt quá số lượng công nhân tối đa (MaxWorkers = 1)
	if (AssignedWorkerBeavers.Num() >= MaxWorkers)
	{
		return false;
	}

	// Xóa các con trỏ null/stale nếu có
	AssignedWorkerBeavers.RemoveAll([](const TObjectPtr<ABeaverAgent>& Ptr) { return !IsValid(Ptr); });

	if (!AssignedWorkerBeavers.Contains(Beaver))
	{
		AssignedWorkerBeavers.Add(Beaver);
		Beaver->AssignWorkplace(this);
		UE_LOG(LogTemp, Warning, TEXT("🌲 [LUMBERJACK FLAG] Đã tuyển Hải ly '%s' vào làm thợ đốn gỗ! (Tổng thợ: %d/%d)"),
			*Beaver->BeaverName, AssignedWorkerBeavers.Num(), MaxWorkers);
		return true;
	}

	return false;
}

bool ATimberLumberjackFlag::RemoveWorker(ABeaverAgent* Beaver)
{
	// Xóa các con trỏ null/stale
	AssignedWorkerBeavers.RemoveAll([](const TObjectPtr<ABeaverAgent>& Ptr) { return !IsValid(Ptr); });

	if (AssignedWorkerBeavers.Num() == 0)
	{
		return false;
	}

	if (Beaver)
	{
		if (AssignedWorkerBeavers.Remove(Beaver) > 0)
		{
			Beaver->ClearWorkplace();
			UE_LOG(LogTemp, Warning, TEXT("🌲 [LUMBERJACK FLAG] Đã cho Hải ly '%s' thôi việc!"), *Beaver->BeaverName);
			return true;
		}
	}
	else
	{
		// Nếu không truyền Beaver cụ thể -> Cho thôi việc Hải ly cuối cùng trong danh sách
		TObjectPtr<ABeaverAgent> LastWorker = AssignedWorkerBeavers.Pop();
		if (IsValid(LastWorker))
		{
			LastWorker->ClearWorkplace();
			UE_LOG(LogTemp, Warning, TEXT("🌲 [LUMBERJACK FLAG] Đã cho thôi việc 1 Hải ly! (Còn lại: %d/%d)"),
				AssignedWorkerBeavers.Num(), MaxWorkers);
			return true;
		}
	}

	return false;
}

void ATimberLumberjackFlag::SetWorkAreaVisible(bool bVisible)
{
	bIsWorkAreaVisible = bVisible;
}

bool ATimberLumberjackFlag::IsCoordInsideWorkRadius(const FIntVector& TargetCoord) const
{
	const int32 Dx = FMath::Abs(TargetCoord.X - OriginGridCoord.X);
	const int32 Dy = FMath::Abs(TargetCoord.Y - OriginGridCoord.Y);

	// Kiểm tra phạm vi Chebyshev / Manhattan bounding box trong bán kính WorkRadius
	return (Dx <= WorkRadius && Dy <= WorkRadius);
}

bool ATimberLumberjackFlag::FindNearestMatureTreeInWorkRadius(const FVector& FromLocation, FIntVector& OutTreeCoord) const
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return false;
	}

	const FIntVector FromCoord = Grid->WorldLocationToGridCoord(FromLocation);

	int32 BestDistance = MAX_int32;
	bool bFound = false;

	// Quét toàn bộ các ô trong bán kính hình vuông [-WorkRadius, +WorkRadius]
	for (int32 Dx = -WorkRadius; Dx <= WorkRadius; ++Dx)
	{
		for (int32 Dy = -WorkRadius; Dy <= WorkRadius; ++Dy)
		{
			const int32 CheckX = OriginGridCoord.X + Dx;
			const int32 CheckY = OriginGridCoord.Y + Dy;

			// Tìm ô đất đặc cao nhất tại cột (X, Y)
			FIntVector GroundCoord;
			if (Grid->GetTopSolidGridCoordAt(CheckX, CheckY, GroundCoord))
			{
				// Kiểm tra ô phía trên mặt đất có phải là Cây Trưởng Thành (TreeMature)
				const FIntVector TreeSpaceCoord = FIntVector(CheckX, CheckY, GroundCoord.Z + 1);

				if (Grid->IsValidGridCoord(TreeSpaceCoord))
				{
					FTimberCell Cell;
					if (Grid->GetCell(TreeSpaceCoord, Cell) && Cell.BlockType == ETimberBlockType::TreeMature)
					{
						// Tính khoảng cách có trọng số Manhattan tới vị trí Hải ly (ưu tiên cùng tầng Z trước)
						const int32 Dist = FMath::Abs(TreeSpaceCoord.X - FromCoord.X) +
						                   FMath::Abs(TreeSpaceCoord.Y - FromCoord.Y) +
						                   FMath::Abs(TreeSpaceCoord.Z - FromCoord.Z) * 2;

						if (Dist < BestDistance)
						{
							BestDistance = Dist;
							OutTreeCoord = TreeSpaceCoord;
							bFound = true;
						}
					}
				}
			}
		}
	}

	return bFound;
}

void ATimberLumberjackFlag::DrawWorkAreaBounds()
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 MinX = OriginGridCoord.X - WorkRadius;
	const int32 MaxX = OriginGridCoord.X + WorkRadius;
	const int32 MinY = OriginGridCoord.Y - WorkRadius;
	const int32 MaxY = OriginGridCoord.Y + WorkRadius;

	const FColor BoundColor = FColor(0, 255, 200); // Xanh ngọc phát sáng (Cyan)

	// Hàm lambda lấy tọa độ 3D bám sát theo độ cao gờ đất thực tế của ô
	auto GetGroundTopPos = [Grid](int32 X, int32 Y) -> FVector
	{
		FIntVector GroundCoord;
		if (Grid->GetTopSolidGridCoordAt(X, Y, GroundCoord))
		{
			return Grid->GridCoordToWorldLocation(GroundCoord, true) + FVector(0.0f, 0.0f, 52.0f);
		}
		return Grid->GridCoordToWorldLocation(FIntVector(X, Y, 0), true) + FVector(0.0f, 0.0f, 52.0f);
	};

	// Vẽ 4 cạnh của dải viền xanh bao quanh toàn bộ khu vực làm việc
	// Cạnh 1 & 2: Dọc theo trục X (MinY và MaxY)
	for (int32 X = MinX; X < MaxX; ++X)
	{
		DrawDebugLine(World, GetGroundTopPos(X, MinY), GetGroundTopPos(X + 1, MinY), BoundColor, false, 0.05f, 0, 3.5f);
		DrawDebugLine(World, GetGroundTopPos(X, MaxY), GetGroundTopPos(X + 1, MaxY), BoundColor, false, 0.05f, 0, 3.5f);
	}

	// Cạnh 3 & 4: Dọc theo trục Y (MinX và MaxX)
	for (int32 Y = MinY; Y < MaxY; ++Y)
	{
		DrawDebugLine(World, GetGroundTopPos(MinX, Y), GetGroundTopPos(MinX, Y + 1), BoundColor, false, 0.05f, 0, 3.5f);
		DrawDebugLine(World, GetGroundTopPos(MaxX, Y), GetGroundTopPos(MaxX, Y + 1), BoundColor, false, 0.05f, 0, 3.5f);
	}
}
