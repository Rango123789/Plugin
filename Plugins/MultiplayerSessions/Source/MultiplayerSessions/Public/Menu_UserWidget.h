// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu_UserWidget.generated.h"

//forward-declare a class/enum within a namespace:
namespace EOnJoinSessionCompleteResult { enum Type; };
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu_UserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int NumOfPublicConnections = 4, FString TypeOfMatch = FString("FreeForAll"), FString LevelPath = TEXT("/Game/ThirdPerson/Maps/Lobby"));
	UFUNCTION(BlueprintCallable)
	void MenuTearDown();

protected:
	//auto-call many times when needed
	virtual bool Initialize() override;
	//trigger when a level is removed >< NativeContruct() trigger after a new level is created and...
	virtual void NativeDestruct() override;

	//widget delegate require its callback to be marked with UFUNCTION()
	UFUNCTION()
	void OnClicked_HostButton();
	UFUNCTION()
	void OnClicked_JoinButton();

	//
	// Callbacks for the custom delegates on the MSubsystem class
	//
	UFUNCTION()
	void OnCreateSessionComplete_Multiplayer(bool bWasSuccessful);

	//if you try to add UFUNCTION for them too - get error - it has parameter type implemented without UCLASS()++
	// why I didn't forward declare 'class FOnlineSessionSearchResult' but it still work?
	//this doesn't happen for 'EOnJoinSessionCompleteResult::Type' - i need to forward-declare it above (so dont worry about this case)
	void OnFindSessionsComplete_Multiplayer(bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults);
	void OnJoinSessionComplete_Multiplayer(EOnJoinSessionCompleteResult::Type JoinResult);

	UFUNCTION()
	void OnStartSessionComplete_Multiplayer(bool bWasSuccessful);
	UFUNCTION()
	void OnDestroySessionComplete_Multiplayer(bool bWasSuccessful);

private:
	//from now on: every data member is private, unless it is UActorComponent+, USceneComponent+
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	//should we choose UGameInstanceSubsystem* parent instead for GENERIC code?
	class UMultiplayerSession_GameSubsystem* MultiplayerSessionSubsystem;

	int NumPublicConnections{4}; //{4}
	FString MatchTypeValue{TEXT("FreeForAll")};   //{TEXT("FreeForAll")}
	FString LevelPath{ TEXT("") };

public:


};
