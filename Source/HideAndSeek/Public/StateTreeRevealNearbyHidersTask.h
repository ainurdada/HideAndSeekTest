#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeRevealNearbyHidersTask.generated.h"

class AActor;

USTRUCT()
struct HIDEANDSEEK_API FStateTreeRevealNearbyHidersInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	bool bFirstDiscovery = false;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 1.0f, Units = "cm"))
	float RevealRadius = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.1f, Units = "s"))
	float DebugDuration = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Output")
	int32 RevealedCount = 0;

	UPROPERTY(EditAnywhere, Category = "Output")
	int32 FailedCount = 0;
};

USTRUCT(meta = (DisplayName = "Reveal Nearby Hiders", Category = "HideAndSeek"))
struct HIDEANDSEEK_API FStateTreeRevealNearbyHidersTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeRevealNearbyHidersInstanceData;

	FStateTreeRevealNearbyHidersTask();

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
	                                       const FStateTreeTransitionResult& Transition) const override;
};
