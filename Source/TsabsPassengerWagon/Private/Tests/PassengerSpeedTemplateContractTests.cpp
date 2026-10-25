#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "PassengerNativeSpeedometer.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Serialization/ObjectWriter.h"
#include "UObject/UObjectHash.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerSpeedTemplateContractTest,
    "Tsabs.Passenger.Interface.SpeedTemplateFailsClosedWithoutNativeMutationOrDuplicate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPassengerSpeedTemplateContractTest::RunTest(const FString& Parameters)
{
    auto* Class = Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Locomotive/UI/Widget_Locomotive_HUD.Widget_Locomotive_HUD_C")));
    auto* Owner = Class ? Class->FindWidgetTreeOwningClass() : nullptr;
    auto* Native = Owner ? Owner->GetWidgetTreeArchetype() : nullptr;
    if (!TestNotNull(TEXT("Public native template"), Native)) return false;
    auto* Destination = NewObject<UWidgetTree>();
    auto* Canvas = Destination->ConstructWidget<UCanvasPanel>(); Destination->RootWidget = Canvas;
    auto* Unrelated = Destination->ConstructWidget<UTextBlock>(); Canvas->AddChild(Unrelated);
    auto* MissingContract = NewObject<UWidgetTree>();
    TestNull(TEXT("Missing asset is explicitly unavailable"), TsabsPassenger::ComposeNativeSpeedometer(Destination, Canvas, nullptr));
    TestNull(TEXT("Changed/incomplete contract is explicitly unavailable"), TsabsPassenger::ComposeNativeSpeedometer(Destination, Canvas, MissingContract));
    TestEqual(TEXT("Failure adds no guessed or partial panel"), Canvas->GetChildrenCount(), 1);
    TestTrue(TEXT("Failure preserves unrelated UI"), Unrelated->GetParent() == Canvas);
    auto* NativeText = CastChecked<UTextBlock>(Native->FindWidget(TEXT("SpeedinKMHText")));
    const FText OriginalText = NativeText->GetText();
    auto* NativeImage = CastChecked<UImage>(Native->FindWidget(TEXT("PowerBG")));
    const ESlateVisibility OriginalVisibility = NativeImage->GetVisibility();
    UTextBlock* Copy = TsabsPassenger::ComposeNativeSpeedometer(Destination, Canvas, Native);
    if (!TestNotNull(TEXT("Valid plain UMG speed subtree composes"), Copy)) return false;
    const int32 Count = Canvas->GetChildrenCount();
    TestNull(TEXT("Repeated composition never creates a second speed subtree"), TsabsPassenger::ComposeNativeSpeedometer(Destination, Canvas, Native));
    TestEqual(TEXT("Duplicate prevention is atomic"), Canvas->GetChildrenCount(), Count);
    Copy->SetText(FText::FromString(TEXT("Passenger changed only its own instance")));
    TestTrue(TEXT("Native template text remains unchanged"), NativeText->GetText().EqualTo(OriginalText));
    TestEqual(TEXT("Native template visibility remains unchanged"), NativeImage->GetVisibility(), OriginalVisibility);
    TestTrue(TEXT("Success still preserves unrelated UI"), Unrelated->GetParent() == Canvas);

    // Pin the observed placeholder contract to the real panel slot APIs. Start
    // with a deep interior node: the old code passes validation and asserts
    // only AFTER attaching most of the destination subtree.
    struct FSlotContract { const TCHAR* Name; UClass* SlotClass; };
    const FSlotContract Contracts[] = {
        {TEXT("SpeedinKMHText"), UHorizontalBoxSlot::StaticClass()},
        {TEXT("Ov_SpeedFuel"), UCanvasPanelSlot::StaticClass()},
        {TEXT("Overlay_2"), UOverlaySlot::StaticClass()},
        {TEXT("PowerBG"), UOverlaySlot::StaticClass()},
        {TEXT("SizeBox_3"), UOverlaySlot::StaticClass()},
        {TEXT("VehicleUIContainer"), USizeBoxSlot::StaticClass()},
        {TEXT("HorizontalBox_0"), UVerticalBoxSlot::StaticClass()},
        {TEXT("VerticalBox_0"), UHorizontalBoxSlot::StaticClass()},
        {TEXT("CurrentSpeedSlot"), UVerticalBoxSlot::StaticClass()},
        {TEXT("Spacer_131"), UHorizontalBoxSlot::StaticClass()},
        {TEXT("TextBlock_3"), UHorizontalBoxSlot::StaticClass()}
    };
    auto Snapshot = [](UObject* Root)
    {
        TArray<UObject*> Objects; GetObjectsWithOuter(Root, Objects, true); Objects.Add(Root);
        TMap<UObject*, TArray<uint8>> Result;
        for (UObject* Object : Objects)
        {
            TArray<uint8>& Bytes = Result.Add(Object);
            FObjectWriter Writer(Object, Bytes, false, false, false, PPF_DuplicateVerbatim);
        }
        return Result;
    };
    const auto NativeBefore = Snapshot(Native);
    for (const FSlotContract& Contract : Contracts)
    {
        UWidget* Original = Native->FindWidget(Contract.Name);
        if (!TestNotNull(TEXT("Inspected contract widget exists"), Original) ||
            !TestNotNull(TEXT("Inspected contract parent exists"), Original->GetParent())) return false;
        TestTrue(FString::Printf(TEXT("%s slot agrees with inspected map and parent API"), Contract.Name),
            Original->Slot->GetClass() == Contract.SlotClass && Original->GetParent()->GetSlotClass() == Contract.SlotClass);
        AddInfo(FString::Printf(TEXT("Slot contract %s parent=%s slot=%s"), Contract.Name,
            *Original->GetParent()->GetName(), *Original->Slot->GetClass()->GetName()));
        auto* Fixture = DuplicateObject<UWidgetTree>(Native, GetTransientPackage());
        UWidget* Corrupt = Fixture->FindWidget(Contract.Name);
        UPanelWidget* OriginalParent = Corrupt->GetParent();
        UClass* IncompatibleClass = Contract.SlotClass == UCanvasPanelSlot::StaticClass()
            ? UHorizontalBoxSlot::StaticClass() : UCanvasPanelSlot::StaticClass();
        auto* Incompatible = NewObject<UPanelSlot>(Fixture, IncompatibleClass);
        Incompatible->Parent = OriginalParent; Incompatible->Content = Corrupt;
        Corrupt->Slot = Incompatible;
        TestTrue(TEXT("Fixture changes only slot class, retaining compatible actual parent and name"),
            Corrupt->GetParent() == OriginalParent && Corrupt->GetFName() == Contract.Name &&
            OriginalParent->GetFName() == Original->GetParent()->GetFName() &&
            Corrupt->Slot->GetClass() != OriginalParent->GetSlotClass());
        auto* RejectDestination = NewObject<UWidgetTree>();
        auto* RejectCanvas = RejectDestination->ConstructWidget<UCanvasPanel>(); RejectDestination->RootWidget = RejectCanvas;
        auto* Keep = RejectDestination->ConstructWidget<UTextBlock>(); RejectCanvas->AddChild(Keep);
        Keep->SetText(FText::FromString(TEXT("Unrelated destination stays untouched")));
        const auto DestinationBefore = Snapshot(RejectDestination);
        const auto FixtureBefore = Snapshot(Fixture);
        AddInfo(FString::Printf(TEXT("Reject incompatible slot before mutation: %s"), Contract.Name));
        TestNull(FString::Printf(TEXT("Incompatible %s slot is safely rejected"), Contract.Name),
            TsabsPassenger::ComposeNativeSpeedometer(RejectDestination, RejectCanvas, Fixture));
        TestTrue(TEXT("Rejected destination serialization and owned-object set are unchanged (including orphan widgets)"),
            DestinationBefore.OrderIndependentCompareEqual(Snapshot(RejectDestination)));
        TestTrue(TEXT("Rejected cloned template remains unchanged"), FixtureBefore.OrderIndependentCompareEqual(Snapshot(Fixture)));
        TestTrue(TEXT("Native template archetype remains byte-for-byte unchanged"), NativeBefore.OrderIndependentCompareEqual(Snapshot(Native)));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerMinimalSpeedPanelTest,
    "Tsabs.Passenger.Interface.MinimalSpeedPanelIgnoresMissingOrCorruptDecoration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPassengerMinimalSpeedPanelTest::RunTest(const FString& Parameters)
{
    auto* Class = Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Locomotive/UI/Widget_Locomotive_HUD.Widget_Locomotive_HUD_C")));
    auto* Owner = Class ? Class->FindWidgetTreeOwningClass() : nullptr;
    auto* Native = Owner ? Owner->GetWidgetTreeArchetype() : nullptr;
    if (!TestNotNull(TEXT("Native template for minimal panel"), Native)) return false;
    auto Snapshot = [](UObject* Root)
    {
        TArray<UObject*> Objects; GetObjectsWithOuter(Root, Objects, true); Objects.Add(Root);
        TMap<UObject*, TArray<uint8>> Result;
        for (UObject* Object : Objects)
        {
            TArray<uint8>& Bytes = Result.Add(Object);
            FObjectWriter Writer(Object, Bytes, false, false, false, PPF_DuplicateVerbatim);
        }
        return Result;
    };
    const auto NativeBefore = Snapshot(Native);
    const TCHAR* Decorations[] = {TEXT("Speedometer"), TEXT("mSpeedometerBG"), TEXT("VehicleVisorBG"), TEXT("TextBlock_2")};
    // Independent fixtures cover absent nodes and incompatible decorative slots;
    // neither participates in the retained-panel contract.
    for (const TCHAR* Name : Decorations)
    {
        for (int32 Mode = 0; Mode < 2; ++Mode)
        {
            auto* Fixture = DuplicateObject<UWidgetTree>(Native, GetTransientPackage());
            UWidget* Decoration = Fixture->FindWidget(Name);
            if (!TestNotNull(TEXT("Decoration fixture exists before removal/corruption"), Decoration)) return false;
            if (Mode == 0)
            {
                Decoration->RemoveFromParent();
                TestNull(TEXT("Decoration really absent from source tree"), Fixture->FindWidget(Name));
            }
            else
            {
                auto* WrongSlot = NewObject<UCanvasPanelSlot>(Fixture);
                WrongSlot->Parent = Decoration->GetParent(); WrongSlot->Content = Decoration;
                Decoration->Slot = WrongSlot;
            }
            const auto FixtureBefore = Snapshot(Fixture);
            auto* Destination = NewObject<UWidgetTree>();
            auto* Canvas = Destination->ConstructWidget<UCanvasPanel>(); Destination->RootWidget = Canvas;
            UTextBlock* Speed = TsabsPassenger::ComposeNativeSpeedometer(Destination, Canvas, Fixture);
            TestNotNull(FString::Printf(TEXT("MinimalSpeedPanel accepts %s decoration %s"), Mode ? TEXT("corrupt") : TEXT("missing"), Name), Speed);
            TestNotNull(TEXT("MinimalSpeedPanel contains real black PowerBG"), Destination->FindWidget(TEXT("PowerBG")));
            TestNotNull(TEXT("MinimalSpeedPanel contains km/h unit"), Destination->FindWidget(TEXT("TextBlock_3")));
            TArray<UObject*> Owned; GetObjectsWithOuter(Destination, Owned, true);
            for (const TCHAR* Excluded : Decorations)
            {
                TestNull(TEXT("MinimalSpeedPanel excludes every decoration"), Destination->FindWidget(Excluded));
                TestFalse(TEXT("MinimalSpeedPanel allocates no orphan decoration"),
                    Owned.ContainsByPredicate([Excluded](const UObject* Object) { return Object->GetFName() == Excluded; }));
            }
            TestTrue(TEXT("Composition never mutates malformed/partial source or owned-object set"),
                FixtureBefore.OrderIndependentCompareEqual(Snapshot(Fixture)));
            TestTrue(TEXT("Original native template stays byte-for-byte immutable"),
                NativeBefore.OrderIndependentCompareEqual(Snapshot(Native)));
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerSpeedOwnedNameCollisionTest,
    "Tsabs.Passenger.Interface.SpeedOwnedDisconnectedNameCollisionRejectsBeforeMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPassengerSpeedOwnedNameCollisionTest::RunTest(const FString& Parameters)
{
    auto* Class = Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Locomotive/UI/Widget_Locomotive_HUD.Widget_Locomotive_HUD_C")));
    auto* Owner = Class ? Class->FindWidgetTreeOwningClass() : nullptr;
    auto* Native = Owner ? Owner->GetWidgetTreeArchetype() : nullptr;
    if (!TestNotNull(TEXT("Native template for owned-name collision"), Native)) return false;
    auto Snapshot = [](UObject* Root)
    {
        TArray<UObject*> Objects; GetObjectsWithOuter(Root, Objects, true); Objects.Add(Root);
        TMap<UObject*, TArray<uint8>> Result;
        for (UObject* Object : Objects)
        {
            TArray<uint8>& Bytes = Result.Add(Object);
            FObjectWriter Writer(Object, Bytes, false, false, false, PPF_DuplicateVerbatim);
        }
        return Result;
    };
    const auto NativeBefore = Snapshot(Native);
    // The final retained node catches any earlier allocations/attachments.
    // Exercise same-class first: stop on its old nonfatal failure rather than
    // reaching the incompatible-class NewObject fatal in the unfixed code.
    UClass* CollisionClasses[] = {UTextBlock::StaticClass(), UImage::StaticClass(), UWidgetTree::StaticClass()};
    for (UClass* CollisionClass : CollisionClasses)
    {
        auto* Destination = NewObject<UWidgetTree>();
        auto* Canvas = Destination->ConstructWidget<UCanvasPanel>(); Destination->RootWidget = Canvas;
        auto* Keep = Destination->ConstructWidget<UTextBlock>(); Canvas->AddChild(Keep);
        Keep->SetText(FText::FromString(TEXT("Unrelated attached destination UI")));
        auto* Occupant = NewObject<UObject>(Destination, CollisionClass, TEXT("TextBlock_3"));
        if (auto* Text = Cast<UTextBlock>(Occupant)) Text->SetText(FText::FromString(TEXT("Preserve disconnected text")));
        TestTrue(TEXT("Collision object has direct destination Outer"), Occupant->GetOuter() == Destination);
        TestNull(TEXT("Owned collision is absent from attached widget tree"), Destination->FindWidget(TEXT("TextBlock_3")));
        if (auto* Widget = Cast<UWidget>(Occupant)) TestNull(TEXT("Owned collision has no panel parent"), Widget->GetParent());
        const auto DestinationBefore = Snapshot(Destination);
        AddInfo(FString::Printf(TEXT("Reject disconnected owned TextBlock_3 class=%s before all mutation"), *CollisionClass->GetName()));
        const bool bRejected = TestNull(TEXT("Owned name collision is safely rejected regardless of class"),
            TsabsPassenger::ComposeNativeSpeedometer(Destination, Canvas, Native));
        const bool bDestinationUnchanged = TestTrue(TEXT("Rejection preserves destination serialization and all owned objects including disconnected occupant"),
            DestinationBefore.OrderIndependentCompareEqual(Snapshot(Destination)));
        const bool bNativeUnchanged = TestTrue(TEXT("Rejection preserves native template serialization and all owned objects"),
            NativeBefore.OrderIndependentCompareEqual(Snapshot(Native)));
        if (!bRejected || !bDestinationUnchanged || !bNativeUnchanged) return false;
    }
    return true;
}
#endif
