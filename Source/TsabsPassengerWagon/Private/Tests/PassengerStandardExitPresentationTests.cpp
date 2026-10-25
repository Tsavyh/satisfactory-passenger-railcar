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
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "UObject/UnrealType.h"
#include "TsabsPassengerAnchor.h"
#include "TsabsPassengerHUD.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerStandardExitPresentationTest,
    "Tsabs.Passenger.Interface.ExitUsesStandardInteractionPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsPassengerStandardExitPresentationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    APlayerController* PC = World->SpawnActor<APlayerController>();
    PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    World->AddController(PC);
    PC->SetPawn(World->SpawnActor<ATsabsPassengerAnchor>());
    UClass* StockClass = LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Interface/UI/InGame/Widget_PlayerInteraction.Widget_PlayerInteraction_C"));
    if (!TestNotNull(TEXT("Original standard interaction class resolves"), StockClass)) return false;
    UUserWidget* Stock = CreateWidget<UUserWidget>(PC, StockClass);
    if (TestNotNull(TEXT("Standard interaction template can be inspected"), Stock))
    {
        for (TFieldIterator<UFunction> It(StockClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
        {
            FString Details;
            for (TFieldIterator<FProperty> P(*It); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
                Details += FString::Printf(TEXT(" %s:%s"), *P->GetName(), *P->GetCPPType());
            AddInfo(FString::Printf(TEXT("STOCK_FUNCTION %s%s"), *It->GetName(), *Details));
        }
        TArray<UWidget*> Widgets;
        if (Stock->WidgetTree) Stock->WidgetTree->GetAllWidgets(Widgets);
        for (UWidget* Widget : Widgets)
        {
            FString Extra;
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
            {
                FAnchors A = CanvasSlot->GetAnchors();
                Extra = FString::Printf(TEXT(" anchors=%.3f,%.3f alignment=%.3f,%.3f"),
                    A.Minimum.X, A.Minimum.Y, CanvasSlot->GetAlignment().X, CanvasSlot->GetAlignment().Y);
            }
            AddInfo(FString::Printf(TEXT("STOCK_WIDGET %s %s%s"), *Widget->GetName(), *Widget->GetClass()->GetName(), *Extra));
        }
    }
    UClass* GameUIClass = LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Interface/UI/InGame/BP_GameUI.BP_GameUI_C"));
    UUserWidget* GameUI = GameUIClass ? CreateWidget<UUserWidget>(PC, GameUIClass) : nullptr;
    if (GameUI && GameUI->WidgetTree)
    {
        TArray<UWidget*> NativeWidgets;
        GameUI->WidgetTree->GetAllWidgets(NativeWidgets);
        for (UWidget* Widget : NativeWidgets)
            if (Widget->IsA(StockClass))
            {
                FString Layout = TEXT("non-canvas parent");
                if (UCanvasPanelSlot* NativeSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
                {
                    FAnchorData Data = NativeSlot->GetLayout();
                    Layout = FString::Printf(TEXT("anchors %.3f %.3f %.3f %.3f align %.3f %.3f offsets %.3f %.3f %.3f %.3f autosize %d"),
                        Data.Anchors.Minimum.X, Data.Anchors.Minimum.Y, Data.Anchors.Maximum.X, Data.Anchors.Maximum.Y,
                        Data.Alignment.X, Data.Alignment.Y, Data.Offsets.Left, Data.Offsets.Top, Data.Offsets.Right, Data.Offsets.Bottom,
                        NativeSlot->GetAutoSize());
                }
                AddInfo(FString::Printf(TEXT("NATIVE_INTERACTION_SLOT %s %s"), *Widget->GetName(), *Layout));
            }
    }
    UTsabsPassengerHUD* HUD = CreateWidget<UTsabsPassengerHUD>(PC, UTsabsPassengerHUD::StaticClass());
    UCanvasPanel* Root = HUD && HUD->WidgetTree ? Cast<UCanvasPanel>(HUD->WidgetTree->RootWidget) : nullptr;
    if (!TestNotNull(TEXT("Passenger presentation root"), Root)) return false;
    UWidget* Prompt = nullptr;
    int32 PromptCount = 0;
    for (UWidget* Child : Root->GetAllChildren())
        if (Child->IsA(StockClass)) { Prompt = Child; ++PromptCount; }
    TestEqual(TEXT("Exactly one normal interaction, independently of informational speed visuals"), PromptCount, 1);
    TestTrue(TEXT("Exit uses the same interaction widget as normal E prompts, not a bottom button bar"),
        Prompt && Prompt->IsA(StockClass));
    UCanvasPanelSlot* PromptSlot = Prompt ? Cast<UCanvasPanelSlot>(Prompt->Slot) : nullptr;
    if (!TestNotNull(TEXT("Interaction occupies its own passive screen slot"), PromptSlot)) return false;
    const FAnchorData Layout = PromptSlot->GetLayout();
    TestTrue(TEXT("Native middle-screen anchor, not bottom edge"),
        Layout.Anchors.Minimum.Equals(FVector2D(0.5, 0.5)) && Layout.Anchors.Maximum.Equals(FVector2D(0.5, 0.5)));
    TestTrue(TEXT("Native center/top alignment"), Layout.Alignment.Equals(FVector2D(0.5, 0.0)));
    TestTrue(TEXT("Native message offset below crosshair"), FMath::IsNearlyEqual(Layout.Offsets.Top, 45.0f));
    TestTrue(TEXT("Interaction sizes to its original text and key capsule"), PromptSlot->GetAutoSize());
    return true;
}
#endif
