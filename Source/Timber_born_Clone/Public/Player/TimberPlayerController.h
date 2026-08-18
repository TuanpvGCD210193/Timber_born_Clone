// Copyright Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Timber_born_Clone/Public/Grid/TimberGridTypes.h"
#include "TimberPlayerController.generated.h"

class ATimberGridManager;
class ATimberBuildingBase;
class UUserWidget;

/**
 * Player Controller điều khiển tương tác chuột, công cụ cọ vẽ & HUD xây dựng (Step 3.4)
 */
UCLASS()
class TIMBER_BORN_CLONE_API ATimberPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATimberPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	// ==========================================
	// BRUSH & TOOL CONFIGURATION
	// ==========================================

	/** Chế độ công cụ cọ vẽ hiện tại */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Tool State")
	ETimberBrushMode CurrentBrushMode = ETimberBrushMode::None;

	/** Lớp công trình đang được chọn để đặt móng */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Tool State")
	TSubclassOf<ATimberBuildingBase> SelectedBuildingClass = nullptr;

	/** Góc xoay hiện tại của công trình (0, 90, 180, 270 độ) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Tool State")
	int32 BuildingRotationAngle = 0;

	/** Class UI Widget HUD xây dựng để tự động sinh ra khi BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|UI")
	TSubclassOf<UUserWidget> BuildHUDWidgetClass = nullptr;

	/** Con trỏ tới Widget HUD đang hiển thị trên màn hình */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|UI")
	TObjectPtr<UUserWidget> BuildHUDWidgetInstance = nullptr;

	// ==========================================
	// HUD TOOL SELECTORS (BLUEPRINT CALLABLE)
	// ==========================================

	/** Chọn công cụ: Lát Đường (Dirt Path) */
	UFUNCTION(BlueprintCallable, Category = "Timber|Tools")
	void SelectTool_PaintPath();

	/** Chọn công cụ: Phá Hủy Đa Năng (Xóa đường & Tháo dỡ công trình) */
	UFUNCTION(BlueprintCallable, Category = "Timber|Tools")
	void SelectTool_Demolish();

	/** Chọn công cụ: Đặt Móng Công Trình */
	UFUNCTION(BlueprintCallable, Category = "Timber|Tools")
	void SelectTool_PlaceBuilding(TSubclassOf<ATimberBuildingBase> BuildingClass);

	/** Hủy chọn công cụ, đưa chuột về trạng thái bình thường */
	UFUNCTION(BlueprintCallable, Category = "Timber|Tools")
	void SelectTool_Deselect();

	/** Xoay công trình 90 độ theo chiều kim đồng hồ */
	UFUNCTION(BlueprintCallable, Category = "Timber|Tools")
	void RotateBuildingClockwise();

	// ==========================================
	// RAYCAST & VALIDATION QUERIES
	// ==========================================

	/** Bắn tia Raycast từ vị trí Cursor chuột để xác định ô Grid mặt đất */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Math")
	bool GetCursorGridCoord(FIntVector& OutGridCoord, FVector& OutWorldHitPos) const;

	/**
	 * Kiểm tra xem một vùng đất N x M có đủ điều kiện đặt móng công trình không
	 * (Cùng độ cao Z, là mặt đất phẳng, không vướng cây/công trình khác)
	 */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Validation")
	bool IsAreaValidForBuilding(const FIntVector& GroundCoord, const FIntPoint& Footprint, int32 RotationDeg, TArray<FIntVector>& OutOccupiedCoords) const;

	/** Lấy con trỏ tới TimberGridManager đang quản lý bản đồ */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid")
	ATimberGridManager* GetGridManager() const;

protected:
	// ==========================================
	// INPUT HANDLERS
	// ==========================================

	/** Nhấn chuột trái: Kích hoạt Lát đường / Phá hủy / Đặt móng */
	void OnLeftMouseDown();

	/** Nhả chuột trái: Dừng kéo vẽ đường */
	void OnLeftMouseUp();

	/** Nhấn chuột phải: Hủy chọn công cụ hiện tại */
	void OnRightMouseDown();

	/** Nhấn phím R: Xoay hướng công trình */
	void OnRotateKeyPressed();

	// ==========================================
	// ACTION EXECUTIONS
	// ==========================================

	/** Thực thi hành động khi Click hoặc Kéo chuột tại ô Grid */
	void ExecuteBrushAction(const FIntVector& TargetCoord);

	/** Lát đường tại ô chỉ định */
	void ExecutePaintPath(const FIntVector& TargetCoord);

	/** Phá hủy đường hoặc tháo dỡ công trình tại ô chỉ định */
	void ExecuteDemolish(const FIntVector& TargetCoord);

	/** Đặt móng công trình mới tại ô mặt đất chỉ định */
	void ExecutePlaceBuilding(const FIntVector& GroundCoord);

	// ==========================================
	// HOLOGRAM PREVIEW MANAGEMENT
	// ==========================================

	/** Cập nhật vị trí và màu sắc (Xanh/Đỏ) của Hologram Preview theo chuột */
	void UpdateHologramPreview(const FIntVector& GroundCoord, bool bIsValid);

	/** Xóa và hủy Hologram Preview khi đổi công cụ hoặc tắt Brush */
	void ClearHologramPreview();

	/** Tạo mới một Hologram Preview Actor tương ứng với SelectedBuildingClass */
	void SpawnHologramPreview();

	/** Tính toán danh sách các ô đường thẳng trực giao từ Start đến End khi kéo chuột */
	void CalculateDragPathCoords(const FIntVector& StartGroundCoord, const FIntVector& EndGroundCoord, TArray<FIntVector>& OutPathCoords) const;

	/** Xây dựng đồng loạt toàn bộ các ô đường hợp lệ khi người chơi thả chuột trái */
	void ExecuteBatchBuildPath(const TArray<FIntVector>& PathCoords);

	/** Bật/Tắt hiển thị Mũi tên 3D chỉ hướng cửa cho tất cả các công trình trên bản đồ */
	void SetAllBuildingsDoorArrowVisible(bool bVisible);

private:
	/** Tham chiếu cache tới GridManager */
	UPROPERTY()
	mutable TWeakObjectPtr<ATimberGridManager> CachedGridManager = nullptr;

	/** Actor Hologram tạm thời dùng để hiển thị bóng móng khi di chuột */
	UPROPERTY()
	TObjectPtr<ATimberBuildingBase> HologramPreviewActor = nullptr;

	/** Cờ đánh dấu chuột trái đang được giữ đè */
	bool bIsLeftMouseDown = false;

	/** Cờ đánh dấu đang kéo thả vẽ đường đi (Drag-to-Build) */
	bool bIsDraggingPath = false;

	/** Tọa độ ô mặt đất bắt đầu kéo đường */
	FIntVector DragStartGroundCoord = FIntVector::ZeroValue;

	/** Danh sách các ô đường trong đoạn kéo hiện tại */
	TArray<FIntVector> CachedDragPathCoords;

	/** Tọa độ ô lưới gần nhất mà chuột vừa xử lý */
	FIntVector LastProcessedCoord = FIntVector(-999, -999, -999);

	/** Tọa độ ô lưới mặt đất hiện tại dưới con trỏ chuột */
	FIntVector CurrentHoverGroundCoord = FIntVector::ZeroValue;

	/** Trạng thái hợp lệ của vị trí hover hiện tại */
	bool bIsCurrentHoverValid = false;
};
