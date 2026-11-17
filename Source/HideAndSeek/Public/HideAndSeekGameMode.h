// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/GameModeBase.h"
#include "HideAndSeekGameMode.generated.h"

UENUM(BlueprintType)
enum class EHideAndSeekRoundPhase : uint8
{
	WaitingToStart,
	Hiding,
	Searching,
	Finished
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHideAndSeekRoundPhaseChanged, EHideAndSeekRoundPhase, EHideAndSeekRoundPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHideAndSeekRoundPresentationChanged);

UCLASS()
class HIDEANDSEEK_API AHideAndSeekGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHideAndSeekGameMode();

	virtual void StartPlay() override;

	UFUNCTION(BlueprintPure, Category = "HideAndSeek|Round")
	EHideAndSeekRoundPhase GetRoundPhase() const
	{
		return RoundPhase;
	}

	UFUNCTION(BlueprintPure, Category = "HideAndSeek|Round")
	float GetHidingTimeRemaining() const;

	bool NotifyHiderFound(AActor* Hider);

	UFUNCTION(BlueprintPure, Category = "HideAndSeek|Round")
	int32 GetTotalHiderCount() const
	{
		return RoundHiders.Num();
	}

	UFUNCTION(BlueprintPure, Category = "HideAndSeek|Round")
	int32 GetFoundHiderCount() const
	{
		return FoundHiders.Num();
	}

	UFUNCTION(BlueprintPure, Category = "HideAndSeek|Round")
	bool ShouldLockPlayerWhileHiding() const
	{
		return bShouldLockPlayerWhileHiding;
	}
	
	UFUNCTION(BlueprintCallable, Category = "HideAndSeek|Round")
	void RequestRestartRound();

	FOnHideAndSeekRoundPhaseChanged OnRoundPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "HideAndSeek|Round")
	FOnHideAndSeekRoundPresentationChanged OnRoundPresentationChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "HideAndSeek|Round", meta=(ClampMin = 0.1f, Units = "s"))
	float HidingDuration = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideAndSeek|Debug")
	bool bShowRoundDebug = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideAndSeek|Debug",
		meta = (ClampMin = 0.05f, Units = "s"))
	float DebugRefreshInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideAndSeek|Round")
	bool bShouldLockPlayerWhileHiding = true;

private:
	void StartSearching();
	void SetRoundPhase(EHideAndSeekRoundPhase NewPhase);
	void RefreshRoundDebug();

	bool CollectRoundHiders();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "HideAndSeek|Round", meta=(AllowPrivateAccess="true"))
	EHideAndSeekRoundPhase RoundPhase = EHideAndSeekRoundPhase::WaitingToStart;

	FTimerHandle HidingTimer;
	FTimerHandle DebugTimer;

	TSet<TWeakObjectPtr<AActor>> RoundHiders;
	TSet<TWeakObjectPtr<AActor>> FoundHiders;

	bool bRestartRequested = false;
};
