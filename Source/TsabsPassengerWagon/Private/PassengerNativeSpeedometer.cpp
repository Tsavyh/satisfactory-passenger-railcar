#include "PassengerNativeSpeedometer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"

namespace
{
    void CopyVisualProperties(UObject* Destination, const UObject* Source, bool bSlot)
    {
        // Plain UMG data only. Do not clone owning trees/slots, delegates, native
        // bindings or UUserWidgets. Styles may reference original fonts/textures.
        for (TFieldIterator<FProperty> P(Source->GetClass()); P; ++P)
        {
            if (!P->HasAnyPropertyFlags(CPF_Edit) || CastField<FObjectPropertyBase>(*P) ||
                CastField<FDelegateProperty>(*P) || CastField<FMulticastDelegateProperty>(*P)) continue;
            if (P->GetOwnerClass() == UWidget::StaticClass())
            {
                const FName Name = P->GetFName();
                if (Name != TEXT("RenderTransform") && Name != TEXT("RenderTransformPivot") &&
                    Name != TEXT("Clipping") && Name != TEXT("RenderOpacity")) continue;
            }
            if (bSlot && P->GetOwnerClass() == UPanelSlot::StaticClass()) continue;
            P->CopyCompleteValue(P->ContainerPtrToValuePtr<void>(Destination), P->ContainerPtrToValuePtr<void>(Source));
        }
    }
}

UTextBlock* TsabsPassenger::ComposeNativeSpeedometer(UWidgetTree* Destination, UCanvasPanel* Canvas, UWidgetTree* NativeTemplate)
{
    if (!Destination || !Canvas || !NativeTemplate || Destination == NativeTemplate) return nullptr;
    // Retain only the native bottom-centre black panel, speed and unit.
    // The Speedometer stripe (scale/numerals/visor) and fixed title are not
    // composed or validated; no driving widget/event graph is copied.
    struct FNode { const TCHAR* Name; const TCHAR* Parent; UClass* Class; };
    const FNode Nodes[] = {
        {TEXT("Ov_SpeedFuel"), nullptr, UOverlay::StaticClass()},
        {TEXT("Overlay_2"), TEXT("Ov_SpeedFuel"), UOverlay::StaticClass()},
        {TEXT("PowerBG"), TEXT("Overlay_2"), UImage::StaticClass()},
        {TEXT("SizeBox_3"), TEXT("Overlay_2"), USizeBox::StaticClass()},
        {TEXT("VehicleUIContainer"), TEXT("SizeBox_3"), UVerticalBox::StaticClass()},
        {TEXT("HorizontalBox_0"), TEXT("VehicleUIContainer"), UHorizontalBox::StaticClass()},
        {TEXT("VerticalBox_0"), TEXT("HorizontalBox_0"), UVerticalBox::StaticClass()},
        {TEXT("CurrentSpeedSlot"), TEXT("VerticalBox_0"), UHorizontalBox::StaticClass()},
        {TEXT("SpeedinKMHText"), TEXT("CurrentSpeedSlot"), UTextBlock::StaticClass()},
        {TEXT("Spacer_131"), TEXT("CurrentSpeedSlot"), USpacer::StaticClass()},
        {TEXT("TextBlock_3"), TEXT("CurrentSpeedSlot"), UTextBlock::StaticClass()}
    };
    // Validate everything before adding even one child. A changed asset fails
    // closed instead of adding an invented panel or a partial driving display.
    // FindWidget sees only attached widgets; any direct-Outer object can occupy
    // a ConstructWidget name, even if disconnected or of an incompatible class.
    TArray<UObject*> DestinationOwned;
    GetObjectsWithOuter(Destination, DestinationOwned, false);
    for (const FNode& Node : Nodes)
    {
        UWidget* Source = NativeTemplate->FindWidget(Node.Name);
        if (!Source || Source->GetClass() != Node.Class || !Source->Slot || Destination->FindWidget(Node.Name)) return nullptr;
        if (DestinationOwned.ContainsByPredicate([&Node](const UObject* Object) { return Object->GetFName() == Node.Name; })) return nullptr;
        if (Node.Parent && (!Source->GetParent() || Source->GetParent()->GetFName() != Node.Parent)) return nullptr;
        // AddChild allocates the parent's exact slot class. Validate every
        // source slot against that API before constructing or attaching any
        // destination widget; a malformed interior slot is external data too.
        const UPanelWidget* ExpectedParent = Node.Parent ? Cast<UPanelWidget>(NativeTemplate->FindWidget(Node.Parent)) : Canvas;
        if (!ExpectedParent || Source->Slot->GetClass() != ExpectedParent->GetSlotClass()) return nullptr;
    }
    TMap<FName, UWidget*> Copies;
    for (const FNode& Node : Nodes)
    {
        UWidget* Source = NativeTemplate->FindWidget(Node.Name);
        UWidget* Copy = Destination->ConstructWidget<UWidget>(Node.Class, Node.Name);
        CopyVisualProperties(Copy, Source, false);
        Copy->SetVisibility(ESlateVisibility::HitTestInvisible);
        UPanelWidget* Parent = Node.Parent ? CastChecked<UPanelWidget>(Copies.FindChecked(Node.Parent)) : Canvas;
        UPanelSlot* Slot = Parent->AddChild(Copy);
        check(Slot->GetClass() == Source->Slot->GetClass());
        CopyVisualProperties(Slot, Source->Slot, true);
        Copies.Add(Node.Name, Copy);
    }
    return CastChecked<UTextBlock>(Copies.FindChecked(TEXT("SpeedinKMHText")));
}
