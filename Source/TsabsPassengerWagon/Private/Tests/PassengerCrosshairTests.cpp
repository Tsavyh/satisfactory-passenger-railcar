#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#if __has_include("PassengerCrosshair.h")
#include "PassengerCrosshair.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Components/CanvasPanel.h"
#include "TsabsPassengerHUD.h"
#include "TsabsPassengerAnchor.h"
#include "FGHUD.h"
#include "UObject/UnrealType.h"
namespace PassengerCrosshairTest
{
    const FMulticastScriptDelegate* ReceivedDelegate;
    bool ReceivedVisibility;
    UTexture2D* ReceivedTexture;
    FLinearColor ReceivedColor;
    int32 ReceivedCalls;
    void Receive(const FMulticastScriptDelegate& Delegate, bool Visible, UTexture2D* Texture, FLinearColor Color)
    {
        ReceivedDelegate = &Delegate; ReceivedVisibility = Visible; ReceivedTexture = Texture; ReceivedColor = Color; ++ReceivedCalls;
    }
}
#endif
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsPassengerCrosshairTest,
    "Tsabs.Passenger.Interface.CrosshairSuppressionIsLocalReversibleAndForwards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsPassengerCrosshairTest::RunTest(const FString& Parameters)
{
#if !__has_include("PassengerCrosshair.h")
    AddError(TEXT("Passenger local crosshair forwarding/suppression helper does not exist"));
    return false;
#else
    using namespace PassengerCrosshairTest;
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    auto* PC = World->SpawnActor<APlayerController>();
    PC->SetPlayer(NewObject<ULocalPlayer>(GEngine)); World->AddController(PC);
    auto* Anchor = World->SpawnActor<ATsabsPassengerAnchor>(); PC->SetPawn(Anchor);
    auto* NativeHUD = World->SpawnActor<AFGHUD>(); NativeHUD->SetOwner(PC);
    auto* OtherHUD = World->SpawnActor<AFGHUD>();
    // Explicit reflected fixture injection: SDK AddPawnHUD is an empty stub.
    // This is UMG/owned policy/real SML forwarding, not native HUD mutation.
    auto* HUDProperty = FindFProperty<FObjectPropertyBase>(APlayerController::StaticClass(), TEXT("MyHUD"));
    auto* PawnHUDProperty = FindFProperty<FObjectPropertyBase>(AFGHUD::StaticClass(), TEXT("mPawnHUD"));
    if (!TestNotNull(TEXT("Engine HUD fixture field"), HUDProperty) || !TestNotNull(TEXT("SDK reflected pawn HUD fixture field"), PawnHUDProperty)) return false;
    HUDProperty->SetObjectPropertyValue_InContainer(PC, NativeHUD);
    auto* Widget = CreateWidget<UTsabsPassengerHUD>(PC, UTsabsPassengerHUD::StaticClass());
    auto* Host = NewObject<UCanvasPanel>(PC); Host->AddChild(Widget);
    auto* Texture = NewObject<UTexture2D>();
    const FLinearColor Color(0.2f,0.3f,0.4f,0.5f);
    auto Forward = [&](const FMulticastScriptDelegate& Delegate, bool Visible)
    {
        using FScope = TCallScope<TsabsPassenger::FCrosshairUpdateSignature>;
        int32 ThirdPartyCalls = 0;
        TArray<TSharedPtr<FScope::HookFunc>> Handlers;
        Handlers.Add(MakeShared<FScope::HookFunc>(&TsabsPassenger::ForwardPassengerCrosshair));
        Handlers.Add(MakeShared<FScope::HookFunc>([&](FScope& Scope, const FMulticastScriptDelegate& D, bool V, UTexture2D* T, FLinearColor C)
        { ++ThirdPartyCalls; Scope(D,V,T,C); }));
        ReceivedCalls = 0;
        FScope Scope(&Handlers, &Receive); Scope(Delegate, Visible, Texture, Color);
        TestEqual(TEXT("Original forwarded exactly once"), ReceivedCalls, 1);
        TestEqual(TEXT("Other subscribers not cancelled"), ThirdPartyCalls, 1);
        TestTrue(TEXT("Delegate identity, texture and color untouched"), ReceivedDelegate == &Delegate && ReceivedTexture == Texture && ReceivedColor == Color);
        return ReceivedVisibility;
    };
    Widget->BeginCrosshairSuppression();
    TestTrue(TEXT("Initialize before AddPawnHUD assignment never suppresses"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    PawnHUDProperty->SetObjectPropertyValue_InContainer(NativeHUD, Widget);
    Widget->BeginCrosshairSuppression();
    TestFalse(TEXT("Only current local passenger visibility is hidden"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    TestTrue(TEXT("Other HUD delegate remains unchanged"), Forward(OtherHUD->mOnCrosshairUpdated, true));
    TestFalse(TEXT("Already-hidden crosshair never becomes visible"), Forward(NativeHUD->mOnCrosshairUpdated, false));
    const uint64 Token = Widget->CrosshairToken;
    Widget->BeginCrosshairSuppression();
    TestEqual(TEXT("Repeated begin retains exact own token"), Widget->CrosshairToken, Token);
    HUDProperty->SetObjectPropertyValue_InContainer(PC, OtherHUD);
    TestTrue(TEXT("HUD replacement invalidates old scope immediately"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    HUDProperty->SetObjectPropertyValue_InContainer(PC, NativeHUD);
    auto* Next = CreateWidget<UTsabsPassengerHUD>(PC, UTsabsPassengerHUD::StaticClass());
    PawnHUDProperty->SetObjectPropertyValue_InContainer(NativeHUD, Next); Next->BeginCrosshairSuppression();
    Widget->EndCrosshairSuppression(); Widget->EndCrosshairSuppression();
    TestFalse(TEXT("Stale removal cannot remove a replacement registration"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    Next->EndCrosshairSuppression();
    TestTrue(TEXT("Exit returns to original visible input"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    TestFalse(TEXT("Exit preserves externally hidden input"), Forward(NativeHUD->mOnCrosshairUpdated, false));
    Next->BeginCrosshairSuppression(); Host->AddChild(Next);
    PC->SetPawn(nullptr);
    PC->OnPossessedPawnChanged.Broadcast(Anchor, nullptr);
    TestEqual(TEXT("Pawn notification retires token without Tick"), Next->CrosshairToken, uint64(0));
    TestNull(TEXT("Pawn notification detaches owned widget without Tick"), Next->GetParent());
    TestTrue(TEXT("After pawn change visibility is restored by forwarding policy"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    Widget->EndCrosshairSuppression(); Next->EndCrosshairSuppression();
    PC->SetPawn(Anchor);
    Next->BeginCrosshairSuppression();
    TestTrue(TEXT("Destruct fixture actually owns a registration"), Next->CrosshairToken != 0);
    const uint64 RetiredToken = Next->CrosshairToken;
    Next->NativeDestruct();
    TestEqual(TEXT("NativeDestruct is an idempotent cleanup fallback"), Next->CrosshairToken, uint64(0));
    TestTrue(TEXT("Destruct no longer suppresses native event input"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    // Invoke the actual lifecycle overrides on the SAME initialized UObject.
    // No new SetupPlayerInputComponent/AddPawnHUD/Begin call is made here.
    // This is not a full Slate rebuild or a runtime visual/native-hook test.
    Next->NativeConstruct();
    TestTrue(TEXT("Reconstruct keeps the exact controller, pawn and current pawn-HUD fixture"),
        Next->GetOwningPlayer() == PC && PC->GetPawn() == Anchor && NativeHUD->GetPawnHUD() == Next);
    TestTrue(TEXT("NativeConstruct rearms with a fresh owned token"), Next->CrosshairToken != 0 && Next->CrosshairToken != RetiredToken);
    TestFalse(TEXT("Reconstructed same widget hides real forwarded crosshair input again"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    const uint64 ReconstructedToken = Next->CrosshairToken;
    Next->NativeConstruct();
    TestEqual(TEXT("Repeated NativeConstruct is idempotent"), Next->CrosshairToken, ReconstructedToken);
    TestEqual(TEXT("Reconstruct has exactly one pawn-change subscription for the widget"),
        PC->OnPossessedPawnChanged.GetAllObjects().FilterByPredicate([&](UObject* Object) { return Object == Next; }).Num(), 1);
    Widget->NativeConstruct();
    TestEqual(TEXT("Stale pawn HUD cannot acquire on NativeConstruct"), Widget->CrosshairToken, uint64(0));
    TestFalse(TEXT("Stale construct cannot disturb the current widget token"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    Next->NativeDestruct();
    TestEqual(TEXT("Reconstructed widget destruct retires its own token"), Next->CrosshairToken, uint64(0));
    TestEqual(TEXT("Reconstructed widget destruct removes its pawn-change subscription"),
        PC->OnPossessedPawnChanged.GetAllObjects().FilterByPredicate([&](UObject* Object) { return Object == Next; }).Num(), 0);
    TestTrue(TEXT("No duplicate registry token survives reconstruct/destruct"), Forward(NativeHUD->mOnCrosshairUpdated, true));
    PC->SetPawn(nullptr);
    Next->NativeConstruct();
    TestEqual(TEXT("Stale pawn cannot acquire on NativeConstruct"), Next->CrosshairToken, uint64(0));
    return true;
#endif
}
#endif
