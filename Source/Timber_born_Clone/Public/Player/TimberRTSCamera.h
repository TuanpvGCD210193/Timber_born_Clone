#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TimberRTSCamera.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ATimberGridManager;

/**
 * Camera Chiến Thuật RTS góc nhìn từ trên cao (Top-down Isometric)
 * Hỗ trợ Pan (WASD/Rê chuột), Orbit xoay quanh tâm (Q/E), Smooth Zoom (Cuộn chuột)
 * Áp dụng SOLID: Tách biệt hoàn toàn thị giác máy quay khỏi logic Player Controller
 */
UCLASS()
class TIMBER_BORN_CLONE_API ATimberRTSCamera : public APawn
{
	GENERATED_BODY()

public:
	ATimberRTSCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ==========================================
	// COMPONENTS
	// ==========================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComp;

	// ==========================================
	// TỐC ĐỘ & THÔNG SỐ CẤU HÌNH (CONFIG)
	// ==========================================

	/** Tốc độ trượt di chuyển camera trên mặt đất (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Movement")
	float PanSpeed = 2500.0f;

	/** Tốc độ xoay camera quanh tâm (độ/giây) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Movement")
	float OrbitSpeed = 90.0f;

	/** Tốc độ cuộn thu phóng khoảng cách Zoom */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Zoom")
	float ZoomSpeed = 600.0f;

	/** Khoảng cách Zoom gần nhất (Cận cảnh Hải ly) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Zoom")
	float MinZoomDistance = 800.0f;

	/** Khoảng cách Zoom xa nhất (Toàn cảnh Thung lũng) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Zoom")
	float MaxZoomDistance = 7000.0f;

	/** Độ mượt mà khi nội suy chuyển động (Càng cao càng nhanh, 5.0f - 10.0f là chuẩn mượt) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Smoothing")
	float SmoothingFactor = 8.0f;

	/** Bật/tắt tự động kẹp vị trí Camera nằm gọn trong biên bản đồ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Bounds")
	bool bClampToGridBounds = true;

	/** Độ nhạy khi rê chuột phải xoay góc nhìn như Editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Movement")
	float MouseLookSensitivity = 0.25f;

	/** Góc ngẩng tối đa lên cao (Pitch âm nhẹ) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Movement")
	float MinPitch = -80.0f;

	/** Góc cúi tối đa sát mặt đất */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Camera Movement")
	float MaxPitch = -15.0f;

	// ==========================================
	// CÁC HÀM XỬ LÝ INPUT (WASD, QE, MOUSE WHEEL)
	// ==========================================

	UFUNCTION(BlueprintCallable, Category = "Timber|Camera Actions")
	void MoveForwardInput(float Value);

	UFUNCTION(BlueprintCallable, Category = "Timber|Camera Actions")
	void MoveRightInput(float Value);

	UFUNCTION(BlueprintCallable, Category = "Timber|Camera Actions")
	void RotateYawInput(float Value);

	UFUNCTION(BlueprintCallable, Category = "Timber|Camera Actions")
	void ZoomIn();

	UFUNCTION(BlueprintCallable, Category = "Timber|Camera Actions")
	void ZoomOut();

	UFUNCTION(BlueprintCallable, Category = "Timber|Camera Actions")
	void RotateLeft();

	UFUNCTION(BlueprintCallable, Category = "Timber|Camera Actions")
	void RotateRight();

	/** Di chuyển dịch chuyển Camera tức thì theo vector kéo chuột (World Space Delta) */
	UFUNCTION(BlueprintCallable, Category = "Timber|Camera Actions")
	void AddPanDelta(const FVector& WorldDelta);

	/** Xoay góc nhìn Camera tự do như Editor Viewport (Yaw trái/phải, Pitch ngẩng/cúi) */
	UFUNCTION(BlueprintCallable, Category = "Timber|Camera Actions")
	void AddCameraRotationDelta(float YawDelta, float PitchDelta);

	/** Lấy góc xoay Yaw hiện tại của SpringArm */
	float GetCameraTargetYaw() const { return TargetYaw; }
	float GetCameraTargetArmLength() const { return TargetArmLength; }

protected:
	FVector TargetLocation;
	float TargetYaw = -45.0f;
	float TargetPitch = -45.0f;
	float TargetArmLength = 3000.0f;

	UPROPERTY(Transient)
	TObjectPtr<ATimberGridManager> GridManager;

	void UpdateCameraMovement(float DeltaTime);
	void ClampCameraLocation();
};
