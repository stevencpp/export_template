@ECHO OFF
set current_path=%cd%
set mypath=%~dp0

for %%T in (cuda diamond dll ping_pong) do (
	call full_clean_one.bat %%T
	call run_one.bat %%T
	if not exist %mypath%\%%T\build\Debug\A.exe (
		echo %%T test failed
		goto end
	)
)

:end