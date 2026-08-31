// Copyright Cthulu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

// Must be the LAST include in the file. Unreal Header Tool generates this file
// from the UCLASS/UPROPERTY/UFUNCTION markup below, and it assumes it is last.
#include "InventoryWidget.generated.h"

/**
 * C++ base class for the player's inventory UI.
 *
 * Abstract on purpose: this class is never spawned directly. A widget Blueprint
 * (Content/Code/UI/) reparents to it, builds the visual layout, and fills in the
 * behaviour hooks. C++ owns the rules; the Blueprint owns the look.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, meta = (DisplayName = "Inventory Widget Base"))
class CTHULU_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
};
