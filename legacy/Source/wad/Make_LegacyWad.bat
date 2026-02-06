@echo off
REM ifdef WAD
REM @echo "Building a new legacy.wad using an old version $(WAD)..."
REM	./wadtool.exe -x $(WAD)
REM cp ../resources/*.txt .
REM cp ../resources/*.png .
REM	cp ../resources/*.h .
REM	cp ../resources/*.lmp .
REM	./d2h.exe doom2hexen.txt XDOOM.lmp
REM	./d2h.exe heretic2hexen.txt XHERETIC.lmp
REM	./wadtool.exe -c ../legacy.wad ../resources/legacy.wad.inventory
REM	rm *.txt *.png *.h *.lmp
REM	@echo "Finished building legacy.wad."
REM else
REM	@echo "Usage: make wad WAD=/path/to/existing/legacy.wad"
REM endif
@echo off
setlocal EnableDelayedExpansion

if "%~1"=="" goto :usage
set "OLD_WAD=%~f1"

if not exist "%OLD_WAD%" (
    echo Datei nicht gefunden: %OLD_WAD%
    goto :usage
)

set "TEMP_DIR=build_temp"

echo.
echo Building new legacy.wad from: %OLD_WAD%
echo Using temporary folder: %TEMP_DIR%
echo.

:: -------------------------------------------------------
:: Alten Temp-Ordner komplett wegmachen (falls von vorherigem Lauf noch da)
:: -------------------------------------------------------
if exist "%TEMP_DIR%\" (
    echo Alte temporaere Dateien loeschen ...
    rd /s /q "%TEMP_DIR%" 2>nul
)

mkdir "%TEMP_DIR%"
if errorlevel 1 (
    echo Fehler beim Erstellen von %TEMP_DIR%
    pause
    exit /b 1
)

cd /d "%TEMP_DIR%" || (
    echo Kann nicht in %TEMP_DIR% wechseln
    pause
    exit /b 1
)

:: -------------------------------------------------------
:: Entpacken + Kopieren – alles landet jetzt im TEMP_DIR
:: -------------------------------------------------------
cd %TEMP_DIR%
echo Entpacke altes WAD ...
..\wadtool.exe -x "%OLD_WAD%"

echo Kopiere Ressourcen-Dateien ...
copy "..\..\resources\*.txt"  . >nul 2>&1
copy "..\..\resources\*.png"  . >nul 2>&1
copy "..\..\resources\*.h"    . >nul 2>&1
copy "..\..\resources\*.lmp"  . >nul 2>&1

:: -------------------------------------------------------
:: Konvertierungen
:: -------------------------------------------------------
echo Konvertiere Textdateien ...
..\d2h.exe doom2hexen.txt  XDOOM.lmp
..\d2h.exe heretic2hexen.txt XHERETIC.lmp

:: -------------------------------------------------------
:: Neues WAD erstellen (Pfad relativ vom TEMP_DIR aus)
:: -------------------------------------------------------
echo Erstelle neues legacy.wad ...
..\wadtool.exe -c "..\legacy.wad" "..\..\resources\legacy.wad.inventory"

cd..
:: -------------------------------------------------------
:: Aufräumen – komplett
:: -------------------------------------------------------
echo.
echo Aufraeumen ...
cd /d ".." || exit /b 1

rd /s /q "%TEMP_DIR%" 2>nul

if exist "%TEMP_DIR%\" (
     echo WARNUNG: Einige Dateien in %TEMP_DIR% konnten nicht geloescht werden
 ) else (
     echo Temp-Ordner erfolgreich entfernt.
 )

echo.
echo Fertig – neues legacy.wad sollte jetzt da sein.
pause
goto :eof

:usage
echo.
echo Usage:
echo   %~nx0 pfad\zu\alter\legacy.wad
echo Beispiel:
echo   %~nx0 old\legacy.wad
pause
REM exit /b 1