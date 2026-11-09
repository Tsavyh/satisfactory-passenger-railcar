#pragma once
#include "Module/GameWorldModule.h"
#include "TsabsPassengerWorldModule.generated.h"

class AFGRecipeManager;
class UFGRecipe;

struct FTsabsPassengerContentState
{
    bool bRegistrationAttempted = false;
    bool bRegistrationVerified = false;
    // Diagnostic only: never used to suppress subsequent reconciliation.
    bool bAvailabilityAttempted = false;
    bool bAvailabilityVerified = false;
    bool bPostInitialized = false;
    bool bReconciling = false;
    bool bTearingDown = false;
};

/** One native SML root per game world. Runtime owner is supplied by SML. */
UCLASS()
class TSABSPASSENGERWAGON_API UTsabsPassengerWorldModule : public UGameWorldModule
{
    GENERATED_BODY()
public:
    UTsabsPassengerWorldModule();
    virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
    virtual void BeginDestroy() override;
    // Native subscriptions belong to the DLL, never a world. Shutdown is only
    // supported after the engine tick loop has returned (ordinary module exit).
    static void InstallAvailabilityHooks();
    static void ShutdownAvailabilityHooks();
    const FTsabsPassengerContentState& GetContentState() const { return ContentState; }
private:
    static void ForwardAvailability(AFGRecipeManager* Manager);
    void BindAvailabilityHooks();
    void ReconcileAvailability(AFGRecipeManager* Manager);
    void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    void ReleaseAvailabilityHooks();
    // Per-world instance only; never static or saved as proof of a future session.
    FTsabsPassengerContentState ContentState;
    UPROPERTY(Transient)
    TSubclassOf<UFGRecipe> NativeFreightRecipe;
    FDelegateHandle WorldCleanupHandle;
};
