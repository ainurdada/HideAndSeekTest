#include "StateTreeOccupyHidingSlotTask.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HidingSpotBehaviorDefinition.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeExecutionContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderOccupySlot, Log, All);

FStateTreeOccupyHidingSlotTask::FStateTreeOccupyHidingSlotTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;
}

EStateTreeRunStatus FStateTreeOccupyHidingSlotTask::EnterState(FStateTreeExecutionContext& Context,
                                                               const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	AActor* Actor = Data.Actor.Get();

	if (!IsValid(Actor) || !Actor->GetWorld())
	{
		UE_LOG(LogHiderOccupySlot, Warning, TEXT("Occupy failed: Actor or World is invalid."));
		return EStateTreeRunStatus::Failed;
	}

	USmartObjectSubsystem* Subsystem = Actor->GetWorld()->GetSubsystem<USmartObjectSubsystem>();

	if (!Subsystem || !Data.ClaimHandle.IsValid() || !Subsystem->IsSmartObjectSlotValid(Data.ClaimHandle.SlotHandle))
	{
		UE_LOG(LogHiderOccupySlot, Warning, TEXT("%s: Occupy failed: subsystem, claim or slot is invalid."),
		       *GetNameSafe(Actor));
		return EStateTreeRunStatus::Failed;
	}

	const UHidingSpotBehaviorDefinition* Definition = 
		Subsystem->MarkSlotAsOccupied<UHidingSpotBehaviorDefinition>(Data.ClaimHandle);
	
	if (!Definition)
	{
		UE_LOG(LogHiderOccupySlot, Warning, TEXT("%s: Occupy rejected for %s."),
			*GetNameSafe(Actor),
			*LexToString(Data.ClaimHandle));
		return EStateTreeRunStatus::Failed;
	}
	
	UE_LOG(LogHiderOccupySlot, Log, TEXT("%s: Occupied %s"),
		*GetNameSafe(Actor),
		*LexToString(Data.ClaimHandle));

	return EStateTreeRunStatus::Succeeded;
}
