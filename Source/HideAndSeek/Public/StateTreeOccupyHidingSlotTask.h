#pragma once

#include "CoreMinimal.h"
#include "SmartObjectRuntime.h"
#include "StateTreeTaskBase.h"
#include "StateTreeOccupyHidingSlotTask.generated.h"

class AActor;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeOccupyHidingSlotInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	FSmartObjectClaimHandle ClaimHandle;
};

USTRUCT(meta = (DisplayName = "Occupy Hiding Slot", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeOccupyHidingSlotTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeOccupyHidingSlotInstanceData;

	FStateTreeOccupyHidingSlotTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;
};
