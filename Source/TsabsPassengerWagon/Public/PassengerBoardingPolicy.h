#pragma once
#include "CoreMinimal.h"
#include "FGUseableInterface.h"
#include "PassengerBoardingPolicy.generated.h"

/** Invalid use-state kept separate from the SDK's valid constructor/stub setter. */
UCLASS()
class TSABSPASSENGERWAGON_API UTsabsPassengerUseState_Blocked : public UFGUseState
{
    GENERATED_BODY()
public:
    UTsabsPassengerUseState_Blocked() { mIsUsableState = false; }
};

enum class ETsabsBoardingFeedback : uint8
{
    Ready, InvalidCharacter, Unavailable, Busy, OutOfReach, ExitBlocked
};
