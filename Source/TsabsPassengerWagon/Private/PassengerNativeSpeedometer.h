#pragma once
#include "CoreMinimal.h"
class UWidgetTree;
class UCanvasPanel;
class UTextBlock;
namespace TsabsPassenger
{
    /** Copies only allowlisted plain UMG visuals; never instantiates the driving UUserWidget. */
    UTextBlock* ComposeNativeSpeedometer(UWidgetTree* Destination, UCanvasPanel* Canvas, UWidgetTree* NativeTemplate);
}
