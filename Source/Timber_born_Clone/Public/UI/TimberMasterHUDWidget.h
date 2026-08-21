#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimberMasterHUDWidget.generated.h"

/**
 * Widget Quản lý HUD Tổng Thể của Thành Phố Hải Ly (Phase 5 - Step 5.3)
 * Bao gồm Thanh Thống Kê Tài Nguyên ở Đỉnh (Top Bar) và Menu Xây Dựng ở Đáy (Bottom Bar)
 * Áp dụng SOLID: Tách riêng Data Binding Model độc lập, giao tiếp với PlayerController qua Event
 */
UCLASS()
class TIMBER_BORN_CLONE_API UTimberMasterHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ==========================================
	// DATA BINDING EVENTS (BLUEPRINT IMPLEMENTABLE)
	// ==========================================

	/**
	 * Sự kiện kích hoạt khi tổng số lượng Gỗ trong toàn bộ khu định cư thay đổi
	 * @param TotalWoodCount Tổng số gỗ hiện có trong Nhà Chính + Tất cả các Kho
	 * @param MaxWoodCapacity Tổng sức chứa kho tối đa của toàn khu định cư
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Timber|HUD Events", meta = (DisplayName = "On Resource Stocks Updated"))
	void OnResourceStocksUpdated(int32 TotalWoodCount, int32 MaxWoodCapacity);

	/**
	 * Sự kiện kích hoạt khi dân số Hải ly hoặc số lượng việc làm thay đổi
	 * @param TotalBeavers Tổng số lượng Hải ly hiện có
	 * @param EmployedBeavers Số Hải ly đang có việc làm tại các công trình
	 * @param MaxPopulationCapacity Giới hạn dân số tối đa
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Timber|HUD Events", meta = (DisplayName = "On Population Stats Updated"))
	void OnPopulationStatsUpdated(int32 TotalBeavers, int32 EmployedBeavers, int32 MaxPopulationCapacity);

	/**
	 * Sự kiện kích hoạt khi người chơi chuyển đổi công cụ cọ vẽ (Lát đường, Đặt móng, Phá hủy, None)
	 * @param ActiveBrushMode Chế độ cọ vẽ hiện tại (dùng để sáng viền nút bấm trên UI)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Timber|HUD Events", meta = (DisplayName = "On Active Tool Changed"))
	void OnActiveToolChanged(uint8 ActiveBrushMode);

	// ==========================================
	// C++ HELPER DISPATCHERS
	// ==========================================

	/** C++ gọi hàm này để đẩy dữ liệu xuống Blueprint UI Widget */
	void UpdateResourceDisplay(int32 TotalWood, int32 MaxCapacity)
	{
		OnResourceStocksUpdated(TotalWood, MaxCapacity);
	}

	void UpdatePopulationDisplay(int32 TotalBeavers, int32 Employed, int32 MaxCap)
	{
		OnPopulationStatsUpdated(TotalBeavers, Employed, MaxCap);
	}

	void UpdateToolDisplay(uint8 BrushMode)
	{
		OnActiveToolChanged(BrushMode);
	}
};
