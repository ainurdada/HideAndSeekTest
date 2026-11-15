#pragma once

#include "CoreMinimal.h"
#include "SmartObjectRuntime.h"
#include "StateTreeTaskBase.h"
#include "StateTreeClaimHidingSlotTask.generated.h"

class AActor;
class USmartObjectSubsystem;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeClaimHidingSlotInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	bool bHasExcludedLocation = false;

	UPROPERTY(EditAnywhere, Category = "Input")
	FVector ExcludedLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 1.0f, Units = "cm"))
	float MinDistanceFromExcludedLocation = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1.0", Units = "cm"))
	float SearchRadius = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxGoalOffsetXY = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxGoalOffsetZ = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", Units = "cm"))
	float CoverMargin = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", Units = "s"))
	float VisibilityDebugDuration = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Output")
	FVector GoalLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Output")
	FSmartObjectClaimHandle ClaimHandle;

	UPROPERTY()
	TWeakObjectPtr<USmartObjectSubsystem> Subsystem;
};

USTRUCT(meta=(DisplayName= "Claim Hiding Slot", Category= "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeClaimHidingSlotTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeClaimHidingSlotInstanceData;

	FStateTreeClaimHidingSlotTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;
};
