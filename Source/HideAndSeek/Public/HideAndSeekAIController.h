// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Engine/HitResult.h"
#include "HideAndSeekAIController.generated.h"

class UStateTreeAIComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHideAndSeekPawnHit, AActor*);

UCLASS()
class HIDEANDSEEK_API AHideAndSeekAIController : public AAIController
{
	GENERATED_BODY()

public:
	AHideAndSeekAIController();

	FOnHideAndSeekPawnHit OnControlledPawnHit;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void TryStartStateTree();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

private:
	UFUNCTION()
	void HandleControlledPawnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	void UnbindPawnHit();

	UPROPERTY()
	TWeakObjectPtr<APawn> ObservedPawn;
};
