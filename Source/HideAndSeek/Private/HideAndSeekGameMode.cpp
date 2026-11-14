// Fill out your copyright notice in the Description page of Project Settings.


#include "HideAndSeekGameMode.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogHideAndSeekRound, Log, All);

AHideAndSeekGameMode::AHideAndSeekGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHideAndSeekGameMode::StartPlay()
{
	Super::StartPlay();

	if (RoundPhase != EHideAndSeekRoundPhase::WaitingToStart)
	{
		return;
	}

	if (!FMath::IsFinite(HidingDuration) || HidingDuration <= 0.0f ||
		(bShowRoundDebug && (!FMath::IsFinite(DebugRefreshInterval) || DebugRefreshInterval <= 0.0f)))
	{
		UE_LOG(LogHideAndSeekRound, Error, TEXT("Round settings contain an invalid duration."));
		return;
	}

	GetWorldTimerManager().SetTimer(
		HidingTimer,
		this,
		&AHideAndSeekGameMode::StartSearching,
		HidingDuration,
		false);

	SetRoundPhase(EHideAndSeekRoundPhase::Hiding);

	if (bShowRoundDebug)
	{
		GetWorldTimerManager().SetTimer(
			DebugTimer,
			this,
			&AHideAndSeekGameMode::RefreshRoundDebug,
			DebugRefreshInterval,
			true);
	}
}

float AHideAndSeekGameMode::GetHidingTimeRemaining() const
{
	if (RoundPhase != EHideAndSeekRoundPhase::Hiding)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, GetWorldTimerManager().GetTimerRemaining(HidingTimer));
}

void AHideAndSeekGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(HidingTimer);
	GetWorldTimerManager().ClearTimer(DebugTimer);

	if (GEngine)
	{
		GEngine->RemoveOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()));
	}

	Super::EndPlay(EndPlayReason);
}

void AHideAndSeekGameMode::StartSearching()
{
	if (RoundPhase == EHideAndSeekRoundPhase::Hiding)
	{
		SetRoundPhase(EHideAndSeekRoundPhase::Searching);
	}
}

void AHideAndSeekGameMode::SetRoundPhase(EHideAndSeekRoundPhase NewPhase)
{
	if (RoundPhase == NewPhase)
	{
		return;
	}

	const EHideAndSeekRoundPhase PreviousPhase = RoundPhase;
	RoundPhase = NewPhase;

	UE_LOG(LogHideAndSeekRound, Log, TEXT("%s -> %s"), *UEnum::GetValueAsString(PreviousPhase),
	       *UEnum::GetValueAsString(RoundPhase));

	OnRoundPhaseChanged.Broadcast(PreviousPhase, RoundPhase);
	RefreshRoundDebug();
}

void AHideAndSeekGameMode::RefreshRoundDebug()
{
	if (!bShowRoundDebug || !GEngine)
	{
		return;
	}

	FString Message;
	FColor Color = FColor::White;

	switch (RoundPhase)
	{
	case EHideAndSeekRoundPhase::WaitingToStart:
		Message = TEXT("HNS | WaitingToStart");
		break;

	case EHideAndSeekRoundPhase::Hiding:
		Message = FString::Printf(TEXT("HNS | Hiding: %.1f s"), GetHidingTimeRemaining());
		Color = FColor::Yellow;
		break;

	case EHideAndSeekRoundPhase::Searching:
		Message = TEXT("HNS | Searching");
		Color = FColor::Green;
		break;
	}

	GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), DebugRefreshInterval * 2.0f,
	                                 Color, Message);
}
