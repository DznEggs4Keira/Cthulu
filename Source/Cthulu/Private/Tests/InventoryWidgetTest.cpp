// Copyright Cthulu. All Rights Reserved.

#include "InventoryWidget.h"
#include "Misc/AutomationTest.h"

// WITH_DEV_AUTOMATION_TESTS is 1 in Editor and Development builds, and 0 in
// Shipping. Wrapping tests in it means test code never ends up in the game we
// hand to players — it is compiled out entirely.
#if WITH_DEV_AUTOMATION_TESTS

/**
 * IMPLEMENT_SIMPLE_AUTOMATION_TEST declares a test class for us.
 *   arg 1: the C++ class name for this test (convention: F-prefix, ends in Test)
 *   arg 2: the dotted name it shows up under in the Session Frontend test tree
 *   arg 3: where it may run, and how "heavy" it is
 *
 * EditorContext = only run inside the editor (fine: we have no game-only tests yet).
 * ProductFilter = a real feature test, as opposed to a smoke or perf test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryWidgetContractTest,
	"Cthulu.UI.InventoryWidget.HonoursBlueprintBaseContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 * Locks in the architecture rule for this class: it is a C++ base that a designer
 * subclasses in a widget Blueprint, never a class we instantiate directly.
 *
 * If someone later deletes `Abstract` from the UCLASS markup, this test fails and
 * says why — that is the whole point of a test like this. It guards a decision,
 * not a calculation.
 *
 * Returning true means "the test ran to completion". Whether it PASSED is decided
 * by the TestX() calls below — each one reports a failure to the framework itself.
 */
bool FInventoryWidgetContractTest::RunTest(const FString& Parameters)
{
	// StaticClass() is the runtime description of the type — Unreal's reflection
	// data. It is how the engine can list this class in the editor, serialise it,
	// and let a Blueprint inherit from it.
	UClass* const WidgetClass = UInventoryWidget::StaticClass();

	if (!TestNotNull(TEXT("UInventoryWidget::StaticClass() should be registered with the engine"), WidgetClass))
	{
		// Bail out early: every check below would crash on a null class.
		return true;
	}

	TestTrue(
		TEXT("UInventoryWidget must stay Abstract so it can only be used via a widget Blueprint subclass"),
		WidgetClass->HasAnyClassFlags(CLASS_Abstract));

	TestTrue(
		TEXT("UInventoryWidget must derive from UUserWidget so UMG can build and display it"),
		WidgetClass->IsChildOf(UUserWidget::StaticClass()));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
