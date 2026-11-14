#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeWatchPlayerTouchTask.generated.h"

class AActor;
class AHideAndSeekAIController;
class AHideAndSeekGameMode;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeWatchPlayerTouchInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.0f, ClampMax = 5.0f, Units = "cm"))
	float ContactTolerance = 2.0f;

	UPROPERTY()
	TWeakObjectPtr<AHideAndSeekAIController> Controller;

	UPROPERTY()
	TWeakObjectPtr<AHideAndSeekGameMode> GameMode;

	FDelegateHandle HitHandle;
	FDelegateHandle PhaseHandle;

	bool bEventSent = false;
};

USTRUCT(meta = (DisplayName = "Watch Player Touch", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeWatchPlayerTouchTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeWatchPlayerTouchInstanceData;

	FStateTreeWatchPlayerTouchTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;
};
