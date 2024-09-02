// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu_UserWidget.h"
#include "MultiplayerSession_GameSubsystem.h"
#include "Components/Button.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

//CreateWidget() will be called in Level BP right before it calls this
void UMenu_UserWidget::MenuSetup(int NumOfPublicConnections, FString TypeOfMatch, FString PathToLevel)
{
	//optional part
	NumPublicConnections = NumOfPublicConnections;
	MatchTypeValue = TypeOfMatch;

	this->LevelPath = PathToLevel + FString(TEXT("?listen"));

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, this->LevelPath);
	
	//now you want the Widget to add to Screen/Viewport
	AddToViewport(); 
	//widget-relevant setup_TIRE0 : you can also do it directly in WBP_child[s]
	//, but now you set it here right in C++ parent so that it will be the defaults for any of the WBP_child[s]
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

    //input-mode relevant: now we want UIOnly so that we can interfact with widget buttons++
	if (UWorld* World = GetWorld(); World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeUIOnly;
			InputModeUIOnly.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			//we want it to focus on 'this' widget lol (the current widget we're in LOL)
			//but the function require TSharedPtr<SWidget> , rather than normal pointer
			//at the same time, the widget class has 'TakeWidget()' return a shared point to this widget
			//so that will satisfy the required paramter!
			InputModeUIOnly.SetWidgetToFocus( TakeWidget() );

			PlayerController->SetInputMode(InputModeUIOnly);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	//better bind Menu::callback to MSubsystem::Delegate before the KEY function is called:
	if (MultiplayerSessionSubsystem)
	{
		MultiplayerSessionSubsystem->OnCreateSessionCompleteDelegate_Multiplayer.AddDynamic(this, &ThisClass::OnCreateSessionComplete_Multiplayer);
		MultiplayerSessionSubsystem->OnFindSessionsCompleteDelegate_Multiplayer.AddUObject(this, &ThisClass::OnFindSessionsComplete_Multiplayer);
		MultiplayerSessionSubsystem->OnJoinSessionCompleteDelegate_Multiplayer.AddUObject(this, &ThisClass::OnJoinSessionComplete_Multiplayer);
		MultiplayerSessionSubsystem->OnStartSessionCompleteDelegate_Multiplayer.AddDynamic(this, &ThisClass::OnStartSessionComplete_Multiplayer);
		MultiplayerSessionSubsystem->OnDestroySessionCompleteDelegate_Multiplayer.AddDynamic(this, &ThisClass::OnDestroySessionComplete_Multiplayer);
	}
}

void UMenu_UserWidget::MenuTearDown()
{
	//when travel to new level, we surely want the widget to dissappear from Screen/Viewport :D 
	RemoveFromParent();

	//input-mode relevant:	we also want to switch in GameOnly mode too
	if ( GetWorld())
	{
		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		if (PlayerController)
		{
			//NO LONGER relevant:
			//FInputModeUIOnly InputModeUIOnly;
				//InputModeUIOnly.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				//InputModeUIOnly.SetWidgetToFocus(TakeWidget());
			//new Inputmode:
			FInputModeGameOnly InputModeGameOnly;
				//InputModeGameOnly.SetConsumeCaptureMouseDown(true); //better dont touch this as stephen does :D 

			PlayerController->SetInputMode(InputModeGameOnly);
			PlayerController->SetShowMouseCursor(false); //now we dont want to see cursor
		}
	}
}

bool UMenu_UserWidget::Initialize()
{
	if (Super::Initialize() == false) return false;
	
	if(GetGameInstance()) MultiplayerSessionSubsystem = GetGameInstance()->GetSubsystem<UMultiplayerSession_GameSubsystem>();

	HostButton->OnClicked.AddDynamic(this, &ThisClass::OnClicked_HostButton);
	JoinButton->OnClicked.AddDynamic(this, &ThisClass::OnClicked_JoinButton);

	return true;
}

void UMenu_UserWidget::NativeDestruct()
{
	MenuTearDown();

	Super::NativeDestruct();
}

//HOST:
void UMenu_UserWidget::OnClicked_HostButton()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue, FString(TEXT("I click the HOST Button"))) ;

	if (MultiplayerSessionSubsystem)
	{
		//disable the button inside here is better! because no in disabling it while ->CreateSession is never triggered!
		HostButton->SetIsEnabled(false);  
		//HostButton->bIsEnabled = false; //also work!
		//you're calling the HOSTING function, that will in turn call SessionInterface->CreateSession() from the external class LOCALLY
		//we dont care what it hapens behind the local class, we implement the local class indepdently, so yeah!
		MultiplayerSessionSubsystem->CreateSession(NumPublicConnections , MatchTypeValue );
	}
}

void UMenu_UserWidget::OnCreateSessionComplete_Multiplayer(bool bWasSuccessful)
{
	if (bWasSuccessful == true)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue
			, FString(TEXT("Session created! Menu callback responses to MSubsystem!")));
		
		if (GetWorld()) GetWorld()->ServerTravel(LevelPath);
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue
		, FString(TEXT("Session FAIL to create! Menu callback responses to MSubsystem!")));

		//when it fails to create session, then it is good to enable it for next try
		HostButton->bIsEnabled = true; 
	}
}

//JOIN:
void UMenu_UserWidget::OnClicked_JoinButton()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue, FString(TEXT("I click the JOIN Button")));

	if (MultiplayerSessionSubsystem)
	{
		JoinButton->bIsEnabled = false;
		MultiplayerSessionSubsystem->FindSessions(10000);
	}
}

void UMenu_UserWidget::OnFindSessionsComplete_Multiplayer(bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResults)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString(TEXT("Start of CUSTOM_callback2()")), false);

	//Stephen doesn't check Num() since For-each loop can handle empty array without being crashed.
	if (SearchResults.Num() <= 0 || !bWasSuccessful)
	{
		JoinButton->bIsEnabled = true;
		return; //if bWasSuccessful=true, but Num = 0 - connection OKAY but find no session
	}

	for (const FOnlineSessionSearchResult& Result : SearchResults)
	{
		FString MatchType;
		Result.Session.SessionSettings.Get(FName("MatchType") , MatchType );

		FString UserName = Result.Session.OwningUserName;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, UserName, false);

		if (MatchType == MatchTypeValue)
		{
			MultiplayerSessionSubsystem->JoinSession(Result);

			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, MatchType, false);
			//if you already Join a Session (among many found Sessions/searchResults) 
			//then you should either 'break;' out of the loop OR 'return;' out of the WHOLE void function. 
			//otherwise, it keeps loop through the rest even if you already join, this is how C++ loop work: 
			// it wont stop until it reach the final elements of the array unless you intefere with break;/return;
			return; //IMPORTANT for performance!
		}
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString(TEXT("End of CUSTOM_callback2()")), false);
}

void UMenu_UserWidget::OnJoinSessionComplete_Multiplayer(EOnJoinSessionCompleteResult::Type JoinResult)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString(TEXT("Start of CUSTOM_callback3()")), false);

	//IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	//if (OnlineSubsystem == nullptr) return;
	//IOnlineSessionPtr OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
	//if (OnlineSessionInterface.IsValid() == false) return;

	if (MultiplayerSessionSubsystem == false) return;
	if (MultiplayerSessionSubsystem->GetOnlineSessionInterface().IsValid() == false) return;

	if (JoinResult == EOnJoinSessionCompleteResult::Success)
	{
		FString Address;            //you can also directly get SessionInterface indepdently
		MultiplayerSessionSubsystem->GetOnlineSessionInterface()->GetResolvedConnectString(NAME_GameSession , Address );

		if (GetGameInstance())
		{
			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController) PlayerController->ClientTravel(Address , ETravelType::TRAVEL_Absolute);
		}

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan,
			FString::Printf(TEXT("Joined level! Connect String: %s"), *Address), false
		);
	}
	else
	{	//print unknown error: - I think "Found no session" is no specific!
		   if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan,FString(TEXT("Unknown Error!")), false);
		   JoinButton->bIsEnabled = true;
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString(TEXT("End of CUSTOM_callback3()")), false);
}

void UMenu_UserWidget::OnStartSessionComplete_Multiplayer(bool bWasSuccessful)
{
}

void UMenu_UserWidget::OnDestroySessionComplete_Multiplayer(bool bWasSuccessful)
{
}
