@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%~dp0..\.test-bin"
if not exist "%OUT%" mkdir "%OUT%"
for %%M in (precise fast) do (
  cl /nologo /EHsc /std:c++20 /O2 /fp:%%M /W4 /WX /I"%ROOT%\Source\TsabsPassengerWagon\Public" /Fe:"%OUT%\passenger-policy-%%M.exe" /Fo:"%OUT%\passenger-policy-%%M.obj" "%ROOT%\Tests\StandalonePassengerPolicyTests.cpp"
  if errorlevel 1 exit /b 1
  "%OUT%\passenger-policy-%%M.exe"
  if errorlevel 1 exit /b 1
)
exit /b 0
