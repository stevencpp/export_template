@ECHO OFF
set test=%1
set verbosity=%2

set current_path=%cd%
set mypath=%~dp0

if "%test%" == "" goto end
if not exist "%mypath%\%test%" goto end

cd %mypath%/%test%
for /F %%X in ('dir *.xti /s /b') do (
	del %%X
)
cd %current_path%


:end