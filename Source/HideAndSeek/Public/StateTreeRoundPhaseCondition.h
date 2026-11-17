#pragma once

#include "CoreMinimal.h"
#include "HideAndSeekGameMode.h"
#include "StateTreeConditionBase.h"
#include "StateTreeRoundPhaseCondition.generated.h"

class AActor;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeRoundPhaseConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EHideAndSeekRoundPhase RequiredPhase = EHideAndSeekRoundPhase::Hiding;
};

USTRUCT(meta = (DisplayName = "Round Phase Is", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeRoundPhaseCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeRoundPhaseConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
