#include "TsabsPassengerWorldModule.h"
#include "PassengerContentPolicy.h"
#include "TsabsPassengerRecipe.h"
#include "TsabsPassengerWagonLog.h"
#include "Engine/World.h"
#include "FGRecipeManager.h"
#include "Registry/ModContentRegistry.h"
#include "Patching/NativeHookManager.h"
#include "Templates/UnrealTemplate.h"

namespace
{
    // Exactly two subscriptions per module lifetime, not per world. World/manager
    // objects are never captured or retained. The weak routing array is pruned
    // on bind, teardown, dispatch and shutdown; it cannot accumulate dead worlds.
    TArray<TWeakObjectPtr<UTsabsPassengerWorldModule>> AvailabilityModules;
    FDelegateHandle AddRecipesHandle;
    FDelegateHandle RemoveRecipesHandle;
    bool bAvailabilityHooksRunning = false;
}

void UTsabsPassengerWorldModule::InstallAvailabilityHooks()
{
    check(IsInGameThread());
    if (bAvailabilityHooksRunning) return;
    bAvailabilityHooksRunning = true;
    if (!WITH_EDITOR)
    {
        // Compile the actual hook expressions in Editor without detouring SDK stubs.
        AddRecipesHandle = SUBSCRIBE_METHOD_AFTER(AFGRecipeManager::AddAvailableRecipes,
            [](AFGRecipeManager* Manager, TArray<TSubclassOf<UFGRecipe>>)
            {
                ForwardAvailability(Manager);
            });
        RemoveRecipesHandle = SUBSCRIBE_METHOD_AFTER(AFGRecipeManager::RemoveAvailableRecipes,
            [](AFGRecipeManager* Manager, const TArray<TSubclassOf<UFGRecipe>>&)
            {
                // Remove arguments may alias the already-mutated native array.
                ForwardAvailability(Manager);
            });
    }
}

void UTsabsPassengerWorldModule::ForwardAvailability(AFGRecipeManager* Manager)
{
    if (!IsInGameThread() || !bAvailabilityHooksRunning) return;
    AvailabilityModules.RemoveAll([](const auto& Weak) { return !Weak.Get(); });
    // This is OUR weak routing list, not SML's live handler list. A callback can
    // tear down any world; subsequent snapshot entries revalidate weak lifetime
    // and ReconcileAvailability checks bTearingDown before any manager access.
    const auto Snapshot = AvailabilityModules;
    for (const auto& Weak : Snapshot)
        if (UTsabsPassengerWorldModule* Module = Weak.Get()) Module->ReconcileAvailability(Manager);
}

void UTsabsPassengerWorldModule::ShutdownAvailabilityHooks()
{
    check(IsInGameThread());
    bAvailabilityHooksRunning = false;
    const auto Snapshot = AvailabilityModules;
    for (const auto& Weak : Snapshot)
        if (UTsabsPassengerWorldModule* Module = Weak.Get()) Module->ReleaseAvailabilityHooks();
    AvailabilityModules.Empty();
    // Ordinary FEngineLoop::Exit -> UnloadModulesAtShutdown runs after the tick
    // stack has returned, not from a native recipe callback. Never call this on
    // world cleanup or at the end of our callback: neither proves SML is idle.
    // Dynamic DLL unload/hot reload during gameplay is explicitly unsupported.
    if (AddRecipesHandle.IsValid())
    {
        UNSUBSCRIBE_METHOD(AFGRecipeManager::AddAvailableRecipes, AddRecipesHandle);
        AddRecipesHandle.Reset();
    }
    if (RemoveRecipesHandle.IsValid())
    {
        UNSUBSCRIBE_METHOD(AFGRecipeManager::RemoveAvailableRecipes, RemoveRecipesHandle);
        RemoveRecipesHandle.Reset();
    }
}

UTsabsPassengerWorldModule::UTsabsPassengerWorldModule()
{
    bRootModule = true;
}

void UTsabsPassengerWorldModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
    if (ContentState.bTearingDown) return;
    UWorld* World = GetWorld();
    const FName Owner = GetOwnerModReference();
    const bool bValidWorld = IsValid(World) && World->IsGameWorld();
    const bool bValidOwner = Owner == FName(TEXT("TsabsPassengerWagon"));
    // The base implementation dereferences the world. Validate before Super.
    if (!bValidWorld || !bValidOwner)
    {
        UE_LOG(LogTsabsPassengerWagon, Error, TEXT("TSABS_CONTENT invalid-context phase=%d world=%d owner=%s"),
            static_cast<int32>(Phase), bValidWorld, *Owner.ToString());
        return;
    }
    Super::DispatchLifecycleEvent(Phase);
    const TSubclassOf<UFGRecipe> Recipe = UTsabsPassengerRecipe::StaticClass();
    if (TsabsPassenger::ShouldRegisterPassengerRecipe(Phase, bValidWorld, bValidOwner, ContentState.bRegistrationAttempted))
    {
        UModContentRegistry* Registry = UModContentRegistry::Get(World);
        if (!IsValid(Registry))
        {
            UE_LOG(LogTsabsPassengerWagon, Error, TEXT("TSABS_CONTENT registration missing-registry"));
            return;
        }
        ContentState.bRegistrationAttempted = true;
        Registry->RegisterRecipe(Owner, Recipe);
        FGameObjectRegistration Registration;
        ContentState.bRegistrationVerified = Registry->GetRecipeInfo(Recipe, Registration) &&
            Registration.RegisteredObject == Recipe.Get() && Registration.OwnedByModReference == Owner &&
            !Registration.HasAnyFlags(EGameObjectRegistrationFlags::Removed | EGameObjectRegistrationFlags::Unregistered);
        UE_LOG(LogTsabsPassengerWagon, Display, TEXT("TSABS_CONTENT registration verified=%d owner=%s recipe=%s netmode=%d"),
            ContentState.bRegistrationVerified, *Owner.ToString(), *GetNameSafe(Recipe.Get()), static_cast<int32>(World->GetNetMode()));
        if (!ContentState.bRegistrationVerified)
        {
            UE_LOG(LogTsabsPassengerWagon, Error, TEXT("TSABS_CONTENT registration readback-failed"));
        }
    }
    if (Phase != ELifecyclePhase::POST_INITIALIZATION || World->GetNetMode() == NM_Client)
        return;
    if (!ContentState.bPostInitialized)
    {
        ContentState.bPostInitialized = true;
        // Resolve only after world initialization. Never load native BP CDOs in constructors/INIT.
        NativeFreightRecipe = LoadClass<UFGRecipe>(nullptr,
            TEXT("/Game/FactoryGame/Recipes/Vehicle/Train/Recipe_FreightWagon.Recipe_FreightWagon_C"));
        if (!NativeFreightRecipe)
            UE_LOG(LogTsabsPassengerWagon, Error, TEXT("TSABS_CONTENT native-freight-reference unresolved; legacy own grant will be revoked"));
        BindAvailabilityHooks();
    }
    ReconcileAvailability(AFGRecipeManager::Get(World));
}

void UTsabsPassengerWorldModule::BindAvailabilityHooks()
{
    check(IsInGameThread());
    if (WorldCleanupHandle.IsValid() || ContentState.bTearingDown || !bAvailabilityHooksRunning) return;
    AvailabilityModules.RemoveAll([this](const auto& Weak) { return !Weak.Get() || Weak.Get() == this; });
    AvailabilityModules.Add(this);
    WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UTsabsPassengerWorldModule::OnWorldCleanup);
}

void UTsabsPassengerWorldModule::ReconcileAvailability(AFGRecipeManager* Manager)
{
    UWorld* World = GetWorld();
    if (!ContentState.bPostInitialized || ContentState.bTearingDown || ContentState.bReconciling ||
        !IsValid(World) || !World->IsGameWorld() || GetOwnerModReference() != FName(TEXT("TsabsPassengerWagon")) ||
        World->GetNetMode() == NM_Client || !IsValid(Manager) || Manager->GetWorld() != World ||
        Manager != AFGRecipeManager::Get(World) || !Manager->HasAuthority())
        return;

    TGuardValue<bool> Guard(ContentState.bReconciling, true);
    const TSubclassOf<UFGRecipe> Recipe = UTsabsPassengerRecipe::StaticClass();
    const auto VerifyRegistration = [this, World, Recipe]()
    {
        UModContentRegistry* Registry = UModContentRegistry::Get(World);
        FGameObjectRegistration Registration;
        ContentState.bRegistrationVerified = ContentState.bRegistrationAttempted && IsValid(Registry) &&
            Registry->GetRecipeInfo(Recipe, Registration) && Registration.RegisteredObject == Recipe.Get() &&
            Registration.OwnedByModReference == GetOwnerModReference() &&
            !Registration.HasAnyFlags(EGameObjectRegistrationFlags::Removed | EGameObjectRegistrationFlags::Unregistered);
    };
    VerifyRegistration();
    ContentState.bAvailabilityAttempted = true;
    for (;;)
    {
        const bool bReferenceResolved = IsValid(NativeFreightRecipe.Get());
        const bool bNativeAvailable = bReferenceResolved && Manager->IsRecipeAvailable(NativeFreightRecipe);
        const bool bRequested = ContentState.bRegistrationVerified && bReferenceResolved && bNativeAvailable;
        const bool bOwnAvailable = Manager->IsRecipeAvailable(Recipe);
        const auto Action = TsabsPassenger::DecidePassengerAvailability(ELifecyclePhase::POST_INITIALIZATION,
            true, true, false, true, ContentState.bRegistrationVerified, bReferenceResolved, false, bNativeAvailable, bOwnAvailable);
        if (Action == TsabsPassenger::EPassengerAvailabilityAction::Grant)
            Manager->AddAvailableRecipe(Recipe);
        else if (Action == TsabsPassenger::EPassengerAvailabilityAction::Revoke)
            Manager->RemoveAvailableRecipes({Recipe});

        // Never use cached context after synchronous native callbacks change/tear it down.
        if (ContentState.bTearingDown || !IsValid(World) || GetWorld() != World || !World->IsGameWorld() ||
            GetOwnerModReference() != FName(TEXT("TsabsPassengerWagon")) || !IsValid(Manager) ||
            Manager->GetWorld() != World || Manager != AFGRecipeManager::Get(World) ||
            World->GetNetMode() == NM_Client || !Manager->HasAuthority())
        {
            ContentState.bAvailabilityVerified = false;
            return;
        }
        VerifyRegistration();
        const bool bDesired = ContentState.bRegistrationVerified && IsValid(NativeFreightRecipe.Get()) && Manager->IsRecipeAvailable(NativeFreightRecipe);
        ContentState.bAvailabilityVerified = Manager->IsRecipeAvailable(Recipe) == bDesired;
        // A finite synchronous nested native transition is settled before returning.
        // Do not retry a refused native write when its desired state has not changed.
        if (ContentState.bAvailabilityVerified || bDesired == bRequested) break;
    }
    if (!ContentState.bRegistrationVerified)
        UE_LOG(LogTsabsPassengerWagon, Error, TEXT("TSABS_CONTENT availability registration invalid; own legacy grant revoked if present"));
    if (!ContentState.bAvailabilityVerified)
        UE_LOG(LogTsabsPassengerWagon, Error, TEXT("TSABS_CONTENT availability parity-readback-failed"));
}

void UTsabsPassengerWorldModule::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    if (World == GetWorld()) ReleaseAvailabilityHooks();
}

void UTsabsPassengerWorldModule::ReleaseAvailabilityHooks()
{
    ContentState.bTearingDown = true;
    ContentState.bPostInitialized = false;
    ContentState.bAvailabilityVerified = false;
    check(IsInGameThread());
    AvailabilityModules.RemoveAll([this](const auto& Weak) { return !Weak.Get() || Weak.Get() == this; });
    // No SML mutation here, even when this was the last world. The currently
    // executing callable/list belongs to the DLL until ordinary shutdown.
    if (WorldCleanupHandle.IsValid())
    {
        FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
        WorldCleanupHandle.Reset();
    }
    NativeFreightRecipe = nullptr;
}

void UTsabsPassengerWorldModule::BeginDestroy()
{
    ReleaseAvailabilityHooks();
    Super::BeginDestroy();
}

