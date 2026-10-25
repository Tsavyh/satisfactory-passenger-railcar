#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "TsabsPassengerAnchor.h"
#include "TsabsPassengerHUD.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerHudCompositionTest,
    "Tsabs.Passenger.Interface.RealWidgetTreeContainsStandardInteraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsPassengerHudCompositionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
    Context.SetCurrentWorld(World);
    UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
    World->SetGameInstance(Instance);
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };
    APlayerController* PC = World->SpawnActor<APlayerController>();
    ULocalPlayer* Local = NewObject<ULocalPlayer>(GEngine);
    PC->SetPlayer(Local);
    // Preview worlds do not run normal controller PostInitializeComponents.
    // PlayerContext resolves through the world's registered controller iterator.
    World->AddController(PC);
    ATsabsPassengerAnchor* Anchor = World->SpawnActor<ATsabsPassengerAnchor>();
    // A generic engine controller avoids pretending that SDK possession stubs
    // implement the game's native transition. This tests UMG composition only.
    PC->SetPawn(Anchor);
    AddInfo(FString::Printf(TEXT("UMG fixture pawn=%d local=%d local_pc=%d context=%d"),
        PC->GetPawn() == Anchor, PC->IsLocalPlayerController(), Local->PlayerController == PC,
        FLocalPlayerContext(Local, World).IsValid()));
    UTsabsPassengerHUD* HUD = CreateWidget<UTsabsPassengerHUD>(PC, UTsabsPassengerHUD::StaticClass());
    if (!TestNotNull(TEXT("Real engine creates the native passenger widget"), HUD)) return false;
    TestTrue(TEXT("Presentation is tied to this exact passenger pawn"), HUD->GetPassengerAnchor() == Anchor);
    UCanvasPanel* Root = HUD->WidgetTree ? Cast<UCanvasPanel>(HUD->WidgetTree->RootWidget) : nullptr;
    if (!TestNotNull(TEXT("Native initialization builds an actual canvas, not only hint data"), Root)) return false;
    UClass* Stock = LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Interface/UI/InGame/Widget_PlayerInteraction.Widget_PlayerInteraction_C"));
    UWidget* Prompt = nullptr;
    int32 PromptCount = 0;
    for (UWidget* Child : Root->GetAllChildren())
        if (Stock && Child->IsA(Stock)) { Prompt = Child; ++PromptCount; }
    TestEqual(TEXT("Only one passive interaction prompt (speed visuals are not prompts)"), PromptCount, 1);
    TestTrue(TEXT("Child reuses the normal E-interaction widget, not a control bar"),
        Prompt && Stock && Prompt->IsA(Stock));
    TestFalse(TEXT("HUD cannot steal keyboard focus"), HUD->IsFocusable());
    HUD->RemoveFromParent();
    return true;
}
#endif
