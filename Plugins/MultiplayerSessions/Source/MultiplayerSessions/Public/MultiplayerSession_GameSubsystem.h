// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSession_GameSubsystem.generated.h"

//
// Declaring our own custom delegates for Menu class to bind its callbacks to 
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateSessionCompleteDelegate_Multiplayer, bool, bWasSuccessful);

//FOnlineSessionSearchResult is not with UCLASS()
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFindSessionsCompleteDelegate_Multiplayer, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults); //, bWasSuccessful , SearchResults

//FOnlineSessionSearchResult::Type is not with UENUM()
DECLARE_MULTICAST_DELEGATE_OneParam(FOnJoinSessionCompleteDelegate_Multiplayer, EOnJoinSessionCompleteResult::Type JoinResult);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartSessionCompleteDelegate_Multiplayer, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDestroySessionCompleteDelegate_Multiplayer, bool, bWasSuccessful);


/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSession_GameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UMultiplayerSession_GameSubsystem();
public:
	//
	// To handle session functionality. Menu [widget] class will call these
	//
	void CreateSession(const int& NumPublicConnections = 4, const FString& MatchTypeValue = FString("FreeForAll") );
	void FindSessions(const int& MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SearchResult);

	void StartSession();
	void DestroySession();

	//
	// Our own custom delegates for Menu class to bind its callbacks to 
	//
	FOnCreateSessionCompleteDelegate_Multiplayer OnCreateSessionCompleteDelegate_Multiplayer;
	FOnFindSessionsCompleteDelegate_Multiplayer OnFindSessionsCompleteDelegate_Multiplayer;
	FOnJoinSessionCompleteDelegate_Multiplayer OnJoinSessionCompleteDelegate_Multiplayer;
	FOnStartSessionCompleteDelegate_Multiplayer OnStartSessionCompleteDelegate_Multiplayer;
	FOnDestroySessionCompleteDelegate_Multiplayer OnDestroySessionCompleteDelegate_Multiplayer;



protected:
	//
	// Internal callbacks for delegate objects. Will be added to the Online Session Interface delegate LIST.
	// These dont need to be called outside this class.
	//
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type JoinResult);

	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr OnlineSessionInterface;

	TSharedPtr<FOnlineSessionSettings> LastSessionSettings; //we didn't make it member in testing version
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FDelegateHandle CreateSession_Handle;

	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FDelegateHandle FindSessions_Handle;
	
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FDelegateHandle JoinSession_Handle;
	
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
	FDelegateHandle StartSession_Handle;

	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	FDelegateHandle DestroySession_Handle;

	int LastNumPublicConnections; //{4}
	FString LastMatchTypeValue;   //{TEXT("FreeForAll")}
	bool bDoOnce = false; //when there is ExistingSession we set it into true and allow to CreateSession() without HOST_DestroySession(). Stephen names it 'bCreateSessionOnDestroy'

public:
	IOnlineSessionPtr GetOnlineSessionInterface() { return OnlineSessionInterface; }
};
