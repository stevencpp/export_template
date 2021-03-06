@ECHO OFF
set test=%1
set verbosity=%2

set current_path=%cd%
set mypath=%~dp0

if "%test%" == "" goto end
if not exist "%mypath%\%test%" goto end

cd %mypath%/%test%

if exist build (
	cd build
	for /F %%X in ('dir *.dir /s /b') do (
		rmdir %%X /s /q
	)
	rmdir Debug intermediate x64 /s /q
)

cd %current_path%

:end