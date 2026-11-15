#include "StateTreeApplyGameplayEffectTask.h"

#include "AbilitySystemComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "StateTreeExecutionContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderApplyEffect, Log, All);

FStateTreeApplyGameplayEffectTask::FStateTreeApplyGameplayEffectTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FStateTreeApplyGameplayEffectTask::EnterState(FStateTreeExecutionContext& Context,
                                                                  const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	AActor* Actor = Data.Actor.Get();

	if (!Data.bApply)
	{
		UE_LOG(LogHiderApplyEffect, Log, TEXT("%s: Apply Gameplay Effect skipped: Apply=false."),
		       *GetNameSafe(Actor));

		return EStateTreeRunStatus::Succeeded;
	}

	UAbilitySystemComponent* AbilitySystem = IsValid(Actor)
		                                         ? Actor->FindComponentByClass<UAbilitySystemComponent>()
		                                         : nullptr;

	if (!IsValid(AbilitySystem))
	{
		UE_LOG(LogHiderApplyEffect, Error, TEXT("%s: Apply Gameplay Effect requires an AbilitySystemComponent."),
		       *GetNameSafe(Actor));

		return EStateTreeRunStatus::Failed;
	}

	if (!Data.EffectClass || !FMath::IsFinite(Data.EffectLevel) || Data.EffectLevel <= 0.0f)
	{
		UE_LOG(LogHiderApplyEffect, Error, TEXT("%s: Requires an EffectClass and a finite positive EffectLevel."),
		       *GetNameSafe(Actor));

		return EStateTreeRunStatus::Failed;
	}

	const FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(
		Data.EffectClass,
		Data.EffectLevel,
		AbilitySystem->MakeEffectContext());

	if (!Spec.IsValid())
	{
		UE_LOG(LogHiderApplyEffect, Error, TEXT("%s: Failed to create spec for %s."),
		       *GetNameSafe(Actor),
		       *GetNameSafe(Data.EffectClass.Get()));

		return EStateTreeRunStatus::Failed;
	}

	const FActiveGameplayEffectHandle Handle = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	if (!Handle.WasSuccessfullyApplied())
	{
		UE_LOG(LogHiderApplyEffect, Error, TEXT("%s: Failed to apply %s."),
		       *GetNameSafe(Actor),
		       *GetNameSafe(Data.EffectClass.Get()));

		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogHiderApplyEffect, Log, TEXT("%s: Applied %s, Level=%.2f."),
	       *GetNameSafe(Actor),
	       *GetNameSafe(Data.EffectClass.Get()),
	       Data.EffectLevel);

	return EStateTreeRunStatus::Succeeded;
}
