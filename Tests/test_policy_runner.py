"""Portable source-level checks; full Unreal/gameplay tests are separate."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
class ActivePolicy(unittest.TestCase):
    def test_native_actor_picker_remains_authoritative(self):
        text=(ROOT/'Source/TsabsPassengerWagon/Private/TsabsPassengerRailcar.cpp').read_text(encoding='utf-8-sig')
        self.assertIn('IsBoardingCharacterValid(Character) && Character->GetBestUsableActor() == this',text)
    def test_removed_geometry_does_not_return(self):
        self.assertFalse((ROOT/'Source/TsabsPassengerWagon/Public/PassengerBoardingGeometry.h').exists())
    def test_harness_exercises_production_reservation(self):
        text=(ROOT/'Tests/StandalonePassengerPolicyTests.cpp').read_text(encoding='utf-8-sig')
        self.assertIn('#include "PassengerBoardingReservation.h"',text)
        self.assertIn('FScopedPassengerBoardingReservation',text)
if __name__=='__main__': unittest.main(verbosity=2)
