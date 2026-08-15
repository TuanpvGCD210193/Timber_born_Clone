// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Pathfinding/TimberAStar.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "Algo/Reverse.h"

float UTimberAStar::CalculateHeuristic(const FIntVector& A, const FIntVector& B)
{
	const float Dx = FMath::Abs(A.X - B.X);
	const float Dy = FMath::Abs(A.Y - B.Y);
	const float Dz = FMath::Abs(A.Z - B.Z);

	// Manhattan 2D + Trọng số chiều cao Z
	return (Dx + Dy) + Dz * 1.5f;
}

float UTimberAStar::CalculateStepCost(
	const ATimberGridManager* GridManager,
	const FIntVector& From,
	const FIntVector& To)
{
	if (!GridManager)
	{
		return 1.0f;
	}

	float BaseCost = 1.5f; // Mặc định đi bộ trên Cỏ/Đất tự nhiên (chậm hơn)

	// Ưu tiên chạy nhanh trên đường đất (DirtPath)
	if (GridManager->HasPathAt(To))
	{
		BaseCost = 1.0f; // Chạy nhanh trên đường
	}

	// Chi phí leo dốc hoặc bước xuống bậc (1 block)
	const int32 DeltaZ = FMath::Abs(To.Z - From.Z);
	if (DeltaZ > 0)
	{
		BaseCost += 0.3f * DeltaZ;
	}

	return BaseCost;
}

bool UTimberAStar::FindPath(
	const ATimberGridManager* GridManager,
	const FIntVector& StartCoord,
	const FIntVector& TargetCoord,
	TArray<FIntVector>& OutPath,
	bool bRequirePathAtTarget)
{
	OutPath.Empty();

	if (!GridManager)
	{
		return false;
	}

	if (!GridManager->IsValidGridCoord(StartCoord) || !GridManager->IsValidGridCoord(TargetCoord))
	{
		UE_LOG(LogTemp, Warning, TEXT("UTimberAStar: Tọa độ Start [%s] hoặc Target [%s] nằm ngoài phạm vi bản đồ!"), 
			*StartCoord.ToString(), *TargetCoord.ToString());
		return false;
	}

	// Nếu yêu cầu bắt buộc phải có đường nối tới công trình
	if (bRequirePathAtTarget && !GridManager->HasPathAt(TargetCoord))
	{
		UE_LOG(LogTemp, Warning, TEXT("UTimberAStar: Điểm đích [%s] không có đường đi kết nối!"), *TargetCoord.ToString());
		return false;
	}

	if (StartCoord == TargetCoord)
	{
		OutPath.Add(StartCoord);
		return true;
	}

	// Cấu trúc dữ liệu A*
	TArray<FAStarNode> OpenSet;
	TSet<FIntVector> ClosedSet;
	TMap<FIntVector, FIntVector> CameFrom;
	TMap<FIntVector, float> GScoreMap;

	// Khởi tạo điểm Start
	GScoreMap.Add(StartCoord, 0.0f);
	OpenSet.HeapPush(FAStarNode(StartCoord, 0.0f, CalculateHeuristic(StartCoord, TargetCoord)));

	const FIntVector CardinalOffsets[4] = {
		FIntVector(1, 0, 0),
		FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0),
		FIntVector(0, -1, 0)
	};

	while (OpenSet.Num() > 0)
	{
		FAStarNode CurrentNode;
		OpenSet.HeapPop(CurrentNode);
		const FIntVector CurrentCoord = CurrentNode.Coord;

		// Đã tìm thấy đích đến!
		if (CurrentCoord == TargetCoord)
		{
			FIntVector TraceCoord = TargetCoord;
			OutPath.Add(TraceCoord);

			while (CameFrom.Contains(TraceCoord))
			{
				TraceCoord = CameFrom[TraceCoord];
				OutPath.Add(TraceCoord);
			}

			Algo::Reverse(OutPath);
			return true;
		}

		ClosedSet.Add(CurrentCoord);

		// Duyệt 4 hướng lân cận
		for (const FIntVector& Offset : CardinalOffsets)
		{
			const int32 NeighborX = CurrentCoord.X + Offset.X;
			const int32 NeighborY = CurrentCoord.Y + Offset.Y;

			// Kiểm tra các nấc độ cao trong khoảng chênh lệch [-1, 0, +1] block (leo dốc <= 1 block)
			for (int32 DeltaZ = -1; DeltaZ <= 1; ++DeltaZ)
			{
				const int32 NeighborZ = CurrentCoord.Z + DeltaZ;
				const FIntVector NeighborCoord(NeighborX, NeighborY, NeighborZ);

				if (!GridManager->IsValidGridCoord(NeighborCoord) || ClosedSet.Contains(NeighborCoord))
				{
					continue;
				}

				// Kiểm tra ô lân cận có thể đứng được không (Walkable hoặc là Đường hoặc có mặt đất bên dưới)
				FTimberCell NeighborCell;
				if (!GridManager->GetCell(NeighborCoord, NeighborCell))
				{
					continue;
				}

				// Điều kiện bước được: Hoặc là Đường đi (DirtPath), hoặc là ô Walkable, hoặc ô trống có sàn bước
				bool bCanWalkOn = NeighborCell.bIsWalkable || (NeighborCell.BlockType == ETimberBlockType::DirtPath);
				
				// Nếu ô hiện tại là khoảng không nhưng ô bên dưới là đất cứng -> đứng được
				if (!bCanWalkOn && NeighborCell.BlockType == ETimberBlockType::None && NeighborZ > 0)
				{
					FTimberCell BelowCell;
					if (GridManager->GetCell(FIntVector(NeighborX, NeighborY, NeighborZ - 1), BelowCell))
					{
						bCanWalkOn = (BelowCell.BlockType == ETimberBlockType::Grass || BelowCell.BlockType == ETimberBlockType::Dirt || BelowCell.BlockType == ETimberBlockType::Cliff);
					}
				}

				if (!bCanWalkOn)
				{
					continue; // Không bước được
				}

				const float TentativeGScore = CurrentNode.GScore + CalculateStepCost(GridManager, CurrentCoord, NeighborCoord);
				const float* ExistingGScore = GScoreMap.Find(NeighborCoord);

				if (!ExistingGScore || TentativeGScore < *ExistingGScore)
				{
					CameFrom.Add(NeighborCoord, CurrentCoord);
					GScoreMap.Add(NeighborCoord, TentativeGScore);

					const float HScore = CalculateHeuristic(NeighborCoord, TargetCoord);
					OpenSet.HeapPush(FAStarNode(NeighborCoord, TentativeGScore, HScore));
				}
			}
		}
	}

	// Không tìm thấy đường đi
	UE_LOG(LogTemp, Warning, TEXT("UTimberAStar: Không tìm thấy đường đi từ [%s] tới [%s]!"), 
		*StartCoord.ToString(), *TargetCoord.ToString());
	return false;
}
