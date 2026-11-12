"""Fail-first structural prerequisite contract; runtime DX11 probe is separate."""
from pathlib import Path
import unittest

BASE = Path(__file__).resolve().parents[1] / 'Source/TsabsPassengerWagon'

class RequiredModule(unittest.TestCase):
    def test_native_widget_module_is_an_explicit_build_dependency(self):
        self.assertIn('PrivateDependencyModuleNames.Add("FactoryDedicatedClient")', (BASE / 'TsabsPassengerWagon.Build.cs').read_text())

    def test_native_class_is_referenced_to_retain_the_dll_import(self):
        text = (BASE / 'Private/TsabsPassengerWagonModule.cpp').read_text()
        self.assertIn('#include "FGAbstractServerWidget.h"', text)
        self.assertIn('UFGAbstractServerWidget::StaticClass()', text)
        self.assertIn('TSABS_STARTUP_PREREQUISITE', text)

if __name__ == '__main__':
    unittest.main(verbosity=2)
