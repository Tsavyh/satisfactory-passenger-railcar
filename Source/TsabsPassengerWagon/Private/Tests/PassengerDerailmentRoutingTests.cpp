#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TsabsPassengerRailcar.h"
#include "TsabsPassengerAnchor.h"
#include "FGCharacterPlayer.h"
#include "FGConstructDisqualifier.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsDerailmentRoutingEditorTest,
    "Tsabs.Passenger.Derailment.RealFlagAndNormalRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTsabsDerailmentRoutingEditorTest::RunTest(const FString& Parameters)
{
    // Reflection writes are fixture-only. Production must only read IsDerailed.
    // The SDK native methods and FUseState setter are STUBS. This does not test
    // a successful native RPC, native disqualifier or physical rerail.
    AddInfo(TEXT("SDK boundary: real reflected flag + production dispatch/own guards only; native use-state setter, rerail/RPC and dismantle success are not certified."));
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    ATsabsPassengerRailcar* Car = World->SpawnActor<ATsabsPassengerRailcar>();
    FBoolProperty* Derailed = FindFProperty<FBoolProperty>(AFGRailroadVehicle::StaticClass(), TEXT("mIsDerailed"));
    FObjectProperty* Driver = FindFProperty<FObjectProperty>(AFGDriveablePawn::StaticClass(), TEXT("mDriver"));
    if (!TestNotNull(TEXT("Railcar fixture"), Car) | !TestNotNull(TEXT("Native replicated derail flag"), Derailed) |
        !TestNotNull(TEXT("Native driver data for own occupied guard"), Driver))
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        return false;
    }
    TestTrue(TEXT("Real derail flag is replicated"), Derailed->HasAnyPropertyFlags(CPF_Net));
    TestTrue(TEXT("Authority initializes pilot"), Car->EnsurePilotAnchor());
    ATsabsPassengerAnchor* Anchor = Car->GetPilotAnchor();
    if (!TestNotNull(TEXT("Single technical pilot"), Anchor))
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        return false;
    }
    FUseState Forged;
    Forged.bIsTraceHit = true;
    Forged.UseLocation = FVector(123.0, -456.0, 789.0);
    Forged.UseComponent = Car->GetPassengerMesh();
    const FText Normal = Car->GetLookAtDecription_Implementation(nullptr, Forged);
    TestFalse(TEXT("Normal invalid-requester feedback exists"), Normal.IsEmpty());
    // Forged hit/state cannot select rerail; only the actual replicated flag can.
    for (ENetRole Role : {ROLE_Authority, ROLE_AutonomousProxy, ROLE_SimulatedProxy})
    {
        Car->SetRole(Role);
        Derailed->SetPropertyValue_InContainer(Car, true);
        TestTrue(TEXT("Inline getter reads the actual reflected flag"), Car->IsDerailed());
        TestTrue(TEXT("Derailed remains selectable under current IsUseable"), Car->IsUseable_Implementation());
        const FText NativeStub = Car->AFGRailroadVehicle::GetLookAtDecription_Implementation(nullptr, Forged);
        TestTrue(TEXT("Derailed production description matches explicitly invoked SDK parent boundary, including non-host roles"),
            Car->GetLookAtDecription_Implementation(nullptr, Forged).EqualTo(NativeStub));
        FUseState Out;
        Out.bIsTraceHit = false;
        Out.UseLocation = FVector(9.0, 8.0, 7.0);
        Car->UpdateUseState_Implementation(nullptr, Forged.UseLocation, nullptr, Out);
        // No class assertion: SDK FUseState::SetUseState has no implementation.
        TestFalse(TEXT("Update does not invent a native trace hit in SDK"), Out.bIsTraceHit);
        Car->OnUse_Implementation(nullptr, Forged);
        TestTrue(TEXT("SDK no-op does not physically rerail (not a runtime success assertion)"), Car->IsDerailed());
        TestTrue(TEXT("Derailed use preserves pilot identity"), Car->GetPilotAnchor() == Anchor);
        TestFalse(TEXT("Derailed use never leaves a reservation"), Car->IsBoardingTransactionActive());
        TestNull(TEXT("Derailed null use creates no driver"), Anchor->GetDriver());
        TestFalse(TEXT("The railcar still refuses direct driving"), Car->CanDriverEnter(nullptr));
        // Existing guard, not native false-return stub: an actual occupied pilot
        // must add the MOD reason and retain the occupant for either real flag.
        Driver->SetObjectPropertyValue_InContainer(Anchor, GetMutableDefault<AFGCharacterPlayer>());
        TestTrue(TEXT("Own occupied-pilot guard remains active while derailed"), Car->HasProtectedPilot());
        TArray<TSubclassOf<UFGConstructDisqualifier>> Reasons;
        Car->GetDismantleDisqualifiers_Implementation(Reasons, {});
        TestTrue(TEXT("Own occupied disqualifier is preserved (not native derail disqualifier evidence)"),
            Reasons.Contains(UFGDDVehicleHasDriver::StaticClass()));
        TestFalse(TEXT("Occupied guard prevents release"), Car->ReleaseEmptyPilot());
        Derailed->SetPropertyValue_InContainer(Car, false);
        TestFalse(TEXT("Inline getter observes recovery to normal flag"), Car->IsDerailed());
        TestTrue(TEXT("Normal feedback returns after flag recovery"),
            Car->GetLookAtDecription_Implementation(nullptr, Forged).EqualTo(Normal));
        TestTrue(TEXT("Recovery does not remove occupied guard"), Car->HasProtectedPilot());
        Driver->SetObjectPropertyValue_InContainer(Anchor, nullptr);
        TestFalse(TEXT("Empty consistent pilot is no longer protected by own guard"), Car->HasProtectedPilot());
        Car->OnUse_Implementation(nullptr, Forged);
        TestTrue(TEXT("Normal invalid use preserves pilot"), Car->GetPilotAnchor() == Anchor);
        TestFalse(TEXT("Normal invalid use does not reserve"), Car->IsBoardingTransactionActive());
        TestNull(TEXT("Normal invalid use cannot create a driver"), Anchor->GetDriver());
    }
    Car->SetRole(ROLE_Authority);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
