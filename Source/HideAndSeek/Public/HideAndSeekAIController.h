// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HideAndSeekAIController.generated.h"

class UStateTreeAIComponent;

UCLASS()
class HIDEANDSEEK_API AHideAndSeekAIController : public AAIController
{
	GENERATED_BODY()

public:
	AHideAndSeekAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
	void TryStartStateTree();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
};
