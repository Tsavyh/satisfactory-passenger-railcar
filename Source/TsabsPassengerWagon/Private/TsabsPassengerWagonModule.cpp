#include "TsabsPassengerWagonLog.h"
#include "Modules/ModuleManager.h"
#include "UI/FGUserWidget.h"
#include "FGAbstractServerWidget.h"
#include "PassengerCrosshair.h"

DEFINE_LOG_CATEGORY(LogTsabsPassengerWagon);

class FTsabsPassengerWagonModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        // This native symbol keeps the FactoryDedicatedClient DLL prerequisite
        // in the import table, rather than relying on incidental engine order.
        UClass* ServerWidgetClass = UFGAbstractServerWidget::StaticClass();
        UE_LOG(LogTsabsPassengerWagon, Display, TEXT("TSABS_STARTUP_PREREQUISITE %s"),
            *GetPathNameSafe(ServerWidgetClass));
        UE_LOG(LogTsabsPassengerWagon, Display, TEXT("Tsabs passenger prototype module loaded; game validation is still pending."));
        TsabsPassenger::InstallPassengerCrosshairHook();
    }
    virtual void ShutdownModule() override
    {
        TsabsPassenger::UninstallPassengerCrosshairHook();
    }
};

IMPLEMENT_MODULE(FTsabsPassengerWagonModule, TsabsPassengerWagon)
