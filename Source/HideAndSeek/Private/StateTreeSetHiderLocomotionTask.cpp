#include "StateTreeSetHiderLocomotionTask.h"

#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderLocomotion, Log, All);

FStateTreeSetHiderLocomotionTask::FStateTreeSetHiderLocomotionTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;
}

EStateTreeRunStatus FStateTreeSetHiderLocomotionTask::EnterState(FStateTreeExecutionContext& Context,
                                                                 const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	AActor* Actor = Data.Actor;

	if (!IsValid(Actor))
	{
		UE_LOG(LogHiderLocomotion, Warning, TEXT("Set Hider Locomotion failed: Actor is invalid."));
		return EStateTreeRunStatus::Failed;
	}

	if (!Actor->GetClass()->ImplementsInterface(UHiderLocomotion::StaticClass()))
	{
		UE_LOG(LogHiderLocomotion, Warning, TEXT("%s does not implement HiderLocomotion."), *GetNameSafe(Actor));
		return EStateTreeRunStatus::Failed;
	}

	if (!IHiderLocomotion::Execute_SetLocomotionIntent(Actor, Data.Gait, Data.bCrouch))
	{
		UE_LOG(LogHiderLocomotion, Warning, TEXT("%s returned false from SetLocomotionIntent."), *GetNameSafe(Actor));
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogHiderLocomotion, Log, TEXT("%s: Gait = %s, Crouch = %s"),
	       *GetNameSafe(Actor),
	       *UEnum::GetValueAsString(Data.Gait),
	       Data.bCrouch ? TEXT("True") : TEXT("False"));

	return EStateTreeRunStatus::Succeeded;
}
