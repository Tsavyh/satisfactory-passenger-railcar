#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TsabsPassengerHUD.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/PanelSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "UObject/UnrealType.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsNativeSpeedometerInspectionTest,
    "Tsabs.Passenger.Interface.NativeLocomotiveTemplateInspection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsNativeSpeedometerInspectionTest::RunTest(const FString& Parameters)
{
    auto* Class = Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/FactoryGame/Buildable/Vehicle/Train/Locomotive/UI/Widget_Locomotive_HUD.Widget_Locomotive_HUD_C")));
    if (!TestNotNull(TEXT("Native locomotive widget class"), Class)) return false;
    auto* Owner = Class->FindWidgetTreeOwningClass();
    UWidgetTree* Tree = Owner ? Owner->GetWidgetTreeArchetype() : nullptr;
    if (!TestNotNull(TEXT("Public template tree API"), Tree)) return false;
    TArray<UWidget*> Widgets;
    Tree->GetAllWidgets(Widgets);
    TArray<TSharedPtr<FJsonValue>> Items;
    for (UWidget* Widget : Widgets)
    {
        auto Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("name"), Widget->GetName());
        Item->SetStringField(TEXT("class"), Widget->GetClass()->GetPathName());
        Item->SetStringField(TEXT("parent"), GetNameSafe(Widget->GetParent()));
        auto ExportProperties = [](UObject* Object)
        {
            auto Props = MakeShared<FJsonObject>();
            if (Object) for (TFieldIterator<FProperty> P(Object->GetClass()); P; ++P)
            {
                const FString Name = P->GetName();
                if (Name.Contains(TEXT("Font")) || Name.Contains(TEXT("Brush")) || Name.Contains(TEXT("Color")) ||
                    Name.Contains(TEXT("Layout")) || Name.Contains(TEXT("Padding")) || Name.Contains(TEXT("Align")) ||
                    Name.Contains(TEXT("Size")) || Name.Contains(TEXT("Width")) || Name.Contains(TEXT("Height")) || Name.Contains(TEXT("Override")) || Name == TEXT("Text") || Name == TEXT("Visibility"))
                {
                    FString Value;
                    P->ExportTextItem_Direct(Value, P->ContainerPtrToValuePtr<void>(Object), nullptr, Object, PPF_None);
                    Props->SetStringField(Name, Value);
                }
            }
            return Props;
        };
        Item->SetObjectField(TEXT("properties"), ExportProperties(Widget));
        Item->SetStringField(TEXT("slotClass"), Widget->Slot ? Widget->Slot->GetClass()->GetName() : TEXT("None"));
        Item->SetObjectField(TEXT("slot"), ExportProperties(Widget->Slot));
        Items.Add(MakeShared<FJsonValueObject>(Item));
    }
    auto Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("boundary"), TEXT("Editor placeholder structure only; no native graphs or rendering certified"));
    Report->SetArrayField(TEXT("widgets"), Items);
    FString Json;
    FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
    TestTrue(TEXT("Persist template evidence"), FFileHelper::SaveStringToFile(Json,
        TEXT("C:/Modding/Satisfactory/Setup/PassengerHUD0111/native-template-inspection.json")));
    TestNotNull(TEXT("Native speed text exists"), Cast<UTextBlock>(Tree->FindWidget(TEXT("SpeedinKMHText"))));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerSpeedFormatTest,
    "Tsabs.Passenger.Interface.SpeedMagnitudeFormatting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPassengerSpeedFormatTest::RunTest(const FString& Parameters)
{
    UFunction* Function = UTsabsPassengerHUD::StaticClass()->FindFunctionByName(TEXT("FormatPassengerSpeed"));
    if (!TestNotNull(TEXT("Passenger read-only speed formatter is implemented"), Function)) return false;
    struct FParams { float SpeedCmPerSecond; bool bAvailable; FText ReturnValue; };
    auto Format = [&](float Speed, bool Available = true)
    {
        FParams Params{Speed, Available, FText::GetEmpty()};
        GetMutableDefault<UTsabsPassengerHUD>()->ProcessEvent(Function, &Params);
        return Params.ReturnValue;
    };
    FNumberFormattingOptions Options; Options.SetMaximumFractionalDigits(0); Options.SetUseGrouping(false);
    TestTrue(TEXT("1000 cm/s is36 km/h"), Format(1000.0f).EqualTo(FText::AsNumber(36, &Options)));
    TestTrue(TEXT("Reverse displays magnitude"), Format(-1000.0f).EqualTo(Format(1000.0f)));
    TestTrue(TEXT("Zero is a valid reading"), Format(0.0f).EqualTo(FText::AsNumber(0, &Options)));
    TestFalse(TEXT("Missing reading never fabricates zero"), Format(0.0f, false).EqualTo(Format(0.0f)));
    const uint32 InvalidBits[] = {0x7fc00000u, 0x7f800000u, 0xff800000u};
    for (uint32 Bits : InvalidBits)
    {
        float Value; FMemory::Memcpy(&Value, &Bits, sizeof(Value));
        TestTrue(TEXT("Nonfinite reading is unavailable"), Format(Value).EqualTo(Format(0.0f, false)));
    }
    return true;
}
#endif
