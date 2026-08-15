// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Timber_born_Clone/Public/Grid/TimberGridTypes.h"
#include "TimberAStar.generated.h"

class ATimberGridManager;

/**
 * Điểm nút phục vụ hàng đợi ưu tiên của thuật toán A*
 */
struct FAStarNode
{
	FIntVector Coord;
	float GScore = TNumericLimits<float>::Max(); // Chi phí thực tế từ Start
	float HScore = 0.0f;                         // Heuristic tới Target
	float FScore() const { return GScore + HScore; }

	FAStarNode() = default;
	FAStarNode(const FIntVector& InCoord, float InGScore, float InHScore)
		: Coord(InCoord), GScore(InGScore), HScore(InHScore)
	{
	}

	bool operator<(const FAStarNode& Other) const
	{
		return FScore() > Other.FScore(); // Min-Heap: Phần tử nhỏ hơn sẽ có ưu tiên cao hơn
	}
};

/**
 * Lớp thuật toán A* Pathfinding 3D tối ưu cho địa hình lưới Voxel
 * Hỗ trợ leo bậc chênh lệch <= 1 block, ưu tiên chạy trên đường đất (DirtPath)
 */
UCLASS()
class TIMBER_BORN_CLONE_API UTimberAStar : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Thuật toán A* tìm đường đi ngắn nhất giữa 2 điểm trên lưới 3D
	 * @param GridManager Con trỏ tới Grid Manager
	 * @param StartCoord Tọa độ điểm bắt đầu
	 * @param TargetCoord Tọa độ điểm đích
	 * @param OutPath Mảng chứa danh sách các ô đường đi tìm được (từ Start -> Target)
	 * @param bRequirePathAtTarget Nếu true, bắt buộc điểm Target phải có đường kết nối
	 * @return true nếu tìm thấy đường đi hợp lệ
	 */
	UFUNCTION(BlueprintCallable, Category = "Timber|Pathfinding")
	static bool FindPath(
		const ATimberGridManager* GridManager,
		const FIntVector& StartCoord,
		const FIntVector& TargetCoord,
		TArray<FIntVector>& OutPath,
		bool bRequirePathAtTarget = false
	);

	/** Tính khoảng cách Heuristic 3D giữa 2 điểm (Manhattan + Euclidean Z) */
	static float CalculateHeuristic(const FIntVector& A, const FIntVector& B);

	/** Tính chi phí di chuyển giữa 2 ô lân cận */
	static float CalculateStepCost(
		const ATimberGridManager* GridManager,
		const FIntVector& From,
		const FIntVector& To
	);
};
