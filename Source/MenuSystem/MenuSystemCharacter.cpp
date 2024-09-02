// Copyright Epic Games, Inc. All Rights Reserved.

#include "MenuSystemCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SkeletalMeshComponent.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"


//////////////////////////////////////////////////////////////////////////
// AMenuSystemCharacter

AMenuSystemCharacter::AMenuSystemCharacter()
: OnCreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete) ),
  OnFindSessionsCompleteDelegate( FOnFindSessionsCompleteDelegate ::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
  OnJoinSessionCompleteDelegate(  FOnJoinSessionCompleteDelegate  ::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	if (OnlineSubsystem)
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		if (GEngine) GEngine->AddOnScreenDebugMessage(0, 10.f, FColor::Red
			, FString::Printf(TEXT("Found system: %s"), *OnlineSubsystem->GetSubsystemName().ToString()));
	}

	//OnActorBeginOverlap                = FActorBeginOverlapSignature::CreateUObject(); - not EXIST
	//GetMesh()->OnComponentBeginOverlap = FComponentBeginOverlapSignature::CreateUObject(); - not EXIST
	//OnCreateSessionComplete            = FOnCreateSessionCompleteDelegate::CreateUObject(); - EXIST

	//SHOULD do it in : ___ to have better performance, as Stephen does :D :D
	//OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &AMenuSystemCharacter::OnCreateSessionComplete);

}

void AMenuSystemCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	if (!OnlineSessionInterface.IsValid()) return;
	OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
	OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
	OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

	UGameInstance* x = GetGameInstance();
	//UGameInstanceSubsystem* ; 

	
}

//plan to be called as we press 1 key from BP (THIS IS NEW, forget about old lessons)
void AMenuSystemCharacter::CreateGameSession()
{
	if (OnlineSessionInterface.IsValid())
	{
		FNamedOnlineSession* ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);

		if (ExistingSession != nullptr) OnlineSessionInterface->DestroySession(NAME_GameSession);


		ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

		//FOnlineSessionSettings SessionSettings; //Why dont use this?

		TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());

		SessionSettings->bIsLANMatch = false;
		SessionSettings->NumPublicConnections = 3;
		SessionSettings->bShouldAdvertise =true;
		SessionSettings->bAllowJoinInProgress =true;
		SessionSettings->bAllowJoinViaPresence = true; //this is a must1
		SessionSettings->bUsesPresence =true;          //this is a must0
		//if you can not find sessions, then this will/may fix
		SessionSettings->bUseLobbiesIfAvailable = true; 

		//compare this with SessionSearch->Set( , , ), this is OPTIONAL, so that you can check it back to join the right session 
		//of the right player, because there are many people created sessions that may also match your search/query settings!
		SessionSettings->Set(FName("MatchType"), FString("FreeForAll"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing); //for NEXT lesson

		if(LocalPlayer)	OnlineSessionInterface->CreateSession( *LocalPlayer->GetPreferredUniqueNetId() , NAME_GameSession, *SessionSettings);
	}
}
//meant to be auto-triggered after you call CreateSessionGame() above
void AMenuSystemCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(1, 10.f, FColor::Blue
		, FString::Printf( TEXT("Created session: %s"), *SessionName.ToString() ) );

		UWorld* World = GetWorld();
		if ( World) World->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"));
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(1, 10.f, FColor::Red, FString(TEXT("Failed to create session!")) );
	}
}

//press KEY2 to run this
void AMenuSystemCharacter::FindGameSession()
{
	if (OnlineSessionInterface.IsValid() == false) return;

	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	SessionSearch = MakeShareable(new FOnlineSessionSearch()) ; //work
		SessionSearch->bIsLanQuery = false; //because we're not using LAN
		SessionSearch->MaxSearchResults = 10000; //because there is a good chance many people will use AppID 480 at the momment
		SessionSearch->QuerySettings.Set<bool>(SEARCH_PRESENCE , true, EOnlineComparisonOp::Equals);
	//this function will actually change the [*SessionSearch] after the finding is complete
	if(LocalPlayer)	OnlineSessionInterface->FindSessions( *LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}

void AMenuSystemCharacter::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (!(SessionSearch.IsValid()) ) return;
	if(!OnlineSessionInterface.IsValid() ) return;
	if (!bWasSuccessful) return;
		
	//TArray<FOnlineSessionSearchResult> Results = SessionSearch->SearchResults; //for fun

	for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults) //or just use 'auto'
	{
		FString SessionID = Result.GetSessionIdStr();
		FString OwningUserName = Result.Session.OwningUserName;
		FString MatchTypeValue;
		bool IsKeyFound = Result.Session.SessionSettings.Get(FName("MatchType") , MatchTypeValue ); //I add 'bool bFindKey' for fun

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan,
			FString::Printf(TEXT("Session ID: %s, OwningUserName: %s") , *SessionID , *OwningUserName ), false 
		);

		//JOIN PART: only when the value of key MatchType matches the value first specify by the session creator we allow it to join!
		if (MatchTypeValue == FString("FreeForAll"))
		{

			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan,
				FString::Printf(TEXT("Joined a session with MatchType: %s"), *MatchTypeValue), false
			);

			ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

			if (LocalPlayer) OnlineSessionInterface->JoinSession(
					*LocalPlayer->GetPreferredUniqueNetId(), //LocalUserID
					NAME_GameSession,						 //SessionName to join - surely want the name the first player created!
					Result     
			);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan,
				FString::Printf(TEXT("pass '->JoinSession'")));
		}
	}
}

void AMenuSystemCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type EValue)
{
	if (!OnlineSessionInterface.IsValid()) return;
	
	FString Address_Of_TheCreatorOfTheLastJoinedSession;
	//this function is to check if this device currently join any session
	//, and if yes modify the FString into the address of the session creator from other device
	bool IsConnectedToASession = 
		OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession , Address_Of_TheCreatorOfTheLastJoinedSession);

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan,
		FString::Printf(TEXT("Connect String: %s"), *Address_Of_TheCreatorOfTheLastJoinedSession), false
	);


	if (IsConnectedToASession)
	{
		APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
		//GetWorld()->GetFirstPlayerController();

		if (PlayerController) PlayerController->ClientTravel(Address_Of_TheCreatorOfTheLastJoinedSession, ETravelType::TRAVEL_Absolute);
		//UGameplayStatics::OpenLevel(this, *Address_Of_TheCreatorOfTheLastJoinedSession);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input
void AMenuSystemCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMenuSystemCharacter::Look);

	}

}

void AMenuSystemCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMenuSystemCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}




