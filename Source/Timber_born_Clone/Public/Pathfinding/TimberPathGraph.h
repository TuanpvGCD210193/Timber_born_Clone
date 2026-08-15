// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Timber_born_Clone/Public/Grid/TimberGridTypes.h"
#include "TimberPathGraph.generated.h"

class ATimberGridManager;

/**
 * Cấu trúc dữ liệu cho 1 điểm nút đường đi (Path Node)
 */
USTRUCT(BlueprintType)
struct TIMBER_BORN_CLONE_API FTimberPathNode
{
	GENERATED_BODY()

	/** Tọa độ ô lưới của điểm đường */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Path Node")
	FIntVector Coord = FIntVector::ZeroValue;

	/** Danh sách tọa độ các điểm đường lân cận đã kết nối (Đông, Tây, Nam, Bắc) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Path Node")
	TArray<FIntVector> ConnectedNeighbors;

	/** Hệ số tăng tốc di chuyển khi đi trên đường (+50% = 1.5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Path Node")
	float SpeedMultiplier = 1.5f;

	FTimberPathNode() = default;

	explicit FTimberPathNode(const FIntVector& InCoord, float InSpeedMultiplier = 1.5f)
		: Coord(InCoord), SpeedMultiplier(InSpeedMultiplier)
	{
	}
};

/**
 * Hệ thống Quản lý Đồ thị Mạng lưới Đường đi (Path Network Graph)
 * Hỗ trợ lưu trữ, tự động liên kết các đường lân cận và leo dốc <= 1 block
 */
UCLASS(BlueprintType)
class TIMBER_BORN_CLONE_API UTimberPathGraph : public UObject
{
	GENERATED_BODY()

public:
	UTimberPathGraph();

	/** Bảng băm lưu trữ toàn bộ các điểm đường đi theo tọa độ FIntVector */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Path Graph")
	TMap<FIntVector, FTimberPathNode> Nodes;

	/**
	 * Thêm một điểm nút đường mới vào đồ thị và tự động kết nối với các đường lân cận
	 * @param Coord Tọa độ ô lưới của đường mới
	 * @param GridManager Con trỏ tới Grid Manager để kiểm tra tính hợp lệ
	 * @return true nếu thêm thành công
	 */
	UFUNCTION(BlueprintCallable, Category = "Timber|Path Graph")
	bool AddPathNode(const FIntVector& Coord, const ATimberGridManager* GridManager);

	/**
	 * Xóa một điểm nút đường khỏi đồ thị và ngắt kết nối với các láng giềng
	 * @param Coord Tọa độ ô lưới cần xóa
	 * @return true nếu xóa thành công
	 */
	UFUNCTION(BlueprintCallable, Category = "Timber|Path Graph")
	bool RemovePathNode(const FIntVector& Coord);

	/** Kiểm tra tại tọa độ có tồn tại đường đi không */
	UFUNCTION(BlueprintPure, Category = "Timber|Path Graph")
	bool HasPathNode(const FIntVector& Coord) const;

	/** Lấy thông tin điểm nút đường tại tọa độ */
	const FTimberPathNode* GetPathNode(const FIntVector& Coord) const;

	/** Lấy danh sách các nút lân cận kết nối trực tiếp với nút này */
	UFUNCTION(BlueprintPure, Category = "Timber|Path Graph")
	TArray<FIntVector> GetConnectedNeighbors(const FIntVector& Coord) const;

	/** Xóa sạch toàn bộ đồ thị đường đi */
	UFUNCTION(BlueprintCallable, Category = "Timber|Path Graph")
	void ClearAllNodes();

	/** Tái xây dựng toàn bộ liên kết cạnh cho tất cả các nút hiện có */
	UFUNCTION(BlueprintCallable, Category = "Timber|Path Graph")
	void RebuildAllAdjacency(const ATimberGridManager* GridManager);

	/**
	 * Kiểm tra xem từ StartCoord có đường đi kết nối liên tục tới TargetCoord hay không (Thuật toán BFS)
	 * @param StartCoord Tọa độ điểm bắt đầu (ví dụ: Cửa District Center)
	 * @param TargetCoord Tọa độ điểm đích (ví dụ: Cửa Công trình / Trại đốn gỗ)
	 * @param OutDistance Tổng số bước / khoảng cách đường đi
	 * @return true nếu kết nối thông suốt
	 */
	UFUNCTION(BlueprintCallable, Category = "Timber|Path Graph")
	bool IsReachable(const FIntVector& StartCoord, const FIntVector& TargetCoord, int32& OutDistance) const;

	/**
	 * Lấy toàn bộ danh sách các điểm nút đường đi kết nối liên tục từ 1 nút gốc (BFS Flood Fill)
	 * @param RootCoord Tọa độ điểm gốc (ví dụ: Cửa District Center)
	 * @return Mảng danh sách tọa độ các điểm đường kết nối
	 */
	UFUNCTION(BlueprintCallable, Category = "Timber|Path Graph")
	TArray<FIntVector> GetAllReachableNodes(const FIntVector& RootCoord) const;
};
