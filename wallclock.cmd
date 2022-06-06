@echo off
set CMDNAME=%0
set CMDPATH=%cd%
set ENDCUTTED=%CMDNAME:.cmd"=%
call set BINNAME=%%ENDCUTTED:"%CMDPATH%\=%%
set CAPTION=Compile %CMDPATH%\%BINNAME%.c
title %CAPTION%
echo.
echo %CAPTION%

if /I exist %BINNAME%.c (
	echo.
	avr-gcc -Os -Wall -I"C:\Program Files (x86)\WinAVR2010\avr\include" -L"Ñ:\Program Files (x86)\WinAVR2010\avr\lib" %BINNAME%.c -o %BINNAME%.elf
	avr-objcopy -O binary -R .eeprom %BINNAME%.elf %BINNAME%.bin
	avr-objcopy -O ihex -R .eeprom %BINNAME%.elf %BINNAME%.hex
) else (
	echo.
	echo File ..\%BINNAME%.c does not exist!
)
echo.
pause

rem for /f "" %%i in (%CMDNAME%) do (set BINNAME=%%~ni)
rem -mmcu=atmega8535