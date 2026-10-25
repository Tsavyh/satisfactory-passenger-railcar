#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "TsabsPassengerAnchor.h"
#include "TsabsPassengerHUD.h"
#include "TsabsPassengerRailcar.h"
#include "FGRailroadVehicleMovementComponent.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerSpeedCompositionTest,
    "Tsabs.Passenger.Interface.PassengerOwnsNativeStyleSpeedWithoutPower",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPassengerSpeedCompositionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    APlayerController* PC = World->SpawnActor<APlayerController>();
    PC->SetPlayer(NewObject<ULocalPlayer>(GEngine)); World->AddController(PC);
    auto* Anchor = World->SpawnActor<ATsabsPassengerAnchor>();
    auto* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    Anchor->InitializePassengerAnchor(Car, FTransform::Identity); PC->SetPawn(Anchor);
    Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(-1000.0f);
    auto* HUD = CreateWidget<UTsabsPassengerHUD>(PC, UTsabsPassengerHUD::StaticClass());
    if (!TestNotNull(TEXT("Real locally initialized passenger widget"), HUD)) return false;
    auto* Speed = Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("SpeedinKMHText")));
    if (!TestNotNull(TEXT("Passenger owns native speed text, not a driving HUD"), Speed)) return false;
    auto* Class = Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Locomotive/UI/Widget_Locomotive_HUD.Widget_Locomotive_HUD_C")));
    auto* Native = Class->FindWidgetTreeOwningClass()->GetWidgetTreeArchetype();
    auto* NativeText = CastChecked<UTextBlock>(Native->FindWidget(TEXT("SpeedinKMHText")));
    TestTrue(TEXT("Native font copied exactly"), Speed->GetFont() == NativeText->GetFont());
    TestTrue(TEXT("Native text color copied exactly"), Speed->GetColorAndOpacity() == NativeText->GetColorAndOpacity());
    TestTrue(TEXT("Independent instance; native template remains untouched"), Speed != NativeText && NativeText->GetText().ToString() == TEXT("999"));
    TestTrue(TEXT("Read-only initial speed comes from outer vehicle"), Speed->GetText().EqualTo(UTsabsPassengerHUD::FormatPassengerSpeed(-1000.0f, true)));
    auto* BG = Cast<UImage>(HUD->GetWidgetFromName(TEXT("PowerBG")));
    auto* NativeBG = CastChecked<UImage>(Native->FindWidget(TEXT("PowerBG")));
    TestNotNull(TEXT("Minimal panel retains the real native black rectangle"), BG);
    if (BG) TestTrue(TEXT("Black rectangle brush including texture/layout/tint retained"), BG->GetBrush() == NativeBG->GetBrush());
    auto* Unit = Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("TextBlock_3")));
    auto* NativeUnit = CastChecked<UTextBlock>(Native->FindWidget(TEXT("TextBlock_3")));
    if (TestNotNull(TEXT("Minimal panel retains native km/h unit"), Unit))
    {
        TestTrue(TEXT("Unit text, font and color retained exactly"), Unit->GetText().EqualTo(NativeUnit->GetText()) &&
            Unit->GetFont() == NativeUnit->GetFont() && Unit->GetColorAndOpacity() == NativeUnit->GetColorAndOpacity());
        TestFalse(TEXT("Unit has no locomotive graph binding"), Unit->TextDelegate.IsBound());
    }
    const TCHAR* Decorations[] = {TEXT("Speedometer"), TEXT("mSpeedometerBG"), TEXT("VehicleVisorBG"), TEXT("TextBlock_2")};
    TArray<UObject*> Owned; GetObjectsWithOuter(Speed->GetOuter(), Owned, true);
    for (const TCHAR* Name : Decorations)
    {
        TestNull(FString::Printf(TEXT("Decoration is absent, not merely hidden: %s"), Name), HUD->GetWidgetFromName(Name));
        TestFalse(FString::Printf(TEXT("No orphan decorative allocation: %s"), Name),
            Owned.ContainsByPredicate([Name](const UObject* Object) { return Object->GetFName() == Name; }));
    }
    const TCHAR* Retained[] = {TEXT("Ov_SpeedFuel"), TEXT("Overlay_2"), TEXT("PowerBG"), TEXT("SizeBox_3"),
        TEXT("VehicleUIContainer"), TEXT("HorizontalBox_0"), TEXT("VerticalBox_0"), TEXT("CurrentSpeedSlot"),
        TEXT("SpeedinKMHText"), TEXT("Spacer_131"), TEXT("TextBlock_3")};
    for (const TCHAR* Name : Retained)
    {
        UWidget* Widget = HUD->GetWidgetFromName(Name);
        UWidget* Source = Native->FindWidget(Name);
        if (!TestNotNull(FString::Printf(TEXT("Retained native panel node: %s"), Name), Widget)) continue;
        TestEqual(TEXT("All retained nodes are non-hit-testable"), Widget->GetVisibility(), ESlateVisibility::HitTestInvisible);
        TestFalse(TEXT("No copied node is a UUserWidget or event graph"), Widget->IsA<UUserWidget>());
        if (!TestNotNull(TEXT("Retained node slot"), Widget->Slot.Get())) continue;
        TestTrue(TEXT("Retained slot class matches native"), Widget->Slot->GetClass() == Source->Slot->GetClass());
        for (TFieldIterator<FProperty> Property(Source->Slot->GetClass()); Property; ++Property)
        {
            if (!Property->HasAnyPropertyFlags(CPF_Edit) || Property->GetOwnerClass() == UPanelSlot::StaticClass() ||
                CastField<FObjectPropertyBase>(*Property) || CastField<FDelegateProperty>(*Property) ||
                CastField<FMulticastDelegateProperty>(*Property)) continue;
            TestTrue(FString::Printf(TEXT("Native slot layout %s.%s"), Name, *Property->GetName()),
                Property->Identical_InContainer(Widget->Slot, Source->Slot));
        }
    }
    auto* Container = HUD->GetWidgetFromName(TEXT("Ov_SpeedFuel"));
    auto* Slot = Container ? Cast<UCanvasPanelSlot>(Container->Slot) : nullptr;
    auto* NativeSlot = CastChecked<UCanvasPanelSlot>(Native->FindWidget(TEXT("Ov_SpeedFuel"))->Slot);
    if (TestNotNull(TEXT("Native canvas ancestor retained"), Slot))
    {
        TestTrue(TEXT("Native anchor and alignment retained"), Slot->GetAnchors() == NativeSlot->GetAnchors() && Slot->GetAlignment() == NativeSlot->GetAlignment());
        TestTrue(TEXT("Native offsets retained"), Slot->GetOffsets() == NativeSlot->GetOffsets());
    }
    const TCHAR* Excluded[] = {TEXT("PowerSlot"),TEXT("mPowerConsumptionText"),TEXT("mwText"),TEXT("mBrakeIndicator"),TEXT("mDockingIndicator"),TEXT("mHintContainer"),TEXT("PointerContainer")};
    for (const TCHAR* Name : Excluded) TestNull(FString::Printf(TEXT("No excluded UI: %s"), Name), HUD->GetWidgetFromName(Name));
    TestFalse(TEXT("No inherited driving-widget graph or ownership"), HUD->IsA(Class));
    TestNotNull(TEXT("Existing exit interaction still present"), HUD->GetWidgetFromName(TEXT("PassengerInteractionCanvas")));
    TestFalse(TEXT("No native text binding can overwrite passenger value"), Speed->TextDelegate.IsBound());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerSpeedLifecycleTest,
    "Tsabs.Passenger.Interface.SpeedUpdatesRecoversAndInvalidatesLocally",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPassengerSpeedLifecycleTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    auto* PC = World->SpawnActor<APlayerController>();
    PC->SetPlayer(NewObject<ULocalPlayer>(GEngine)); World->AddController(PC);
    auto* Anchor = World->SpawnActor<ATsabsPassengerAnchor>();
    auto* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    Anchor->InitializePassengerAnchor(Car, FTransform::Identity); PC->SetPawn(Anchor);
    auto* HUD = CreateWidget<UTsabsPassengerHUD>(PC, UTsabsPassengerHUD::StaticClass());
    auto* Text = HUD ? Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("SpeedinKMHText"))) : nullptr;
    if (!TestNotNull(TEXT("Actual local speed widget"), Text)) return false;
    auto* Host = NewObject<UCanvasPanel>(PC); Host->AddChild(HUD);
    const uint32 Cases[] = {0x447a0000u,0xc47a0000u,0x00000000u,0x80000000u,0x7fc00000u,0x7f800000u,0xff800000u,0x44505555u,0x44505556u};
    for (uint32 Bits : Cases)
    {
        float Speed; FMemory::Memcpy(&Speed, &Bits, sizeof(Speed));
        Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(Speed);
        HUD->NativeTick(FGeometry(), 0.01f);
        TestTrue(FString::Printf(TEXT("Updated signed/nonfinite speed %08x"), Bits), Text->GetText().EqualTo(UTsabsPassengerHUD::FormatPassengerSpeed(Speed, true)));
        const FText Cached = Text->GetText(); HUD->NativeTick(FGeometry(), 0.01f);
        TestTrue(TEXT("Unchanged reading remains stable"), Text->GetText().EqualTo(Cached));
    }
    Anchor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    HUD->NativeTick(FGeometry(), 0.01f);
    TestTrue(TEXT("Lost attachment is unavailable not zero"), Text->GetText().EqualTo(UTsabsPassengerHUD::FormatPassengerSpeed(0, false)));
    Anchor->AttachToActor(Car, FAttachmentTransformRules::KeepWorldTransform);
    Car->GetRailroadVehicleMovementComponent()->SetForwardSpeed(1000.0f);
    HUD->NativeTick(FGeometry(), 0.01f);
    TestTrue(TEXT("Valid speed recovers without cached unavailable"), Text->GetText().EqualTo(UTsabsPassengerHUD::FormatPassengerSpeed(1000, true)));
    PC->SetPawn(nullptr); HUD->NativeTick(FGeometry(), 0.01f);
    TestNull(TEXT("Pawn loss detaches only owned HUD"), HUD->GetParent());
    TestTrue(TEXT("Stale numeric reading invalidated on pawn loss"), Text->GetText().EqualTo(UTsabsPassengerHUD::FormatPassengerSpeed(0, false)));
    return true;
}
#endif
