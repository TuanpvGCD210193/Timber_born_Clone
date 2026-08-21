#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimberConstructionManager.generated.h"

class ATimberBuildingBase;
class ABeaverAgent;
class ATimberGridManager;

/**
 * Toán tử so sánh 2 tầng cho Hàng Đợi Xây Dựng (Priority-First & FIFO Ordered Queue)
 * 1. Tầng 1: Priority cao hơn (4 > 3 > 2 > 1) luôn đứng trước!
 * 2. Tầng 2: Cùng Priority thì công trình đặt móng trước (PlacementOrderIndex nhỏ hơn) đứng trước!
 */
struct FBuildingQueuePredicate
{
	bool operator()(const TWeakObjectPtr<ATimberBuildingBase>& A, const TWeakObjectPtr<ATimberBuildingBase>& B) const;
};

/**
 * Sub-Class Chuyên Trách Quản Lý Xây Dựng & Hàng Đợi Cung Ứng Vật Liệu (Phase 4 - Step 4.3)
 * Áp dụng SOLID: Tách biệt hoàn toàn khỏi GridManager và BeaverAgent, đóng vai trò Tổng Công Trình Sư
 */
UCLASS(BlueprintType)
class TIMBER_BORN_CLONE_API UTimberConstructionManager : public UObject
{
	GENERATED_BODY()

public:
	UTimberConstructionManager();

	/** Khởi tạo liên kết với GridManager của thế giới */
	void Initialize(ATimberGridManager* InGridManager);

	/** Đăng ký một móng công trình mới vào Hàng đợi Xây dựng */
	UFUNCTION(BlueprintCallable, Category = "Timber|Construction")
	void RegisterConstructionSite(ATimberBuildingBase* Building);

	/** Hủy đăng ký một công trình khỏi Hàng đợi khi đã xây xong hoặc bị xóa */
	UFUNCTION(BlueprintCallable, Category = "Timber|Construction")
	void UnregisterConstructionSite(ATimberBuildingBase* Building);

	/** Sắp xếp lại toàn bộ Hàng đợi Xây dựng theo Priority và FIFO */
	UFUNCTION(BlueprintCallable, Category = "Timber|Construction")
	void SortConstructionQueue();

	/** Lấy danh sách toàn bộ các công trình đang chờ xây theo thứ tự ưu tiên */
	const TArray<TWeakObjectPtr<ATimberBuildingBase>>& GetConstructionQueue() const { return ConstructionQueue; }

	/** Lấy công trình móng ưu tiên cao nhất tiếp theo đang cần nạp gỗ */
	ATimberBuildingBase* GetNextSiteNeedingWood() const;

	/** Lấy công trình móng ưu tiên cao nhất tiếp theo đã đủ gỗ và đang cần thợ gõ búa */
	ATimberBuildingBase* GetNextSiteNeedingBuilders() const;

	/** Giữ slot trước khi Hải ly bắt đầu chạy tới móng, tránh điều thừa thợ. */
	bool ReserveBuilderSlot(ATimberBuildingBase* TargetSite);

	/** Hủy slot của một Hải ly chưa tới móng. */
	void ReleaseReservedBuilderSlot(ATimberBuildingBase* TargetSite);

	/**
	 * Tìm nguồn gỗ tối ưu cho móng công trình:
	 * Chỉ quét Kho / Nhà Chính đã hoàn thành, kết nối District và còn gỗ.
	 * Hải ly xây dựng tuyệt đối không tự nhận việc chặt cây.
	 * @return true nếu tìm thấy nguồn gỗ hợp lệ trong kho.
	 */
	bool FindBestWoodSourceForSite(ATimberBuildingBase* TargetSite, const FIntVector& BeaverLocation,
		FIntVector& OutSourceCoord, ATimberBuildingBase*& OutSourceBuilding);

	/** Chuyển slot đang trên đường thành builder đang gõ búa. */
	UFUNCTION(BlueprintCallable, Category = "Timber|Construction")
	bool AssignBuilderToSite(ATimberBuildingBase* TargetSite, ABeaverAgent* Beaver);

	/** Giải phóng 1 Hải ly rời khỏi móng xây dựng */
	UFUNCTION(BlueprintCallable, Category = "Timber|Construction")
	void ReleaseBuilderFromSite(ATimberBuildingBase* TargetSite, ABeaverAgent* Beaver);

	/** Cập nhật tiến độ thi công hợp lực cho công trình theo số lượng thợ hiện có (10%-40%/s) */
	UFUNCTION(BlueprintCallable, Category = "Timber|Construction")
	void AdvanceCooperativeConstruction(ATimberBuildingBase* TargetSite, float DeltaTime);

	/** Đếm tổng số công trình đang nằm trong hàng đợi xây dựng */
	UFUNCTION(BlueprintPure, Category = "Timber|Construction")
	int32 GetQueueCount() const { return ConstructionQueue.Num(); }

protected:
	/** Danh sách Hàng Đợi Xây Dựng đã được sắp xếp tự động */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ATimberBuildingBase>> ConstructionQueue;

	/** Con trỏ tới GridManager */
	UPROPERTY(Transient)
	TWeakObjectPtr<ATimberGridManager> GridManager;

	/** Bộ đếm tăng dần cấp phát PlacementOrderIndex cho từng móng (FIFO ID) */
	int64 GlobalPlacementCounter = 0;
};
