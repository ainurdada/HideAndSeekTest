#pragma once


#include "CoreMinimal.h"
#include "HiderLocomotion.h"
#include "StateTreeTaskBase.h"
#include "StateTreeSetHiderLocomotionTask.generated.h"

class AActor;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeSetHiderLocomotionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category="Parameter")
	EHiderGait Gait = EHiderGait::Run;

	UPROPERTY(EditAnywhere, Category="Parameter")
	bool bCrouch = false;
};

USTRUCT(meta=(DisplayName="Set Hider Locomotion", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeSetHiderLocomotionTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeSetHiderLocomotionInstanceData;

	FStateTreeSetHiderLocomotionTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;
};
