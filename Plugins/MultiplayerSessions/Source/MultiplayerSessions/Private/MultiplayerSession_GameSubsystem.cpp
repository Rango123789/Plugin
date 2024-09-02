// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSession_GameSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

UMultiplayerSession_GameSubsystem::UMultiplayerSession_GameSubsystem() : 
	OnCreateSessionCompleteDelegate( FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	OnFindSessionsCompleteDelegate ( FOnFindSessionsCompleteDelegate ::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	OnJoinSessionCompleteDelegate  ( FOnJoinSessionCompleteDelegate  ::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	OnStartSessionCompleteDelegate ( FOnStartSessionCompleteDelegate ::CreateUObject(this, &ThisClass::OnStartSessionComplete)),
	OnDestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete))
{

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red
		, FString::Printf(TEXT("Found system- OUTER")));

	if (OnlineSubsystem)
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

		//you've already print this from outside, let's see what we got if we also print it here
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red
			, FString::Printf(TEXT("Found system- inside pluggin: %s"), *OnlineSubsystem->GetSubsystemName().ToString()));
	}

	//temporary place, before they one gradually one by one:
		//OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate) ; //MOVE
		//OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate) ; //MOVE
		//OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate) ;
		//OnlineSessionInterface->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegate) ;
		//OnlineSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate) ;


}

//Create:
void UMultiplayerSession_GameSubsystem::CreateSession(const int& NumPublicConnections, const FString& MatchTypeValue)
{
//formality step:
	if (OnlineSessionInterface.IsValid() == false) 
	{
		OnCreateSessionCompleteDelegate_Multiplayer.Broadcast(false); //at least let CUSTOM_callback_X know in worst case :)
		return; //but a side prolem is you dont know whether false by this step or false in the botton of this HOST_X() lol
	} 	

	CreateSession_Handle = OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

//main steps:
	if (OnlineSessionInterface->GetNamedSession(NAME_GameSession))
	{
		//give it one chance to call HOST_CreateSession() right inside DestroySession() recursively
		bDoOnce = true; 
		//the reason why we do it here is that, if there is no session at first, we dont even need them in MSubsystem
		LastNumPublicConnections = NumPublicConnections;
		LastMatchTypeValue = MatchTypeValue;
		//this is the KEY to solve the problem, call HOST_DestroySession():
		DestroySession();
	}
	else
	{
		ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController(); //	GetGameInstance()->GetFirstGamePlayer()
		//FUniqueNetId NetId;  //NOT work, because FUniqueNetId is abstract class
		//const FUniqueNetId* NetId = & (*LocalPlayer->GetPreferredUniqueNetId()) ; //WORK with 'const', this way we pass '&NetId' for the function
		const FUniqueNetId& NetId = *LocalPlayer->GetPreferredUniqueNetId(); //THIS WORK like a charm with 'const' - I talked about this already

		//the function only require Literal object, but dont know why Stephen make a shared pointer of it
		//that we also have to dereference it as we pass it LOL
		LastSessionSettings = MakeShareable(new FOnlineSessionSettings());

		//IOnlineSubsystem::Get()->GetSubsystemName() = return 'FName SubststemName', put FName is wise
		//even if you dont, it still work due to implicit concversion on 'comparison operators'
		LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == FName("NULL") ? true : false;
	
		LastSessionSettings->NumPublicConnections = NumPublicConnections ;
		LastSessionSettings->bShouldAdvertise = true;
		LastSessionSettings->bAllowJoinInProgress = true;
		LastSessionSettings->bAllowJoinViaPresence = true;
		LastSessionSettings->bUsesPresence = true;

		//this help me fix LAN work, Steam not work; without this switch Region sometime works
		LastSessionSettings->bUseLobbiesIfAvailable = true; 
		//Use to keep different Builds from seeing each other during searches: 
		LastSessionSettings->BuildUniqueId = 1;
		//this is also one of the most important sesstings for SessionSettings:
		LastSessionSettings->Set(FName("MatchType") , MatchTypeValue , EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		if (!LastSessionSettings.IsValid()) return; //mostly, SharedPointer = MakeShareable() should be valid, also we just create it above lol

		if (!OnlineSessionInterface->CreateSession(NetId, NAME_GameSession, *LastSessionSettings)) //call and check result atst
		{
			OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSession_Handle);

			OnCreateSessionCompleteDelegate_Multiplayer.Broadcast(false);
		}
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString::Printf(TEXT("LAN: %d"), LastSessionSettings->bIsLANMatch), false);
	}
}

void UMultiplayerSession_GameSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (OnlineSessionInterface.IsValid() == false) return;

	//after we finish creating session successfully and auto-trigger 'THIS callback', we use this callback to remove it from the list
	OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSession_Handle);

	//I know that if it reach this function body, then it must be true (otherwise it is unlinked above already), but better careful:
	OnCreateSessionCompleteDelegate_Multiplayer.Broadcast(bWasSuccessful);

	//now I will move this into the Menu class instead, perhaps to keep this class even more GENERIC?
	 //if (GetWorld()) GetWorld()->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"));
}

//Find:
void UMultiplayerSession_GameSubsystem::FindSessions(const int& MaxSearchResults)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString("Start of HOST_FindSession()"), false);
//formality steps:
	if (OnlineSessionInterface.IsValid() == false) 
	{
		OnFindSessionsCompleteDelegate_Multiplayer.Broadcast(false, TArray<FOnlineSessionSearchResult>()); 
		return; 
	}

	FindSessions_Handle = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate) ; 

//main steps:
	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	const FUniqueNetId& NetId = *LocalPlayer->GetPreferredUniqueNetId() ;

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == FName("NULL") ? true : false ;
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	//do not confuse with LastSessionSettings->Set(FName("MatchType") , FString("FreeForAll") , ...) 
	// - also set a pair but we did it for different purpose 
	//here we decide which SEARCH_[MODE] is , where the 'value' I dont unserstand yet.
	// - we only compare FString("FreeForAll") after we get SearchResults via ->the FindSessions()
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE , true , EOnlineComparisonOp::Equals);

	if (OnlineSessionInterface->FindSessions(NetId, LastSessionSearch.ToSharedRef()) == false) // || LastSessionSearch->SearchResults.Num() <= 0
	{
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessions_Handle);

		//in case fail or find no session, you can pass in either: 'LastSessionSearch->SearchResults' or 'empty array'_RECOMMENDED
		//if it fail it can never reach next callback no matter what fake or real array to be passed in LOL, so dont worry
		OnFindSessionsCompleteDelegate_Multiplayer.Broadcast(false, TArray<FOnlineSessionSearchResult>() );
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString("End of HOST_FindSession()"), false);
}

void UMultiplayerSession_GameSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString("Start of OnFind__callback2()"), false);

	if (OnlineSessionInterface.IsValid() == false) return;

	OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessions_Handle);

	//in stead of doing this, you can directly add '|| LastSessionSearch->SearchResults.Num() <= 0' in HOST_X! LOL
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		OnFindSessionsCompleteDelegate_Multiplayer.Broadcast(false, TArray<FOnlineSessionSearchResult>());
		return; //to avoid it reach the line below broadcast twice LOL
	}

	OnFindSessionsCompleteDelegate_Multiplayer.Broadcast(bWasSuccessful, LastSessionSearch->SearchResults);


	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString("End of OnFind__callback2()"), false);
}

//Join:
void UMultiplayerSession_GameSubsystem::JoinSession(const FOnlineSessionSearchResult& SearchResult)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString("Start of HOST_JoinSession()"), false);
//formality steps:
	if (OnlineSessionInterface.IsValid() == false) 
	{
		OnJoinSessionCompleteDelegate_Multiplayer.Broadcast(EOnJoinSessionCompleteResult::UnknownError); //at least let CUSTOM_callback_X know in worst case :)
		return; //but a side prolem is you dont know whether false by this step or false in the botton of this HOST_X() lol
	}

	JoinSession_Handle = OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

//main steps:
	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	if (OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult) == false)
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSession_Handle);

		OnJoinSessionCompleteDelegate_Multiplayer.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString("End of HOST_JoinSession()"), false);
}

void UMultiplayerSession_GameSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type JoinResult)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString("Start of OnJoin__callback3()"), false);
	//this time we dont care what the result is just remove the DO from the list
	//and then broadcast the exact Enum value to be handled from CUSTOM callback3:
	if (OnlineSessionInterface.IsValid() == false) return;

	OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSession_Handle);

	OnJoinSessionCompleteDelegate_Multiplayer.Broadcast(JoinResult);

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString("End of OnJoin__callback3()"), false);
}


//Start: NO need until very late sessions
void UMultiplayerSession_GameSubsystem::StartSession()
{
}

void UMultiplayerSession_GameSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}

//Destroy: NO need until very late sessions
void UMultiplayerSession_GameSubsystem::DestroySession()
{
//formality steps:
	if (OnlineSessionInterface.IsValid() == false) 
	{
		OnDestroySessionCompleteDelegate_Multiplayer.Broadcast(false); //at least let CUSTOM_callback_X know in worst case :)
		return; //but a side prolem is you dont know whether false by this step or false in the botton of this HOST_X() lol
	}

	DestroySession_Handle = OnlineSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

//main steps:
	if (OnlineSessionInterface->DestroySession(NAME_GameSession) == false)
	{
		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySession_Handle);

		OnDestroySessionCompleteDelegate_Multiplayer.Broadcast(false);
	}
}

void UMultiplayerSession_GameSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (OnlineSessionInterface.IsValid() == false) return;

	OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySession_Handle);

	//do extra things, that other callback_X doesn't have:
	if (bWasSuccessful == true && bDoOnce == true) 
	{
		bDoOnce = false; //next time wont do it again
		CreateSession(LastNumPublicConnections, LastMatchTypeValue);
	}

	//back to track, we dont need this CUSTOM_callback_X yet, but still just broadcast it!
	OnDestroySessionCompleteDelegate_Multiplayer.Broadcast(bWasSuccessful);
}
