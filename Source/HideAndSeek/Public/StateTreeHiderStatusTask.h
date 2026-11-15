#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "StateTreeHiderStatusTask.generated.h"

class AActor;
class UHiderAbilitySystemComponent;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeHiderStatusInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "HideAndSeek.Status"))
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bStatusChanged = false;

	UPROPERTY()
	TWeakObjectPtr<UHiderAbilitySystemComponent> AbilitySystem;
};

USTRUCT(meta = (DisplayName = "Hider Status", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeHiderStatusTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeHiderStatusInstanceData;

	FStateTreeHiderStatusTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult& Transition) const override;
};
