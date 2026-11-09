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
#include "Components/CanvasPanel.h"
#include <bit>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsResolvedExitKeyTest,
    "Tsabs.Passenger.NativeFirst.Interface.ExitBindingRemainsVisibleWithoutSpeedRule",
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
    // Read-only control hint, NOT proof that native exit/placement is allowed.
    const uint32 Speeds[] = {0x00000000u, 0x44505555u, 0x44505556u,
        0xc4505555u, 0xc4505556u, 0x44610000u, 0x7fc00000u, 0x7f800000u, 0xff800000u};
    for (uint32 Bits : Speeds)
    {
        Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(std::bit_cast<float>(Bits));
        HUD->RefreshExitHint(true);
        HUD->NativeTick(FGeometry(), 0.01f);
        TestTrue(FString::Printf(TEXT("Effective Leave-key hint is independent of speed bits %08x"), Bits),
            HUD->LastPrompt.ToString().Contains(TEXT("<Key>E</>")));
        TestFalse(TEXT("No custom speed warning in normal travel"), HUD->LastPrompt.ToString().Contains(TEXT("km/h")));
        TestEqual(TEXT("Only the local informational interaction is shown"), HUD->InteractionWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);
        TestEqual(TEXT("Parent HUD remains visible"), HUD->GetVisibility(), ESlateVisibility::HitTestInvisible);
    }
    TestNull(TEXT("Obsolete custom speed-warning RPC no longer registered"), Anchor->FindFunction(TEXT("Client_NotifyExitSpeedRejected")));
    TestNull(TEXT("Obsolete local speed-warning event no longer registered"), HUD->FindFunction(TEXT("NotifyExitSpeedRejected")));
    Mappings.Reset();
    TestTrue(TEXT("No binding never invents E or a generic X"), HUD->ResolveLeaveKey().IsEmpty());

    // Real UMG parent removal with a generic engine controller. SetPawn is a
    // fixture transition, NOT a claim to exercise Satisfactory possession/RPCs.
    UCanvasPanel* Host = NewObject<UCanvasPanel>(PC);
    UCanvasPanel* Unrelated = NewObject<UCanvasPanel>(PC);
    Host->AddChild(HUD);
    Host->AddChild(Unrelated);
    TestTrue(TEXT("Old passenger HUD is actually attached before the transition"), HUD->GetParent() == Host);
    Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(900.0f);
    const FText BeforePawnChange = HUD->LastPrompt;
    PC->SetPawn(nullptr);
    HUD->NativeTick(FGeometry(), 0.01f);
    TestNull(TEXT("Loss of owning pawn removes the old HUD from its real parent"), HUD->GetParent());
    TestTrue(TEXT("HUD removal preserves unrelated UI"), Unrelated->GetParent() == Host);
    TestTrue(TEXT("Delayed feedback cannot mutate a HUD after pawn loss"), HUD->LastPrompt.EqualTo(BeforePawnChange));

    ATsabsPassengerAnchor* NextAnchor = World->SpawnActor<ATsabsPassengerAnchor>();
    if (!TestNotNull(TEXT("Next passenger fixture anchor"), NextAnchor)) return false;
    ATsabsPassengerRailcar* NextCar = World->SpawnActor<ATsabsPassengerRailcar>();
    if (!TestNotNull(TEXT("Next passenger fixture car"), NextCar) ||
        !TestTrue(TEXT("Next fixture attachment"), NextAnchor->InitializePassengerAnchor(NextCar, FTransform::Identity))) return false;
    NextCar->GetRailroadVehicleMovementComponent()->SetForwardSpeed(900.0f);
    PC->SetPawn(NextAnchor);
    UTsabsPassengerHUD* NextHUD = CreateWidget<UTsabsPassengerHUD>(PC, UTsabsPassengerHUD::StaticClass());
    if (!TestNotNull(TEXT("Reentry creates a fresh HUD instance"), NextHUD)) return false;
    TestTrue(TEXT("Fresh HUD is tied only to the new passenger"), NextHUD->GetPassengerAnchor() == NextAnchor);
    TestFalse(TEXT("Fresh HUD never invents a speed rejection"), NextHUD->LastPrompt.ToString().Contains(TEXT("km/h")));
    Host->AddChild(NextHUD);
    Host->AddChild(HUD);
    HUD->NativeTick(FGeometry(), 0.01f);
    TestNull(TEXT("Stale HUD removes itself when a different anchor owns the controller"), HUD->GetParent());
    TestTrue(TEXT("Stale HUD cannot remove the replacement HUD"), NextHUD->GetParent() == Host);
    TestTrue(TEXT("Unrelated UI survives replacement"), Unrelated->GetParent() == Host);
    PC->SetPawn(nullptr);
    NextHUD->NativeTick(FGeometry(), 0.01f);
    TestNull(TEXT("Replacement HUD also detaches on pawn loss"), NextHUD->GetParent());
    return true;
}
#endif
