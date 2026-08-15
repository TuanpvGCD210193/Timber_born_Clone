// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Pathfinding/TimberPathGraph.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
UTimberPathGraph::UTimberPathGraph()
{
}

bool UTimberPathGraph::AddPathNode(const FIntVector& Coord, const ATimberGridManager* GridManager)
{
	if (Nodes.Contains(Coord))
	{
		return false; // Đã tồn tại nút đường này
	}

	FTimberPathNode NewNode(Coord);

	// 4 hướng lân cận chính (Đông, Tây, Nam, Bắc)
	const FIntVector CardinalOffsets[4] = {
		FIntVector(1, 0, 0),
		FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0),
		FIntVector(0, -1, 0)
	};

	for (const FIntVector& Offset : CardinalOffsets)
	{
		// Kiểm tra các độ cao chênh lệch trong khoảng [-1, 0, +1] block (leo dốc <= 1 block)
		for (int32 DeltaZ = -1; DeltaZ <= 1; ++DeltaZ)
		{
			const FIntVector NeighborCoord(Coord.X + Offset.X, Coord.Y + Offset.Y, Coord.Z + DeltaZ);

			if (Nodes.Contains(NeighborCoord))
			{
				// Kết nối 2 chiều (Bidirectional connection)
				NewNode.ConnectedNeighbors.AddUnique(NeighborCoord);
				Nodes[NeighborCoord].ConnectedNeighbors.AddUnique(Coord);
			}
		}
	}

	Nodes.Add(Coord, NewNode);
	UE_LOG(LogTemp, Log, TEXT("UTimberPathGraph: Đã thêm nút đường tại [%d, %d, %d] (Kết nối %d láng giềng)."), 
		Coord.X, Coord.Y, Coord.Z, NewNode.ConnectedNeighbors.Num());

	return true;
}

bool UTimberPathGraph::RemovePathNode(const FIntVector& Coord)
{
	if (!Nodes.Contains(Coord))
	{
		return false;
	}

	// Ngắt kết nối từ tất cả các láng giềng trỏ tới nút này
	const FTimberPathNode& NodeToRemove = Nodes[Coord];
	for (const FIntVector& NeighborCoord : NodeToRemove.ConnectedNeighbors)
	{
		if (FTimberPathNode* NeighborNode = Nodes.Find(NeighborCoord))
		{
			NeighborNode->ConnectedNeighbors.Remove(Coord);
		}
	}

	Nodes.Remove(Coord);
	UE_LOG(LogTemp, Log, TEXT("UTimberPathGraph: Đã xóa nút đường tại [%d, %d, %d]."), Coord.X, Coord.Y, Coord.Z);
	return true;
}

bool UTimberPathGraph::HasPathNode(const FIntVector& Coord) const
{
	return Nodes.Contains(Coord);
}

const FTimberPathNode* UTimberPathGraph::GetPathNode(const FIntVector& Coord) const
{
	return Nodes.Find(Coord);
}

TArray<FIntVector> UTimberPathGraph::GetConnectedNeighbors(const FIntVector& Coord) const
{
	if (const FTimberPathNode* Node = Nodes.Find(Coord))
	{
		return Node->ConnectedNeighbors;
	}
	return TArray<FIntVector>();
}

void UTimberPathGraph::ClearAllNodes()
{
	Nodes.Empty();
	UE_LOG(LogTemp, Log, TEXT("UTimberPathGraph: Đã xóa sạch toàn bộ đồ thị đường đi."));
}

void UTimberPathGraph::RebuildAllAdjacency(const ATimberGridManager* GridManager)
{
	const FIntVector CardinalOffsets[4] = {
		FIntVector(1, 0, 0),
		FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0),
		FIntVector(0, -1, 0)
	};

	// Xóa toàn bộ liên kết cũ
	for (auto& Pair : Nodes)
	{
		Pair.Value.ConnectedNeighbors.Empty();
	}

	// Tái thiết lập kết nối 2 chiều cho toàn bộ các nút
	for (auto& Pair : Nodes)
	{
		const FIntVector& Coord = Pair.Key;
		FTimberPathNode& CurrentNode = Pair.Value;

		for (const FIntVector& Offset : CardinalOffsets)
		{
			for (int32 DeltaZ = -1; DeltaZ <= 1; ++DeltaZ)
			{
				const FIntVector NeighborCoord(Coord.X + Offset.X, Coord.Y + Offset.Y, Coord.Z + DeltaZ);

				if (Nodes.Contains(NeighborCoord))
				{
					CurrentNode.ConnectedNeighbors.AddUnique(NeighborCoord);
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UTimberPathGraph: Đã tái xây dựng toàn bộ liên kết cho %d nút đường."), Nodes.Num());
}

bool UTimberPathGraph::IsReachable(const FIntVector& StartCoord, const FIntVector& TargetCoord, int32& OutDistance) const
{
	OutDistance = 0;

	if (!Nodes.Contains(StartCoord) || !Nodes.Contains(TargetCoord))
	{
		return false;
	}

	if (StartCoord == TargetCoord)
	{
		return true;
	}

	// BFS Queue & Visited Map with Distance
	TQueue<FIntVector> Queue;
	TMap<FIntVector, int32> Distances;

	Queue.Enqueue(StartCoord);
	Distances.Add(StartCoord, 0);

	while (!Queue.IsEmpty())
	{
		FIntVector Current;
		Queue.Dequeue(Current);

		const int32 CurrentDist = Distances[Current];

		if (Current == TargetCoord)
		{
			OutDistance = CurrentDist;
			return true;
		}

		if (const FTimberPathNode* CurrentNode = Nodes.Find(Current))
		{
			for (const FIntVector& NeighborCoord : CurrentNode->ConnectedNeighbors)
			{
				if (!Distances.Contains(NeighborCoord))
				{
					Distances.Add(NeighborCoord, CurrentDist + 1);
					Queue.Enqueue(NeighborCoord);
				}
			}
		}
	}

	return false;
}

TArray<FIntVector> UTimberPathGraph::GetAllReachableNodes(const FIntVector& RootCoord) const
{
	TArray<FIntVector> ReachableNodes;

	if (!Nodes.Contains(RootCoord))
	{
		return ReachableNodes;
	}

	TQueue<FIntVector> Queue;
	TSet<FIntVector> Visited;

	Queue.Enqueue(RootCoord);
	Visited.Add(RootCoord);

	while (!Queue.IsEmpty())
	{
		FIntVector Current;
		Queue.Dequeue(Current);
		ReachableNodes.Add(Current);

		if (const FTimberPathNode* CurrentNode = Nodes.Find(Current))
		{
			for (const FIntVector& NeighborCoord : CurrentNode->ConnectedNeighbors)
			{
				if (!Visited.Contains(NeighborCoord))
				{
					Visited.Add(NeighborCoord);
					Queue.Enqueue(NeighborCoord);
				}
			}
		}
	}

	return ReachableNodes;
}
