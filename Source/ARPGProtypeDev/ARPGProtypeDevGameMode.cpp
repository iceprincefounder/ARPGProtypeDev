// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ARPGProtypeDevGameMode.h"
#include "ARPGProtypeDevCharacter.h"
#include "UObject/ConstructorHelpers.h"

AARPGProtypeDevGameMode::AARPGProtypeDevGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonProtype/Characters/NoWeaponSoldierCharacterBP"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
