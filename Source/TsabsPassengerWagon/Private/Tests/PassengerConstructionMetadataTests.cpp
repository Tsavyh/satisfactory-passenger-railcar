#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"
#include "FGRecipe.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsConstructionMetadataTest, "Tsabs.Passenger.Content.ConstructionMetadata", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsConstructionMetadataTest::RunTest(const FString& Parameters)
{
    UClass* Class = LoadClass<UFGRecipe>(nullptr, TEXT("/Script/TsabsPassengerWagon.TsabsPassengerRecipe"));
    if (!TestNotNull(TEXT("Passenger recipe resolves"), Class)) return false;
    const UFGRecipe* Recipe = Class->GetDefaultObject<UFGRecipe>();
    // GetRecipeName/GetDisplayName SDK bodies are not a source of truth: inspect own CDO.
    const FBoolProperty* Override = FindFProperty<FBoolProperty>(Class, TEXT("mDisplayNameOverride"));
    const FTextProperty* Name = FindFProperty<FTextProperty>(Class, TEXT("mDisplayName"));
    if (TestNotNull(TEXT("Recipe name override property"), Override)) TestTrue(TEXT("Own recipe has explicit display name"), Override->GetPropertyValue_InContainer(Recipe));
    if (TestNotNull(TEXT("Recipe name text property"), Name)) TestEqual(TEXT("Own recipe label has no trial suffix"), Name->GetPropertyValue_InContainer(Recipe).ToString(), FString(TEXT("Vagón de pasajeros")));

    FString Text;
    const FString Path = FPaths::Combine(FPaths::ProjectModsDir(), TEXT("TsabsPassengerWagon/TsabsPassengerWagon.uplugin"));
    if (!TestTrue(TEXT("Read actual plugin metadata"), FFileHelper::LoadFileToString(Text, *Path))) return false;
    TSharedPtr<FJsonObject> Metadata;
    if (!TestTrue(TEXT("Parse actual plugin metadata"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Metadata) && Metadata.IsValid())) return false;
    TestEqual(TEXT("Candidate version integer"), Metadata->GetIntegerField(TEXT("Version")), 16);
    TestEqual(TEXT("Candidate display version"), Metadata->GetStringField(TEXT("VersionName")), FString(TEXT("0.1.15")));
    TestEqual(TEXT("Candidate semantic version"), Metadata->GetStringField(TEXT("SemVersion")), FString(TEXT("0.1.15")));
    return true;
}
#endif
