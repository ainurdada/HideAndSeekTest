#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "StateTreeTaskBase.h"
#include "StateTreeWatchPlayerSightTask.generated.h"

class AActor;
class UWorld;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeWatchPlayerSightInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 1.0f, Units = "cm"))
	float SightRange = 350.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.02f, Units = "s"))
	float CheckInterval = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.0f, Units = "s"))
	float ConfirmTime = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bDrawDebug = false;

	UPROPERTY()
	TWeakObjectPtr<UWorld> World;

	UPROPERTY()
	FTimerHandle CheckTimer;

	double FirstVisibleTime = -1.0;
	bool bEventSent = false;
};

USTRUCT(meta = (DisplayName = "Watch Player Sight", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeWatchPlayerSightTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeWatchPlayerSightInstanceData;

	FStateTreeWatchPlayerSightTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;
};
