#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeApplyGameplayEffectTask.generated.h"

class AActor;
class UGameplayEffect;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeApplyGameplayEffectInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	bool bApply = true;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.001f))
	float EffectLevel = 1.0f;
};

USTRUCT(meta = (DisplayName = "Apply Gameplay Effect", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeApplyGameplayEffectTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeApplyGameplayEffectInstanceData;

	FStateTreeApplyGameplayEffectTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;
};
