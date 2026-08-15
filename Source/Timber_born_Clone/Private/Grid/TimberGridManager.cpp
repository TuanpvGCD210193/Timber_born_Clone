// Copyright Timberborn Clone Project. All Rights Reserved.

#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "Timber_born_Clone/Public/Pathfinding/TimberAStar.h"
#include "DrawDebugHelpers.h"

#if WITH_EDITOR
#include "Misc/ScopedSlowTask.h"
#endif

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

#if WITH_EDITOR
	// Tự động kiểm tra và vẽ lại bản đồ đã lưu trong Viewport Editor khi mở Level hoặc sau khi Rebuild
	if (SavedGridData.Num() > 0)
	{
		TArray<UInstancedStaticMeshComponent*> AllAttachedISMs;
		GetComponents<UInstancedStaticMeshComponent>(AllAttachedISMs);

		int32 TotalInstances = 0;
		for (UInstancedStaticMeshComponent* ISM : AllAttachedISMs)
		{
			if (IsValid(ISM))
			{
				TotalInstances += ISM->GetInstanceCount();
			}
		}

		if (TotalInstances == 0)
		{
			LoadTerrainData();
		}
	}
#endif
}

void ATimberGridManager::BeginPlay()
{
	Super::BeginPlay();

	// Tự động phục hồi bản đồ đã lưu nếu có dữ liệu
	if (SavedGridData.Num() > 0)
	{
		LoadTerrainData();
	}
	else if (bAutoInitializeOnBeginPlay && GridCells.Num() == 0)
	{
		InitializeGrid();
		RebuildISMComponents();
	}
}

void ATimberGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Thúc đẩy vòng đời cây sinh trưởng (Stump -> Sapling -> Mature)
	AdvanceTreeGrowth(DeltaTime);
}

bool ATimberGridManager::ChopTree(const FIntVector& Coord, int32& OutWoodEarned)
{
	OutWoodEarned = 0;

	if (!IsValidGridCoord(Coord))
	{
		return false;
	}

	const int32 CellIdx = GetCellIndex(Coord.X, Coord.Y, Coord.Z);
	FTimberCell& Cell = GridCells[CellIdx];

	if (Cell.BlockType == ETimberBlockType::TreeMature)
	{
		OutWoodEarned = TreeConfig.WoodPerMatureTree;
		SetBlock(Coord, ETimberBlockType::TreeStump, false);
		GridCells[CellIdx].TreeStage = ETreeGrowthStage::Stump;
		GridCells[CellIdx].TreeGrowthTimer = TreeConfig.StumpToSaplingDuration;
		return true;
	}
	else if (Cell.BlockType == ETimberBlockType::TreeSapling)
	{
		OutWoodEarned = 1; // Cây non cho 1 gỗ
		SetBlock(Coord, ETimberBlockType::TreeStump, false);
		GridCells[CellIdx].TreeStage = ETreeGrowthStage::Stump;
		GridCells[CellIdx].TreeGrowthTimer = TreeConfig.StumpToSaplingDuration;
		return true;
	}

	return false;
}

bool ATimberGridManager::PlantSapling(const FIntVector& Coord)
{
	if (!IsValidGridCoord(Coord))
	{
		return false;
	}

	const int32 CellIdx = GetCellIndex(Coord.X, Coord.Y, Coord.Z);
	FTimberCell& Cell = GridCells[CellIdx];

	if (Cell.BlockType == ETimberBlockType::None || Cell.BlockType == ETimberBlockType::TreeStump)
	{
		SetBlock(Coord, ETimberBlockType::TreeSapling, false);
		GridCells[CellIdx].TreeStage = ETreeGrowthStage::Sapling;
		GridCells[CellIdx].TreeGrowthTimer = TreeConfig.SaplingToMatureDuration;
		return true;
	}

	return false;
}

void ATimberGridManager::AdvanceTreeGrowth(float DeltaTime)
{
	if (!TreeConfig.bAutoRegrowth || GridCells.Num() == 0)
	{
		return;
	}

	for (int32 i = 0; i < GridCells.Num(); ++i)
	{
		FTimberCell& Cell = GridCells[i];

		if (Cell.TreeStage == ETreeGrowthStage::Stump)
		{
			Cell.TreeGrowthTimer -= DeltaTime;
			if (Cell.TreeGrowthTimer <= 0.0f)
			{
				const FIntVector Coord = Cell.GridCoord;
				SetBlock(Coord, ETimberBlockType::TreeSapling, false);
				GridCells[i].TreeStage = ETreeGrowthStage::Sapling;
				GridCells[i].TreeGrowthTimer = TreeConfig.SaplingToMatureDuration;
			}
		}
		else if (Cell.TreeStage == ETreeGrowthStage::Sapling)
		{
			Cell.TreeGrowthTimer -= DeltaTime;
			if (Cell.TreeGrowthTimer <= 0.0f)
			{
				const FIntVector Coord = Cell.GridCoord;
				SetBlock(Coord, ETimberBlockType::TreeMature, false);
				GridCells[i].TreeStage = ETreeGrowthStage::Mature;
				GridCells[i].TreeGrowthTimer = 0.0f;
			}
		}
	}
}

bool ATimberGridManager::BuildPath(const FIntVector& Coord)
{
	if (!IsValidGridCoord(Coord))
	{
		return false;
	}

	// Đặt khối DirtPath lên ô lưới và hiển thị ISM tức thì
	const bool bSuccess = SetBlock(Coord, ETimberBlockType::DirtPath, true);
	if (bSuccess)
	{
		if (!PathGraph)
		{
			PathGraph = NewObject<UTimberPathGraph>(this, TEXT("TimberPathGraph"));
		}

		PathGraph->AddPathNode(Coord, this);
		return true;
	}

	return false;
}

bool ATimberGridManager::RemovePath(const FIntVector& Coord)
{
	if (!IsValidGridCoord(Coord))
	{
		return false;
	}

	const int32 CellIdx = GetCellIndexFromVector(Coord);
	if (GridCells[CellIdx].BlockType == ETimberBlockType::DirtPath)
	{
		ClearBlock(Coord);
		if (PathGraph)
		{
			PathGraph->RemovePathNode(Coord);
		}
		return true;
	}

	return false;
}

bool ATimberGridManager::HasPathAt(const FIntVector& Coord) const
{
	if (!IsValidGridCoord(Coord))
	{
		return false;
	}

	const int32 CellIdx = GetCellIndexFromVector(Coord);
	return (GridCells[CellIdx].BlockType == ETimberBlockType::DirtPath);
}

bool ATimberGridManager::FindPath(
	const FIntVector& StartCoord, 
	const FIntVector& TargetCoord, 
	TArray<FIntVector>& OutPath, 
	bool bRequirePathAtTarget) const
{
	return UTimberAStar::FindPath(this, StartCoord, TargetCoord, OutPath, bRequirePathAtTarget);
}

void ATimberGridManager::DrawDebugPath(const TArray<FIntVector>& Path, FColor LineColor, float Duration)
{
#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (!World || Path.Num() == 0)
	{
		return;
	}

	for (int32 i = 0; i < Path.Num(); ++i)
	{
		const FVector NodePos = GridCoordToWorldLocation(Path[i], true) + FVector(0, 0, 20.0f);
		
		// Vẽ điểm nút hình hộp
		DrawDebugBox(World, NodePos, FVector(15.0f, 15.0f, 15.0f), LineColor, false, Duration, 0, 2.0f);

		// Nối đường line tới nút tiếp theo
		if (i < Path.Num() - 1)
		{
			const FVector NextPos = GridCoordToWorldLocation(Path[i + 1], true) + FVector(0, 0, 20.0f);
			DrawDebugLine(World, NodePos, NextPos, LineColor, false, Duration, 0, 4.0f);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ATimberGridManager: Đã vẽ Debug Path gồm %d bước trong Viewport."), Path.Num());
#endif
}

bool ATimberGridManager::IsBuildingConnectedToDistrict(const FIntVector& BuildingDoorCoord, int32& OutPathDistance) const
{
	OutPathDistance = 0;

	if (!PathGraph)
	{
		return false;
	}

	return PathGraph->IsReachable(DistrictCenterDoorCoord, BuildingDoorCoord, OutPathDistance);
}

void ATimberGridManager::DrawDebugDistrictNetwork()
{
#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (!World || !PathGraph)
	{
		return;
	}

	const TArray<FIntVector> ReachableNodes = PathGraph->GetAllReachableNodes(DistrictCenterDoorCoord);

	for (const FIntVector& NodeCoord : ReachableNodes)
	{
		const FVector NodePos = GridCoordToWorldLocation(NodeCoord, true) + FVector(0, 0, 25.0f);
		DrawDebugBox(World, NodePos, FVector(20.0f, 20.0f, 5.0f), FColor::Cyan, false, 10.0f, 0, 2.0f);

		// Nối đường line tới các láng giềng đã kết nối
		const TArray<FIntVector> Neighbors = PathGraph->GetConnectedNeighbors(NodeCoord);
		for (const FIntVector& NeighborCoord : Neighbors)
		{
			const FVector NeighborPos = GridCoordToWorldLocation(NeighborCoord, true) + FVector(0, 0, 25.0f);
			DrawDebugLine(World, NodePos, NeighborPos, FColor::Yellow, false, 10.0f, 0, 3.0f);
		}
	}

	// Đánh dấu ô cửa District Center bằng hộp phát sáng màu Magenta
	const FVector DoorWorldPos = GridCoordToWorldLocation(DistrictCenterDoorCoord, true) + FVector(0, 0, 35.0f);
	DrawDebugBox(World, DoorWorldPos, FVector(25.0f, 25.0f, 25.0f), FColor::Magenta, false, 10.0f, 0, 4.0f);

	UE_LOG(LogTemp, Log, TEXT("ATimberGridManager: Đã vẽ mạng lưới District Network gồm %d nút đường hợp lệ kết nối về District Center."), ReachableNodes.Num());
#endif
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

	// 3. Làm sạch đồ thị đường đi
	if (PathGraph)
	{
		PathGraph->ClearAllNodes();
	}

	// 4. Reset toàn bộ dữ liệu ô lưới
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
#if WITH_EDITOR
	FScopedSlowTask SlowTask(5.0f, FText::FromString(TEXT("Đang sinh Bản đồ tự nhiên & Cụm rừng...")));
	SlowTask.MakeDialog();
#endif

	// ========================================================
	// BƯỚC 1 (20%): KHỞI TẠO GRID & LÀM SẠCH INSTANCES
	// ========================================================
#if WITH_EDITOR
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Bước 1/5: Khởi tạo Grid & Làm sạch ISM...")));
#endif
	UE_LOG(LogTemp, Log, TEXT("[====................]  20%% - Khởi tạo Grid & Làm sạch ISM..."));

	InitializeGrid();
	ClearAllInstances();
	RebuildISMComponents();

	// Khởi tạo Random Stream theo MapSeed (Seed > 0: Tái tạo 100% bản đồ cố định; Seed = 0: Ngẫu nhiên)
	const int32 EffectiveSeed = (TerrainConfig.MapSeed != 0) ? TerrainConfig.MapSeed : FMath::RandRange(1, 9999999);
	FRandomStream RandStream(EffectiveSeed);
	UE_LOG(LogTemp, Log, TEXT("ATimberGridManager: Đang sinh bản đồ với Map Seed: %d"), EffectiveSeed);

	const int32 BaseH = FMath::Clamp(TerrainConfig.BaseHeight, 1, GridSizeZ - 2);

	// ========================================================
	// BƯỚC 2 (40%): TÍNH TOÁN CAO NGUYÊN ĐA TẦNG & VÁCH ĐÁ LỒI LÕM (STEP 1.3.1)
	// ========================================================
#if WITH_EDITOR
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Bước 2/5: Sinh Cao nguyên đa tầng & Vách đá lồi lõm...")));
#endif
	UE_LOG(LogTemp, Log, TEXT("[========............]  40%% - Sinh Cao nguyên đa tầng & Vách đá lồi lõm..."));

	TArray<int32> TopHeightMap;
	TopHeightMap.Init(BaseH, GridSizeX * GridSizeY);

	// 1. Sinh các đại cao nguyên bậc thang phẳng rộng lớn (Broad Multi-tier Mesas - Hình 2 & 3)
	for (int32 Plateau = 0; Plateau < TerrainConfig.PlateauCount; ++Plateau)
	{
		const int32 Margin = FMath::Clamp(TerrainConfig.PlateauRadius / 2, 2, GridSizeX / 3);
		const int32 CenterX = RandStream.RandRange(Margin, FMath::Max(Margin, GridSizeX - Margin));
		const int32 CenterY = RandStream.RandRange(Margin, FMath::Max(Margin, GridSizeY - Margin));

		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			for (int32 X = 0; X < GridSizeX; ++X)
			{
				const float Dist = FVector2D::Distance(FVector2D(X, Y), FVector2D(CenterX, CenterY));
				const float Angle = FMath::Atan2(static_cast<float>(Y - CenterY), static_cast<float>(X - CenterX));
				const float OrganicRadius = TerrainConfig.PlateauRadius * (1.0f + 0.25f * FMath::Sin(Angle * 3.0f + Plateau * 2.1f));

				if (Dist <= OrganicRadius)
				{
					// Phân tầng bậc thang cao nguyên rộng (Mesa Terraces)
					const float Factor = 1.0f - (Dist / OrganicRadius);
					const int32 Tier = FMath::Clamp(FMath::FloorToInt(Factor * TerrainConfig.MaxTiers), 0, TerrainConfig.MaxTiers - 1);
					int32 ExtraHeight = (Tier + 1) * TerrainConfig.TierHeight;

					// VÁCH ĐÁ LỒI LÕM SO LE (HÌNH 2: Răng cưa phong hóa tự nhiên)
					if (TerrainConfig.CliffJaggedness > 0.0f)
					{
						const float PillarNoise = FMath::PerlinNoise2D(FVector2D(X * 0.85f + Plateau * 13.7f, Y * 0.85f + Plateau * 17.3f));
						if (PillarNoise > (1.0f - TerrainConfig.CliffJaggedness * 0.8f))
						{
							ExtraHeight += 1; // Cột đá nhô cao thêm 1 block
						}
					}

					const int32 MapIdx = X + Y * GridSizeX;
					TopHeightMap[MapIdx] = FMath::Max(TopHeightMap[MapIdx], FMath::Min(BaseH + ExtraHeight, GridSizeZ - 2));
				}
			}
		}
	}

	// ========================================================
	// BƯỚC 3 (60%): ĐIỀN DỮ LIỆU ĐẠI BIOME LIỀN MẠCH (HÌNH 2 & 3)
	// ========================================================
#if WITH_EDITOR
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Bước 3/5: Phân bổ Đại vùng Cỏ xanh & Đất hoang...")));
#endif
	UE_LOG(LogTemp, Log, TEXT("[============........]  60%% - Phân bổ Đại vùng Cỏ xanh & Đất hoang..."));

	for (int32 Y = 0; Y < GridSizeY; ++Y)
	{
		for (int32 X = 0; X < GridSizeX; ++X)
		{
			const int32 TopH = TopHeightMap[X + Y * GridSizeX];
			const bool bIsElevated = (TopH > BaseH);

			// Tần số thấp (NoiseScale = 0.045f) tạo đại vùng thung lũng cỏ liền mạch không chó đốm
			const float RawNoise = FMath::PerlinNoise2D(FVector2D(
				X * TerrainConfig.NoiseScale + EffectiveSeed * 0.03f, 
				Y * TerrainConfig.NoiseScale + EffectiveSeed * 0.03f
			));
			const float BiomeNoise = FMath::Clamp((RawNoise + 1.0f) * 0.5f, 0.0f, 1.0f);

			for (int32 Z = 0; Z < TopH; ++Z)
			{
				const int32 CellIdx = GetCellIndex(X, Y, Z);
				
				if (Z == TopH - 1)
				{
					// --- LỚP BỀ MẶT TRÊN CÙNG (SURFACE LAYER) ---
					if (TerrainConfig.bHillTopAlwaysRock && bIsElevated)
					{
						// Đỉnh cao nhất của cao nguyên luôn là khối Đá (Cliff)
						GridCells[CellIdx].BlockType = ETimberBlockType::Cliff;
					}
					else
					{
						// Đại vùng Cỏ xanh màu mỡ liền mạch vs Đại vùng Đất khô cằn rộng lớn (Không chó đốm!)
						if (BiomeNoise < TerrainConfig.GrassRatio)
						{
							GridCells[CellIdx].BlockType = ETimberBlockType::Grass;
						}
						else
						{
							GridCells[CellIdx].BlockType = ETimberBlockType::Dirt;
						}
					}

					// Bề mặt là có thể đi lại
					GridCells[CellIdx].bIsWalkable = true;
				}
				else
				{
					// --- LỚP THÂN VÀ ĐÁY BÊN DƯỚI (SUBSURFACE LAYER) ---
					if (bIsElevated && Z >= BaseH - 1)
					{
						GridCells[CellIdx].BlockType = ETimberBlockType::Cliff; // Toàn bộ vách thành cao nguyên dựng đứng là Đá!
					}
					else
					{
						GridCells[CellIdx].BlockType = ETimberBlockType::Dirt; // Lòng đất sâu
					}
					GridCells[CellIdx].bIsWalkable = false;
				}
			}
		}
	}

	// ========================================================
	// BƯỚC 4 (80%): RẢI CÁC CỤM RỪNG HỮU CƠ TRÊN NỀN CỎ
	// ========================================================
#if WITH_EDITOR
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Bước 4/5: Rải cụm rừng Cây hữu cơ trên nền cỏ...")));
#endif
	UE_LOG(LogTemp, Log, TEXT("[================....]  80%% - Rải cụm rừng Cây hữu cơ trên nền cỏ..."));

	for (int32 Cluster = 0; Cluster < ForestConfig.ClusterCount; ++Cluster)
	{
		const int32 Margin = ForestConfig.ClusterRadius + 1;
		const int32 CenterX = RandStream.RandRange(Margin, FMath::Max(Margin, GridSizeX - Margin));
		const int32 CenterY = RandStream.RandRange(Margin, FMath::Max(Margin, GridSizeY - Margin));

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
					const int32 TopH = TopHeightMap[TargetX + TargetY * GridSizeX];
					if (TopH <= 0 || TopH >= GridSizeZ - 1)
					{
						continue;
					}

					// Lấy thông tin khối bề mặt ngay dưới chân cây (Step 1.3.3)
					const int32 SurfaceIdx = GetCellIndex(TargetX, TargetY, TopH - 1);
					const ETimberBlockType SurfaceType = GridCells[SurfaceIdx].BlockType;

					// Phân cấp tỷ lệ mọc cây theo loại khối bề mặt:
					float BiomeMultiplier = 0.0f;
					if (SurfaceType == ETimberBlockType::Grass)
					{
						BiomeMultiplier = ForestConfig.GrassTreeDensity; // Dày đặc trên Cỏ xanh (0.90)
					}
					else if (SurfaceType == ETimberBlockType::Dirt)
					{
						BiomeMultiplier = ForestConfig.DirtTreeDensity; // Thưa thớt trên Đất khô (0.25)
					}
					else
					{
						BiomeMultiplier = ForestConfig.RockTreeDensity; // Tuyệt đối 0.0% trên Đá (0.00)
					}

					if (BiomeMultiplier <= 0.0f)
					{
						continue; // Bỏ qua nếu là Đá (Cliff)
					}

					const float Alpha = Dist / ForestConfig.ClusterRadius;
					const float BaseProbability = FMath::Lerp(ForestConfig.CenterDensity, ForestConfig.EdgeDensity, Alpha);
					const float FinalProbability = BaseProbability * BiomeMultiplier;

					if (RandStream.FRand() <= FinalProbability)
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

	// ========================================================
	// BƯỚC 5 (100%): BATCH INSTANCES CREATION (GOM VÀ THÊM 1 LẦN DUY NHẤT)
	// ========================================================
#if WITH_EDITOR
	SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Bước 5/5: Hoàn tất Batch ISM Rendering...")));
#endif

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

	// Tự động sao lưu dữ liệu địa hình vừa sinh vào SavedGridData
	SavedGridData = GridCells;
	Modify();
	MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("[====================] 100%% - Hoàn tất Batch ISM Rendering & Đã lưu dữ liệu vào Actor!"));
}

void ATimberGridManager::SaveTerrainData()
{
	SavedGridData = GridCells;
	Modify();
	MarkPackageDirty();
	UE_LOG(LogTemp, Log, TEXT("ATimberGridManager: Đã lưu vĩnh viễn %d ô dữ liệu địa hình vào Actor/Level Package!"), SavedGridData.Num());
}

void ATimberGridManager::LoadTerrainData()
{
	if (SavedGridData.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATimberGridManager: Không tìm thấy dữ liệu địa hình đã lưu trong Actor!"));
		return;
	}

	// Dọn dẹp Level trước
	ClearAllInstances();
	RebuildISMComponents();

	// Nạp lại dữ liệu đã lưu
	GridCells = SavedGridData;

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

	// Đồng bộ lại các điểm đường đi vào PathGraph
	if (PathGraph)
	{
		PathGraph->ClearAllNodes();
	}
	else
	{
		PathGraph = NewObject<UTimberPathGraph>(this, TEXT("TimberPathGraph"));
	}

	for (const FTimberCell& Cell : GridCells)
	{
		if (Cell.BlockType == ETimberBlockType::DirtPath)
		{
			PathGraph->AddPathNode(Cell.GridCoord, this);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ATimberGridManager: Đã phục hồi và tái tạo thành công %d ô địa hình từ dữ liệu lưu trữ!"), SavedGridData.Num());
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
