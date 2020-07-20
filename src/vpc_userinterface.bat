@echo off

devtools\bin\vpc.exe /ofd +UserInterface /mksln openfortress_ui.sln /2013

if errorlevel 1 (
	pause
)

exit