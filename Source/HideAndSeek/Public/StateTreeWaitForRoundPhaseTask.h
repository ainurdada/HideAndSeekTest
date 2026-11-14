#pragma once

#include "CoreMinimal.h"
#include "HideAndSeekGameMode.h"
#include "StateTreeTaskBase.h"
#include "StateTreeWaitForRoundPhaseTask.generated.h"

class AActor;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeWaitForRoundPhaseInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EHideAndSeekRoundPhase RequiredPhase =
		EHideAndSeekRoundPhase::Hiding;

	UPROPERTY()
	TWeakObjectPtr<AHideAndSeekGameMode> GameMode;

	FDelegateHandle PhaseChangedHandle;
	bool bWaiting = false;
};

USTRUCT(meta = (
	DisplayName = "Wait For Round Phase",
	Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeWaitForRoundPhaseTask
	: public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeWaitForRoundPhaseInstanceData;

	FStateTreeWaitForRoundPhaseTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;
};
