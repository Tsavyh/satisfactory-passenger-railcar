#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "TsabsPassengerAnchor.h"
#include "TsabsPassengerHUD.h"
#include "TsabsPassengerRailcar.h"
#include "FGRailroadVehicleMovementComponent.h"
#include <bit>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsResolvedExitKeyTest,
    "Tsabs.Passenger.Interface.ExitReadsEffectiveLeaveBindingNotGenericIcon",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsResolvedExitKeyTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    APlayerController* PC = World->SpawnActor<APlayerController>();
    PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    World->AddController(PC);
    PC->SetPawn(World->SpawnActor<ATsabsPassengerAnchor>());
    UEnhancedPlayerInput* Input = NewObject<UEnhancedPlayerInput>(PC);
    PC->PlayerInput = Input;
    UTsabsPassengerHUD* HUD = CreateWidget<UTsabsPassengerHUD>(PC, UTsabsPassengerHUD::StaticClass());
    if (!TestNotNull(TEXT("Presentation instance"), HUD)) return false;
    UInputAction* Leave = LoadObject<UInputAction>(nullptr,
        TEXT("/Game/FactoryGame/Inputs/Vehicles/Actions/IA_Vehicles_Leave.IA_Vehicles_Leave"));
    UInputAction* Other = NewObject<UInputAction>();
    // Populate the non-const fixture's effective mapping array directly. This
    // verifies presentation reads; it does NOT claim to simulate a key event,
    // user-profile rebuild, or the game's input-processing pipeline.
    TArray<FEnhancedActionKeyMapping>& Mappings = const_cast<TArray<FEnhancedActionKeyMapping>&>(Input->GetEnhancedActionMappings());
    Mappings.Emplace(Other, EKeys::X);
    Mappings.Emplace(Leave, EKeys::Gamepad_FaceButton_Bottom);
    Mappings.Emplace(Leave, EKeys::F);
    TestEqual(TEXT("Keyboard shows its actual leave mapping, not another action or controller icon"),
        HUD->ResolveLeaveKey().ToString(), FString(TEXT("F")));
    Mappings.Last().Key = EKeys::E;
    TestEqual(TEXT("Changed effective binding is reflected without a stale F or fixed key"),
        HUD->ResolveLeaveKey().ToString(), FString(TEXT("E")));
    ATsabsPassengerAnchor* Anchor = Cast<ATsabsPassengerAnchor>(PC->GetPawn());
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestTrue(TEXT("Prompt fixture owns an attached railcar"), Anchor->InitializePassengerAnchor(Car, FTransform::Identity))) return false;
    Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(900.0f);
    HUD->RefreshExitHint(true);
    TestTrue(TEXT("Cruising at-or-above30 hides the Leave-key hint and any idle restriction"), HUD->LastPrompt.IsEmpty());
    TestEqual(TEXT("Hidden text also collapses only the owned interaction widget"), HUD->InteractionWidget->GetVisibility(), ESlateVisibility::Collapsed);
    UFunction* Rejection = HUD->FindFunction(TEXT("NotifyExitSpeedRejected"));
    if (!TestNotNull(TEXT("A rejected exit attempt can notify the local passenger HUD"), Rejection)) return false;
    HUD->ProcessEvent(Rejection, nullptr);
    TestTrue(TEXT("Explicit rejection shows the speed explanation without a misleading key glyph"),
        HUD->LastPrompt.EqualTo(UTsabsPassengerHUD::FormatExitPrompt(FText::FromString(TEXT("E")), false)));
    HUD->NativeTick(FGeometry(), 2.0f);
    TestTrue(TEXT("Rejection remains visible briefly"), HUD->LastPrompt.ToString().Contains(TEXT("30")));
    HUD->NativeTick(FGeometry(), 1.0f);
    TestTrue(TEXT("After three seconds the rejection clears without advertising Leave while still fast"), HUD->LastPrompt.IsEmpty());
    TestEqual(TEXT("The rejected-attempt warning expires back to hidden"), HUD->InteractionWidget->GetVisibility(), ESlateVisibility::Collapsed);
    HUD->NativeTick(FGeometry(), 10.0f);
    TestFalse(TEXT("Continued fast travel cannot retrigger a rejected-attempt warning"), HUD->LastPrompt.ToString().Contains(TEXT("km/h")));
    HUD->ProcessEvent(Rejection, nullptr);
    HUD->NativeTick(FGeometry(), 1.0f);
    HUD->ProcessEvent(Rejection, nullptr);
    HUD->NativeTick(FGeometry(), 2.0f);
    TestTrue(TEXT("A fresh rejected press restarts the short warning"), HUD->LastPrompt.ToString().Contains(TEXT("30")));
    Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(0.0f);
    HUD->NativeTick(FGeometry(), 0.01f);
    TestFalse(TEXT("Slowing below30 clears a stale rejection immediately"), HUD->LastPrompt.ToString().Contains(TEXT("km/h")));
    TestTrue(TEXT("Slowing below30 restores the effective Leave-key instruction"),
        HUD->LastPrompt.EqualTo(UTsabsPassengerHUD::FormatExitPrompt(FText::FromString(TEXT("E")), true)));
    TestEqual(TEXT("The passive interaction becomes visible again when allowed"), HUD->InteractionWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);
    HUD->ProcessEvent(Rejection, nullptr);
    TestFalse(TEXT("A late server rejection cannot show the speed warning after slowing"), HUD->LastPrompt.ToString().Contains(TEXT("km/h")));
    Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(900.0f);
    HUD->NativeTick(FGeometry(), 0.01f);
    TestFalse(TEXT("Accelerating again is not a new exit attempt"), HUD->LastPrompt.ToString().Contains(TEXT("km/h")));
    TestTrue(TEXT("Accelerating again hides the ordinary Leave instruction"), HUD->LastPrompt.IsEmpty());
    struct FVisibleCase { uint32 Bits; bool bVisible; };
    const FVisibleCase VisibilityCases[] = {
        {0x00000000u, true}, {0x44505555u, true}, {0x44505556u, false},
        {0xc4505555u, true}, {0xc4505556u, false}, {0x7fc00000u, false},
        {0x7f800000u, false}, {0xff800000u, false}};
    for (const FVisibleCase& Case : VisibilityCases)
    {
        Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(std::bit_cast<float>(Case.Bits));
        HUD->NativeTick(FGeometry(), 0.01f);
        TestEqual(FString::Printf(TEXT("Leave hint visibility follows strict signed access policy: %08x"), Case.Bits),
            !HUD->LastPrompt.IsEmpty(), Case.bVisible);
        TestEqual(TEXT("Widget visibility matches the instruction without collapsing the ticking HUD"),
            HUD->InteractionWidget->GetVisibility(), Case.bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
        TestEqual(TEXT("The parent HUD remains ticking/visible even when its prompt is hidden"),
            HUD->GetVisibility(), ESlateVisibility::HitTestInvisible);
    }
    UFunction* ClientFeedback = Anchor->FindFunction(TEXT("Client_NotifyExitSpeedRejected"));
    if (TestNotNull(TEXT("Remote server rejection has an owning-client feedback route"), ClientFeedback))
    {
        TestTrue(TEXT("Feedback is a reliable client RPC"), ClientFeedback->HasAllFunctionFlags(FUNC_Net | FUNC_NetClient | FUNC_NetReliable));
        TestFalse(TEXT("Feedback is never a multicast to other passengers"), ClientFeedback->HasAnyFunctionFlags(FUNC_NetMulticast));
    }
    Mappings.Reset();
    TestTrue(TEXT("No binding never invents E or a generic X"), HUD->ResolveLeaveKey().IsEmpty());
    return true;
}
#endif
