#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "StateTreeTaskBase.h"
#include "StateTreeWatchHidingCoverTask.generated.h"

class AActor;
class UWorld;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeWatchHidingCoverInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	FVector GoalLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.0f, Units = "cm"))
	float CoverMargin = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.05f, Units = "s"))
	float CheckInterval = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.0f, Units = "s"))
	float ConfirmTime = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bCoverLost = false;

	UPROPERTY()
	TWeakObjectPtr<UWorld> World;

	UPROPERTY()
	FTimerHandle CheckTimer;

	double FirstExposedTime = -1.0;
};

USTRUCT(meta = (DisplayName = "Watch Hiding Cover", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeWatchHidingCoverTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeWatchHidingCoverInstanceData;

	FStateTreeWatchHidingCoverTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;
};
