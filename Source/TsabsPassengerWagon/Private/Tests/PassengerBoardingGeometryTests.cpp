#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include <limits>
#include "PassengerBoardingGeometry.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTsabsBoardingGeometryTest,
    "Tsabs.Passenger.Boarding.BodyReachAndVisibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTsabsBoardingGeometryTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
    auto MakeBox = [&](FVector Location, FVector Extent)
    {
        AActor* Actor = World->SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Box);
        Box->SetBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();
        Actor->SetActorLocation(Location);
        return Box;
    };
    // Real engine collision boxes. NOT a vanilla placeholder/chassis or a passenger trip.
    UBoxComponent* Body = MakeBox(FVector::ZeroVector, FVector(2000.0, 100.0, 150.0));
    const FVector NearEnd(1800.0, 200.0, 0.0);
    const FRotator TowardBody(0.0, -90.0, 0.0);
    auto Reach = [&](float Range, FVector CharacterOrigin, FVector CameraOrigin, FRotator Look)
    {
        return TsabsPassenger::CanReachBoardingBody(Body, nullptr, Range, CharacterOrigin, CameraOrigin, Look);
    };
    TestTrue(TEXT("Long vehicle centre may be outside range"), NearEnd.Size() > 150.0);
    TestTrue(TEXT("Near chassis end is in reach despite distant centre"), Reach(150.0f, NearEnd, NearEnd, TowardBody));
    TestFalse(TEXT("Standard use range is not expanded"), Reach(99.0f, NearEnd, NearEnd, TowardBody));
    TestFalse(TEXT("Looking away denies use"), Reach(150.0f, NearEnd, NearEnd, FRotator(0.0, 90.0, 0.0)));
    TestFalse(TEXT("Near camera cannot authorize distant character"), Reach(150.0f, FVector(1800.0, 5000.0, 0.0), NearEnd, TowardBody));
    TestFalse(TEXT("Camera too far cannot authorize view hit"), Reach(150.0f, NearEnd, FVector(1800.0, 5000.0, 0.0), TowardBody));
    TestFalse(TEXT("Zero use range fails closed"), Reach(0.0f, NearEnd, NearEnd, TowardBody));
    TestFalse(TEXT("NaN range fails closed"), Reach(std::numeric_limits<float>::quiet_NaN(), NearEnd, NearEnd, TowardBody));
    TestFalse(TEXT("Infinite range fails closed"), Reach(std::numeric_limits<float>::infinity(), NearEnd, NearEnd, TowardBody));
    TestFalse(TEXT("Nonfinite camera fails closed"), Reach(150.0f, NearEnd, FVector(std::numeric_limits<double>::quiet_NaN(), 0, 0), TowardBody));
    UBoxComponent* Obstacle = MakeBox(FVector(1800.0, 160.0, 0.0), FVector(20.0, 10.0, 100.0));
    TestFalse(TEXT("World obstruction blocks otherwise valid body hit"), Reach(150.0f, NearEnd, NearEnd, TowardBody));
    Obstacle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestTrue(TEXT("Removing obstruction restores geometry reach"), Reach(150.0f, NearEnd, NearEnd, TowardBody));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestFalse(TEXT("No real body collision is not a centre-distance fallback"), Reach(150.0f, NearEnd, NearEnd, TowardBody));
    TestFalse(TEXT("Missing body fails closed"), TsabsPassenger::CanReachBoardingBody(nullptr, nullptr, 150.0f, NearEnd, NearEnd, TowardBody));
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
