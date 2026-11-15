#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeWatchRevealedTask.generated.h"

class AActor;
class UHiderAbilitySystemComponent;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeWatchRevealedInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bHasExcludedLocation = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	FVector ExcludedLocation = FVector::ZeroVector;

	UPROPERTY()
	TWeakObjectPtr<UHiderAbilitySystemComponent> AbilitySystem;

	FDelegateHandle RevealAppliedHandle;
};

USTRUCT(meta = (DisplayName = "Watch Revealed", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeWatchRevealedTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeWatchRevealedInstanceData;

	FStateTreeWatchRevealedTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;
};
