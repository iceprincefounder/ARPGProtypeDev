// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ARPGProtypeDevCharacter.h"
#include "Engine/Engine.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// AARPGProtypeDevCharacter

AARPGProtypeDevCharacter::AARPGProtypeDevCharacter()
{
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
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AARPGProtypeDevCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AARPGProtypeDevCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AARPGProtypeDevCharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AARPGProtypeDevCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AARPGProtypeDevCharacter::MoveRight);

	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AARPGProtypeDevCharacter::Attack);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AARPGProtypeDevCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AARPGProtypeDevCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AARPGProtypeDevCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AARPGProtypeDevCharacter::TouchStopped);
}

void AARPGProtypeDevCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AARPGProtypeDevCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AARPGProtypeDevCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AARPGProtypeDevCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AARPGProtypeDevCharacter::IsInAir() const
{
	return GetMovementComponent()->IsFalling();
}

bool AARPGProtypeDevCharacter::IsInWater() const
{
	return GetMovementComponent()->IsSwimming();
}

bool AARPGProtypeDevCharacter::IsAccelerating() const
{
	FVector Acceleration = GetCharacterMovement()->GetCurrentAcceleration();
	if (Acceleration.Size() > 0)
	{
		return true;
	}
	return false;
}

bool AARPGProtypeDevCharacter::IsAttacking() const
{
	return bIsAttacking;
}

float AARPGProtypeDevCharacter::GetSpeed() const
{
	return GetVelocity().Size();
}

void AARPGProtypeDevCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		if (bIsAttacking)
		{
			Value *= 0.1f;
		}
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AARPGProtypeDevCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		if (bIsAttacking)
		{
			Value *= 0.1f;
		}
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

/** Called for jump input */
void AARPGProtypeDevCharacter::Jump()
{
	Super::Jump();
}

/** Called for stop jumping input */
void AARPGProtypeDevCharacter::StopJumping()
{
	Super::StopJumping();
}

/** Called for attack input */
void AARPGProtypeDevCharacter::Attack()
{
	if (bIsAttacking)
	{
		bSaveAttacking = true;
	}
	else
	{
		bIsAttacking = true;
		AttackCombo();
	}
	//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, FString("OnResetAttack"));
}

/** Do attack combo. */
void AARPGProtypeDevCharacter::AttackCombo()
{
	if (AttackComboMontage.Num() > 0)
	{
		int32 CurrentCount = ComboCount % AttackComboMontage.Num();
		UAnimMontage* AnimMontage = AttackComboMontage[CurrentCount];
		if (AnimMontage && !AnimMontage->IsPendingKill() && AnimMontage->IsValidLowLevel())
		{
			PlayAnimMontage(AnimMontage);
		}
	}
}

void AARPGProtypeDevCharacter::OnSaveAttack()
{
	if (bSaveAttacking)
	{
		bSaveAttacking = false;
		ComboCount++;
		AttackCombo();
	}
}

void AARPGProtypeDevCharacter::OnResetAttack()
{
	ComboCount = 0;
	bSaveAttacking = false;
	bIsAttacking = false;
}
