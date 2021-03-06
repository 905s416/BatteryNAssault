// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BatteryNAssault.h"
#include "BatteryNAssaultCharacter.h"

//////////////////////////////////////////////////////////////////////////
// ABatteryNAssaultCharacter

ABatteryNAssaultCharacter::ABatteryNAssaultCharacter()
{
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinder<UClass> MachineGun;
		FConstructorStatics() : MachineGun(TEXT("Class'/Game/Weapon/ProjectileWeapons/MachineGun.MachineGun_C'")) {}
	};
	static FConstructorStatics ConstructorStatics;

	if (ConstructorStatics.MachineGun.Object)
	{
		Gun = Cast<UClass>(ConstructorStatics.MachineGun.Object);
	}
	//Temp->K2_SetWorldRotation(FollowCamera.)

	EnergyCostPerSecond = 0.5f;
	MaxEnergy = 100.f;
	Energy = MaxEnergy;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;




	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	Temp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Temp"));
	Temp->AttachTo(RootComponent);



	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->AttachTo(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	TeamID = 0;
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void ABatteryNAssaultCharacter::BeginPlay()
{
	Super::BeginPlay();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	AWeapon *Spawn = GetWorld()->SpawnActor<AWeapon>(Gun, SpawnParameters);
	if (Spawn)
	{
		//Spawn->AttachRootComponentTo(GetMesh(),"WeaponSocket", EAttachLocation::SnapToTarget);
		Spawn->AttachRootComponentTo(Temp);
		Weapon = Spawn;
		
		

	}
}

void ABatteryNAssaultCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Energy > 0)
	{
		Energy -= EnergyCostPerSecond * DeltaTime;
	}

	FString Message = FString::Printf(TEXT("Energy: %.2f"), Energy);
	GEngine->AddOnScreenDebugMessage(0, 5.f, FColor::White, Message);
	if (Temp->GetComponentRotation() != CameraBoom->GetComponentRotation())
	{
		FRotator currentCameraRotation = FMath::RInterpTo(Temp->GetComponentRotation(), CameraBoom->GetComponentRotation(), GetWorld()->GetDeltaSeconds(), 0.5f);
		Temp->SetWorldRotation(currentCameraRotation);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ABatteryNAssaultCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Set up gameplay key bindings
	check(InputComponent);
	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	InputComponent->BindAxis("MoveForward", this, &ABatteryNAssaultCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &ABatteryNAssaultCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &ABatteryNAssaultCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &ABatteryNAssaultCharacter::LookUpAtRate);

	// handle touch devices
	InputComponent->BindTouch(IE_Pressed, this, &ABatteryNAssaultCharacter::TouchStarted);
	InputComponent->BindTouch(IE_Released, this, &ABatteryNAssaultCharacter::TouchStopped);

	InputComponent->BindAction(TEXT("Fire"),
		IE_Pressed,
		this,
		&ABatteryNAssaultCharacter::StartFire);

	InputComponent->BindAction(TEXT("Fire"),
		IE_Released,
		this,
		&ABatteryNAssaultCharacter::StopFire);
	
}


void ABatteryNAssaultCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	// jump, but only on the first touch
	if (FingerIndex == ETouchIndex::Touch1)
	{
		Jump();
	}
}

void ABatteryNAssaultCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		StopJumping();
	}
}

void ABatteryNAssaultCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ABatteryNAssaultCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ABatteryNAssaultCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ABatteryNAssaultCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}

}

void ABatteryNAssaultCharacter::StartFire()
{
	
	Weapon->StartAttack();
}

void ABatteryNAssaultCharacter::StopFire()
{
	Weapon->EndAttack();
}

void ABatteryNAssaultCharacter::Recharge(float charge)
{
	//Energy += charge;
	Energy = FMath::Clamp(Energy + charge, 0.0f, MaxEnergy);

	FString Message = FString::Printf(TEXT("Charging"));
	GEngine->AddOnScreenDebugMessage(4, 0.1f, FColor::White, Message);
}

float ABatteryNAssaultCharacter::GetEnergy()
{
	return Energy;
}