#include "StateTreeRoundPhaseCondition.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"

bool FStateTreeRoundPhaseCondition::TestCondition(
	FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	const AActor* Actor = Data.Actor.Get();
	const UWorld* World = IsValid(Actor) ? Actor->GetWorld() : nullptr;

	const AHideAndSeekGameMode* GameMode = World ? World->GetAuthGameMode<AHideAndSeekGameMode>() : nullptr;

	return IsValid(GameMode) && GameMode->GetRoundPhase() == Data.RequiredPhase;
}