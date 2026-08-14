// Copyright Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "DrawDebugHelpers.h"

ATimberGridManager::ATimberGridManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// Tạo Root Component để Actor có thể di chuyển và xoay trong Level
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ATimberGridManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ATimberGridManager::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitializeOnBeginPlay && GridCells.Num() == 0)
	{
		InitializeGrid();
		RebuildISMComponents();
	}
}

void ATimberGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FVector ATimberGridManager::GridCoordToWorldLocation(const FIntVector& Coord, bool bCenterOffset) const
{
	const FVector ActorOrigin = GetActorLocation();
	const FVector Offset = bCenterOffset 
		? FVector(CellSize * 0.5f, CellSize * 0.5f, CellSize * 0.5f) 
		: FVector::ZeroVector;

	return ActorOrigin + FVector(Coord.X * CellSize, Coord.Y * CellSize, Coord.Z * CellSize) + Offset;
}

FIntVector ATimberGridManager::WorldLocationToGridCoord(const FVector& WorldLocation) const
{
	const FVector RelativePos = WorldLocation - GetActorLocation();

	const int32 X = FMath::FloorToInt(RelativePos.X / CellSize);
	const int32 Y = FMath::FloorToInt(RelativePos.Y / CellSize);
	const int32 Z = FMath::FloorToInt(RelativePos.Z / CellSize);

	return FIntVector(X, Y, Z);
}

bool ATimberGridManager::GetCell(const FIntVector& Coord, FTimberCell& OutCell) const
{
	if (!IsValidGridCoord(Coord))
	{
		return false;
	}

	const int32 Index = GetCellIndexFromVector(Coord);
	if (GridCells.IsValidIndex(Index))
	{
		OutCell = GridCells[Index];
		return true;
	}

	return false;
}

bool ATimberGridManager::SetCellType(const FIntVector& Coord, ETimberBlockType NewType, bool bWalkable)
{
	if (!IsValidGridCoord(Coord))
	{
		return false;
	}

	const int32 Index = GetCellIndexFromVector(Coord);
	if (GridCells.IsValidIndex(Index))
	{
		GridCells[Index].BlockType = NewType;
		GridCells[Index].bIsWalkable = bWalkable;
		return true;
	}

	return false;
}

void ATimberGridManager::InitializeGrid()
{
	const int32 TotalCells = GridSizeX * GridSizeY * GridSizeZ;
	GridCells.Empty(TotalCells);
	GridCells.Reserve(TotalCells);

	for (int32 Z = 0; Z < GridSizeZ; ++Z)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			for (int32 X = 0; X < GridSizeX; ++X)
			{
				FTimberCell NewCell(FIntVector(X, Y, Z), ETimberBlockType::None, false);
				GridCells.Add(NewCell);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ATimberGridManager: Đã khởi tạo thành công %d ô lưới (Kích thước: %dx%dx%d, CellSize: %.1fcm)"), 
		TotalCells, GridSizeX, GridSizeY, GridSizeZ, CellSize);
}

void ATimberGridManager::RebuildISMComponents()
{
	for (const auto& Pair : BlockMeshConfigs)
	{
		const ETimberBlockType BlockType = Pair.Key;
		const FTimberBlockMeshConfig& Config = Pair.Value;

		if (!Config.StaticMesh)
		{
			continue;
		}

		UInstancedStaticMeshComponent* ISM = GetOrCreateISMComponent(BlockType);
		if (ISM)
		{
			ISM->SetStaticMesh(Config.StaticMesh);
			if (Config.OverrideMaterial)
			{
				ISM->SetMaterial(0, Config.OverrideMaterial);
			}
		}
	}
}

UInstancedStaticMeshComponent* ATimberGridManager::GetOrCreateISMComponent(ETimberBlockType BlockType)
{
	if (BlockType == ETimberBlockType::None)
	{
		return nullptr;
	}

	if (TObjectPtr<UInstancedStaticMeshComponent>* Existing = BlockISMMap.Find(BlockType))
	{
		if (*Existing && IsValid(*Existing))
		{
			return *Existing;
		}
	}

	// Tạo mới ISM Component với cờ tối ưu hiệu năng triệt để
	const FString CompName = FString::Printf(TEXT("ISM_%s"), *UEnum::GetValueAsString(BlockType));
	UInstancedStaticMeshComponent* NewISM = NewObject<UInstancedStaticMeshComponent>(this, *CompName);
	if (!NewISM)
	{
		return nullptr;
	}

	NewISM->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	
	// TỐI ƯU HÓA HIỆU NĂNG:
	NewISM->SetCanEverAffectNavigation(false); // Không tốn CPU rebuild NavMesh thừa
	NewISM->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Chỉ dùng Raycast click chuột, tắt Chaos Physics
	NewISM->SetCollisionResponseToAllChannels(ECR_Ignore);
	NewISM->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	NewISM->bCastDynamicShadow = true;
	NewISM->bAffectDynamicIndirectLighting = true;

	NewISM->RegisterComponent();

	if (const FTimberBlockMeshConfig* Config = BlockMeshConfigs.Find(BlockType))
	{
		if (Config->StaticMesh)
		{
			NewISM->SetStaticMesh(Config->StaticMesh);
			if (Config->OverrideMaterial)
			{
				NewISM->SetMaterial(0, Config->OverrideMaterial);
			}
		}
	}

	BlockISMMap.Add(BlockType, NewISM);
	return NewISM;
}

bool ATimberGridManager::SetBlock(const FIntVector& Coord, ETimberBlockType NewType, bool bWalkable)
{
	if (!IsValidGridCoord(Coord))
	{
		return false;
	}

	const int32 CellIdx = GetCellIndexFromVector(Coord);
	if (!GridCells.IsValidIndex(CellIdx))
	{
		return false;
	}

	// Nếu ô cũ đã có Mesh, xóa instance cũ trước
	if (GridCells[CellIdx].BlockType != ETimberBlockType::None && GridCells[CellIdx].InstanceIndex != INDEX_NONE)
	{
		ClearBlock(Coord);
	}

	if (NewType == ETimberBlockType::None)
	{
		GridCells[CellIdx].BlockType = ETimberBlockType::None;
		GridCells[CellIdx].bIsWalkable = false;
		GridCells[CellIdx].InstanceIndex = INDEX_NONE;
		return true;
	}

	UInstancedStaticMeshComponent* ISM = GetOrCreateISMComponent(NewType);
	int32 NewInstIdx = INDEX_NONE;

	if (ISM && ISM->GetStaticMesh())
	{
		FVector MeshOffset = FVector::ZeroVector;
		FVector MeshScale = FVector(1.0f);

		if (const FTimberBlockMeshConfig* Config = BlockMeshConfigs.Find(NewType))
		{
			MeshOffset = Config->MeshOffset;
			MeshScale = Config->MeshScale;
		}

		// Tính toán vị trí tâm ô
		const FVector WorldLocation = GridCoordToWorldLocation(Coord, true) + MeshOffset;
		const FTransform InstanceTransform(FRotator::ZeroRotator, WorldLocation, MeshScale);

		NewInstIdx = ISM->AddInstance(InstanceTransform, true);
	}

	GridCells[CellIdx].BlockType = NewType;
	GridCells[CellIdx].bIsWalkable = bWalkable;
	GridCells[CellIdx].InstanceIndex = NewInstIdx;

	return true;
}

bool ATimberGridManager::ClearBlock(const FIntVector& Coord)
{
	if (!IsValidGridCoord(Coord))
	{
		return false;
	}

	const int32 CellIdx = GetCellIndexFromVector(Coord);
	if (!GridCells.IsValidIndex(CellIdx))
	{
		return false;
	}

	const ETimberBlockType OldType = GridCells[CellIdx].BlockType;
	const int32 OldInstIdx = GridCells[CellIdx].InstanceIndex;

	if (OldType != ETimberBlockType::None && OldInstIdx != INDEX_NONE)
	{
		if (TObjectPtr<UInstancedStaticMeshComponent>* ISMPtr = BlockISMMap.Find(OldType))
		{
			UInstancedStaticMeshComponent* ISM = *ISMPtr;
			if (ISM && ISM->IsValidInstance(OldInstIdx))
			{
				const int32 LastIndex = ISM->GetInstanceCount() - 1;
				ISM->RemoveInstance(OldInstIdx);

				// ISM RemoveInstance thực hiện Swap với phần tử cuối cùng!
				// Cần cập nhật lại InstanceIndex của ô đang giữ LastIndex
				if (OldInstIdx != LastIndex)
				{
					for (FTimberCell& Cell : GridCells)
					{
						if (Cell.BlockType == OldType && Cell.InstanceIndex == LastIndex)
						{
							Cell.InstanceIndex = OldInstIdx;
							break;
						}
					}
				}
			}
		}
	}

	GridCells[CellIdx].BlockType = ETimberBlockType::None;
	GridCells[CellIdx].bIsWalkable = false;
	GridCells[CellIdx].InstanceIndex = INDEX_NONE;
	GridCells[CellIdx].TreeStage = ETreeGrowthStage::None;
	GridCells[CellIdx].TreeGrowthTimer = 0.0f;

	return true;
}

void ATimberGridManager::ClearAllInstances()
{
	// 1. Quét sạch 100% mọi UInstancedStaticMeshComponent đang bám trên Actor
	TArray<UInstancedStaticMeshComponent*> AllAttachedISMs;
	GetComponents<UInstancedStaticMeshComponent>(AllAttachedISMs);

	for (UInstancedStaticMeshComponent* ISM : AllAttachedISMs)
	{
		if (IsValid(ISM))
		{
			ISM->ClearInstances();
			ISM->DestroyComponent();
		}
	}

	// 2. Làm sạch từ điển tham chiếu
	BlockISMMap.Empty();

	// 3. Reset toàn bộ dữ liệu ô lưới
	for (FTimberCell& Cell : GridCells)
	{
		Cell.BlockType = ETimberBlockType::None;
		Cell.bIsWalkable = false;
		Cell.InstanceIndex = INDEX_NONE;
		Cell.TreeStage = ETreeGrowthStage::None;
		Cell.TreeGrowthTimer = 0.0f;
	}

	UE_LOG(LogTemp, Log, TEXT("ATimberGridManager: Đã quét và xóa sạch 100%% (%d) ISM Components trên Actor."), AllAttachedISMs.Num());
}

void ATimberGridManager::GenerateNaturalTerrainAndForests()
{
	InitializeGrid();
	ClearAllInstances();
	RebuildISMComponents();

	const int32 BaseH = FMath::Clamp(TerrainConfig.BaseHeight, 1, GridSizeZ - 2);

	// ========================================================
	// 1. TÍNH TOÁN ĐỘ CAO SOLID HEIGHTMAP (KHÔNG BAO GIỜ BỊ LỖ HỔNG)
	// ========================================================
	TArray<int32> TopHeightMap;
	TopHeightMap.Init(BaseH, GridSizeX * GridSizeY);

	for (int32 Hill = 0; Hill < TerrainConfig.HillCount; ++Hill)
	{
		const int32 Margin = TerrainConfig.HillRadius + 1;
		const int32 CenterX = FMath::RandRange(Margin, FMath::Max(Margin, GridSizeX - Margin));
		const int32 CenterY = FMath::RandRange(Margin, FMath::Max(Margin, GridSizeY - Margin));

		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			for (int32 X = 0; X < GridSizeX; ++X)
			{
				const float Dist = FVector2D::Distance(FVector2D(X, Y), FVector2D(CenterX, CenterY));
				if (Dist <= TerrainConfig.HillRadius)
				{
					const float HeightFactor = 1.0f - (Dist / TerrainConfig.HillRadius);
					const int32 ExtraHeight = FMath::Clamp(FMath::RoundToInt(HeightFactor * TerrainConfig.HillHeight), 1, TerrainConfig.HillHeight);
					const int32 MapIdx = X + Y * GridSizeX;
					TopHeightMap[MapIdx] = FMath::Max(TopHeightMap[MapIdx], FMath::Min(BaseH + ExtraHeight, GridSizeZ - 2));
				}
			}
		}
	}

	// ========================================================
	// 2. ĐIỀN ĐẦY ĐẶC DỮ LIỆU GRIDCELLS (SUB: DIRT, WALL: CLIFF, TOP: GRASS)
	// ========================================================
	for (int32 Y = 0; Y < GridSizeY; ++Y)
	{
		for (int32 X = 0; X < GridSizeX; ++X)
		{
			const int32 TopH = TopHeightMap[X + Y * GridSizeX];
			for (int32 Z = 0; Z < TopH; ++Z)
			{
				const int32 CellIdx = GetCellIndex(X, Y, Z);
				if (Z == TopH - 1)
				{
					GridCells[CellIdx].BlockType = ETimberBlockType::Grass;
					GridCells[CellIdx].bIsWalkable = true;
				}
				else
				{
					GridCells[CellIdx].BlockType = (Z >= BaseH - 1) ? ETimberBlockType::Cliff : ETimberBlockType::Dirt;
					GridCells[CellIdx].bIsWalkable = false;
				}
			}
		}
	}

	// ========================================================
	// 3. RẢI CÁC CỤM RỪNG HỮU CƠ TRÊN NỀN CỎ
	// ========================================================
	for (int32 Cluster = 0; Cluster < ForestConfig.ClusterCount; ++Cluster)
	{
		const int32 Margin = ForestConfig.ClusterRadius + 1;
		const int32 CenterX = FMath::RandRange(Margin, FMath::Max(Margin, GridSizeX - Margin));
		const int32 CenterY = FMath::RandRange(Margin, FMath::Max(Margin, GridSizeY - Margin));

		for (int32 OffY = -ForestConfig.ClusterRadius; OffY <= ForestConfig.ClusterRadius; ++OffY)
		{
			for (int32 OffX = -ForestConfig.ClusterRadius; OffX <= ForestConfig.ClusterRadius; ++OffX)
			{
				const int32 TargetX = CenterX + OffX;
				const int32 TargetY = CenterY + OffY;

				if (!IsValidCoord(TargetX, TargetY, 0))
				{
					continue;
				}

				const float Dist = FVector2D::Distance(FVector2D(TargetX, TargetY), FVector2D(CenterX, CenterY));
				if (Dist <= ForestConfig.ClusterRadius)
				{
					const float Alpha = Dist / ForestConfig.ClusterRadius;
					const float SpawnProbability = FMath::Lerp(ForestConfig.CenterDensity, ForestConfig.EdgeDensity, Alpha);

					if (FMath::FRand() <= SpawnProbability)
					{
						const int32 TopH = TopHeightMap[TargetX + TargetY * GridSizeX];
						if (TopH < GridSizeZ - 1)
						{
							const int32 TreeCellIdx = GetCellIndex(TargetX, TargetY, TopH);
							GridCells[TreeCellIdx].BlockType = ETimberBlockType::TreeMature;
							GridCells[TreeCellIdx].bIsWalkable = false;
							GridCells[TreeCellIdx].TreeStage = ETreeGrowthStage::Mature;
						}
					}
				}
			}
		}
	}

	// ========================================================
	// 4. BATCH INSTANCES CREATION (GOM TRANSFORM VÀ THÊM 1 LẦN DUY NHẤT - CỰC NHANH!)
	// ========================================================
	struct FBlockBatch
	{
		TArray<FTransform> Transforms;
		TArray<int32> CellIndices;
	};
	TMap<ETimberBlockType, FBlockBatch> Batches;

	for (int32 CellIdx = 0; CellIdx < GridCells.Num(); ++CellIdx)
	{
		const FTimberCell& Cell = GridCells[CellIdx];
		if (Cell.BlockType == ETimberBlockType::None)
		{
			continue;
		}

		FVector MeshOffset = FVector::ZeroVector;
		FVector MeshScale = FVector(1.0f);
		if (const FTimberBlockMeshConfig* Config = BlockMeshConfigs.Find(Cell.BlockType))
		{
			MeshOffset = Config->MeshOffset;
			MeshScale = Config->MeshScale;
		}

		const FVector WorldPos = GridCoordToWorldLocation(Cell.GridCoord, true) + MeshOffset;
		const FTransform InstTransform(FRotator::ZeroRotator, WorldPos, MeshScale);

		FBlockBatch& Batch = Batches.FindOrAdd(Cell.BlockType);
		Batch.Transforms.Add(InstTransform);
		Batch.CellIndices.Add(CellIdx);
	}

	for (auto& Pair : Batches)
	{
		const ETimberBlockType BType = Pair.Key;
		FBlockBatch& Batch = Pair.Value;

		UInstancedStaticMeshComponent* ISM = GetOrCreateISMComponent(BType);
		if (ISM && ISM->GetStaticMesh() && Batch.Transforms.Num() > 0)
		{
			const int32 StartIdx = ISM->GetInstanceCount();
			ISM->AddInstances(Batch.Transforms, true);

			for (int32 i = 0; i < Batch.CellIndices.Num(); ++i)
			{
				GridCells[Batch.CellIndices[i]].InstanceIndex = StartIdx + i;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ATimberGridManager: Đã sinh thành công Bản đồ Solid và Cụm rừng bằng Batch ISM Rendering!"));
}

void ATimberGridManager::DrawDebugGridBounds()
{
#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	const FVector Extents = FVector(
		(GridSizeX * CellSize) * 0.5f,
		(GridSizeY * CellSize) * 0.5f,
		(GridSizeZ * CellSize) * 0.5f
	);
	const FVector Center = Origin + Extents;

	// Vẽ hộp bao quanh toàn bộ bản đồ
	DrawDebugBox(World, Center, Extents, FColor::Cyan, false, 5.0f, 0, 4.0f);

	// In thông tin debug
	const FString DebugMsg = FString::Printf(TEXT("Grid Dimensions: [%d x %d x %d] | Total Cells: %d"), 
		GridSizeX, GridSizeY, GridSizeZ, (GridSizeX * GridSizeY * GridSizeZ));
	
	DrawDebugString(World, Center + FVector(0, 0, Extents.Z + 50.0f), DebugMsg, nullptr, FColor::White, 5.0f, true, 1.2f);
#endif
}
