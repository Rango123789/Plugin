// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
//auto-trigger when a player join the game, you dont need to call it at all
void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer == nullptr || NewPlayer->PlayerState == nullptr) return;

	if (GameState)
	{
		int NumOfPlayer = GameState->PlayerArray.Num();
		FString PlayerName1;
		FString PlayerName2;
		//I prefer this, it has 'Member' can just use this directly LOL
		PlayerName1 = NewPlayer->PlayerState->GetPlayerName() ;
		//if it is To, then you dont need <To> lol, this STEPHEN code:
		APlayerState* PlayerState = NewPlayer->GetPlayerState<APlayerState>(); //pass in To is redudant
		if(PlayerState) PlayerName2 = PlayerState->GetPlayerName();

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow
		//	, FString::Printf(TEXT("Num of Players via GameState_mem : % d"), GameState->PlayerArray.Num()) );

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow
			, FString::Printf(TEXT("Num of Players via direct_function : % d"), GetNumPlayers()));

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow
			, FString::Printf(TEXT("the player '%s' has JOIN the game_direct access"), *PlayerName1 ));
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow
			//, FString::Printf(TEXT("the player '%s' has JOIN the game_GetPlayerState<T>()"), *PlayerName2));

	}
}

//auto-trigger when a player leave the game, you dont need to call it at all
void ALobbyGameMode::Logout(AController* Exiting)
{
	if (Exiting == nullptr || Exiting->PlayerState == nullptr) return;

	if (GameState)
	{
		//minus here or where you print it is both OKay:
		int NumOfPlayer = GameState->PlayerArray.Num();

		FString PlayerName;
		//I prefer this, it has 'Member' can just use this directly LOL
		PlayerName = Exiting->PlayerState->GetPlayerName();

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow
			, FString::Printf(TEXT("Num of Players via GameState_mem : % d"), GameState->PlayerArray.Num() - 1 ));

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow
			//, FString::Printf(TEXT("Num of Players via direct_function : % d"), GetNumPlayers() - 1));

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow
			, FString::Printf(TEXT("the player '%s' has LEAVE the game_direct access"), *PlayerName));
	}
}
