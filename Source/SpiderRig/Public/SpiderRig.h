#pragma once

#include "ControlRig.h"
#include "CCDIK.h"
#include "FSpiderLegDef.h"
#include "FSpiderSpineDef.h"
#include "Engine/SpringInterpolator.h"
#include "SpiderRig.generated.h"

#define MAX_SPIDER_LEG_LENGTH 8
#define MAX_SPIDER_LEG_BONE_LENGTH 8

class USpiderEffectsComponent;
class ACharacter;
class UCurveFloat;
class UCharacterMovementComponent;

UCLASS(Blueprintable)
class SPIDERRIG_API USpiderRig : public UControlRig
{
	GENERATED_BODY()

private:
	bool InitializeLegs();
	bool InitializeSpine();
	bool InitializeVariables();
	//
	void SetLegLocation(const int32& LegIndex, const FVector& NewLegLocationGlobal, const float& Dt);
	void SetSpineTransform(const FVector& SpineLocationGlobal, const FRotator& RotationGlobal, const float& Dt);

	FORCEINLINE FVector RotateWorldToGlobal(const FVector& LocationWorld) const
	{
		return ParentSceneComponent->GetComponentRotation().UnrotateVector(LocationWorld);
	}

	FORCEINLINE FVector RotateGlobalToWorld(const FVector& LocationGlobal) const
	{
		return ParentSceneComponent->GetComponentRotation().RotateVector(LocationGlobal);
	}

	FORCEINLINE FVector TransformGlobalToWorld(const FVector& LocationGlobal) const
	{
		return ParentSceneComponent->GetComponentTransform().TransformPosition(LocationGlobal);
	}

	FORCEINLINE FVector TransformWorldToGlobal(const FVector& LocationWorld) const
	{
		return ParentSceneComponent->GetComponentTransform().InverseTransformPosition(LocationWorld);
	}

	bool TraceSingleLeg(
		FVector& LegLocationWorld,
		const FVector& RootLocationWorld,
		const FVector& UpVectorWorld
	) const;

protected:
	virtual bool Execute(const FName& InEventName) override;
	virtual void Initialize(bool bRequestInit) override;

private:
	// whether the bones are correctly configured or not
	bool bIsReady = false;

	// cache rig indices for faster lookup
	int32 SpineIndex = -1;
	int32 LegIndices[MAX_SPIDER_LEG_LENGTH][MAX_SPIDER_LEG_BONE_LENGTH + 1];
	int32 LegLength = 0;


	// falling related properties
	bool bIsFalling = false;
	float JumpImpact = 0.0f;
	float JumpZStart = 0.0f;
	bool bIsFallStarted = false;


	// time related properties
	float PrevFrame = 0;


	// movement related properties
	float LastMovementTimestamp = 0;
	float LaggedHorizontalSpeed = 0;
	float MotorValue = 0;

	// spine related properties
	FVector InitialSpineLocationGlobal{0};
	FVectorRK4SpringInterpolator SpineSpringInterpolator;
	FRotator FinalSpineRotation{0};

	// legs related properties
	TArray<float> RotationLimitsPerItem;
	TArray<FCCDIKChainLink> TemporaryChain;
	TArray<FVector> LegLocationsWorld;
	TArray<bool> LegState;
	TArray<FVector> FinalLegLocationsGlobal;


	// pre-initialized properties
	AActor* ParentActor = nullptr;
	ACharacter* ParentCharacter = nullptr;
	UCharacterMovementComponent* CharacterMovementComponent = nullptr;
	URigHierarchy* RigHierarchy = nullptr;
	USpiderEffectsComponent* SpiderEffects;

	// runtime initialized properties
	bool bIsInitialized = false;
	USceneComponent* ParentSceneComponent = nullptr;
	UWorld* LivingWorld = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Legs"), Category = "Rig Config")
	TArray<FSpiderLegDef> Legs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Spine"), Category = "Rig Config")
	FSpiderSpineDef Spine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Toe Offset Timeline"), Category = "Movement")
	TObjectPtr<UCurveFloat> ToeOffsetTimeline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Toe Stick To Ground Timeline"), Category = "Movement")
	TObjectPtr<UCurveFloat> ToeStickGroundTimeline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Minimum Leg Spread"), Category = "Falling")
	float FallingMinLegSpread = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Maximum Leg Spread"), Category = "Falling")
	float FallingMaxLegSpread = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Minimum Leg Offset Z"), Category = "Falling")
	float FallingMinLegOffset = -20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Maxmimum Leg Offset Z"), Category = "Falling")
	float FallingMaxLegOffset = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Step Height"), Category = "Movement")
	float StepHeight = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Spring Lag"), Category = "Spine")
	float SpineSpringLag = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Rotation Lag"), Category = "Spine")
	float SpineRotationLag = 20.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Spring Stiffness"), Category = "Spine")
	float SpineSpringStiffness = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Spring Damping Ratio"), Category = "Spine")
	float SpineSpringDampingRatio = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Throttle Multiplier"), Category = "Movement")
	float ThrottleMultiplier = 3.01f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Leg Movement Lag"), Category = "Movement")
	float LegMovementLag = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Lazy Lag"), Category = "Movement")
	float LazyLag = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Lazy Stall Lag Multiplier"), Category = "Movement")
	float LazyStallLagMultiplier = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Movement Transition Duration"), Category = "Movement")
	float MovementTransitionDuration = 0.25f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Animation Off-Cycle Coefficient"), Category = "Movement")
	float AnimationOffCycleCoefficient = 0.45f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Toe Inward Distance"), Category = "Traces")
	float ToeTraceDepthInward = 20.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Toe Outward Distance"), Category = "Traces")
	float ToeTraceDepthOutward = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Origin Upward Distance"), Category = "Traces")
	float ToeTraceOriginUpward = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Toe Trace Radius"), Category = "Traces")
	float ToeTraceRadius = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Toe Placement Lag"), Category = "Movement")
	float ToePlacementLagSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "IK Precision"), Category = "Rig Config")
	float IKPrecision = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "IK Iteration"), Category = "Rig Config")
	int32 IKSolveIteration = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Toe Lag"), Category = "Falling")
	float ToeFallingLag = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Rotation To Z-Speed Coefficient"), Category = "Falling")
	float FallingRotationZSpeedCoefficient = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Rotation Limit"), Category = "Falling")
	float FallingRotationLimit = 30.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Maximum Leg Offset Horizontal"), Category = "Falling")
	float FallingLegOffsetHorizontalLimit = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Minimum Leg Spread To Z-Speed Coefficient"), Category = "Falling")
	float FallingLegSpreadSpeedCoefficient = 0.01f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Maximum Leg Spread To Z-Speed Coefficient"), Category = "Falling")
	float FallingLegLocationCoefficient = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(DisplayName = "Fall Impact On Spine"), Category = "Falling")
	float FallingImpactOnSpine = 30.0f;
};
