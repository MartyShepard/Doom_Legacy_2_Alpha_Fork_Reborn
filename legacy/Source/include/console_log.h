// console_log.h
#ifndef CONSOLE_LOG_H
#define CONSOLE_LOG_H

#pragma once
#include "console.h"   // für CONS_Printf
#include <cstdio>
#include <cstdarg>

/* ==========================================================================
   LOGGING-SYSTEM – Programmier-Notiz (Developer Documentation)
   ==========================================================================

   Ziel:
   - Einheitliche, level-gesteuerte Ausgabe über CONS_Printf
   - Alte Aufrufe (CONS_Printf) bleiben unverändert und funktionieren weiter
   - Neue Aufrufe können mit Level gefiltert werden
   - Im Spiel über "loglevel X" steuerbar
   - Farben in der Konsole + Datei/Zeile für Debugging

   --------------------------------------------------------------------------
   Die 4 wichtigen Teile und wie sie zusammenhängen:

   1. CV_PossibleValue_t loglevel_cons_t
      → Definiert die Auswahl-Liste, die im Spiel angezeigt wird
      → Wird in der consvar_t verwendet (für "loglevel ?" und Help)

   2. consvar_t cv_loglevel
      → Die eigentliche Console-Variable (wie cv_scr_depth, cv_fullscreen)
      → Wird in config.cfg gespeichert
      → Hat CV_CALL → ruft bei Änderung CV_LogLevelChanged() auf

   3. LogLevel current_loglevel (globale Variable)
      → Der aktuelle interne Wert (int/enum), den das Makro prüft
      → Wird im Callback von cv_loglevel aktualisiert
      → Das Makro schaut nur auf diese Variable!

   4. Makro CONS_Print(level, fmt, ...)
      → Überschreibt/ergänzt die alte CONS_Printf
      → Prüft: if (level <= current_loglevel || current_loglevel == LOG_ALL)
      → Fügt Datei:Zeile + Farb-Tag hinzu
      → Ruft am Ende die echte ::CONS_Printf auf

   --------------------------------------------------------------------------
   Wie der komplette Ablauf funktioniert, wenn du schreibst:

       CONS_Print(LOG_DEBUG, "Player pos: %d, %d\n", x, y);

   1. Makro wird expandiert
   2. Prüft: Ist LOG_DEBUG (6) <= current_loglevel ?
      → Nein → nichts wird ausgegeben
      → Ja  → weiter

   3. Baut den String:
      "DEBUG r_draw16.cpp:123: Player pos: 123, 456\n"

   4. Ruft die echte CONS_Printf auf → landet in Konsole + Terminal

   --------------------------------------------------------------------------
   Wichtige Level-Bedeutungen (dein aktuelles Schema):

   LOG_HELP  (0) → Gar nichts (außer evtl. kritische Errors)
   LOG_OFF   (1) → Gar nichts (außer evtl. kritische Errors)
   LOG_DOOM  (2) → Original Doom-Meldungen (Start, Demo, IWAD etc.)
   LOG_HUD   (3) → Bildschirm Augaben
   LOG_INFO  (4) → Normale Infos   
   LOG_WARN  (5) → Warnungen   
   LOG_ERROR (6) → Nur echte Fehler (oft mit I_Error)
   LOG_DEBUG (7) → Detaillierte Debug-Infos für Entwickler
   LOG_ALL   (8) → Alles anzeigen (Debug-Modus)

   Besonderheit:
   - LOG_ERROR leitet automatisch an I_Error weiter (außer bei LOG_DEBUG oder LOG_ALL)
   - Alte Aufrufe ohne Level (CONS_Printf) werden bei LOG_DOOM (5) und höher angezeigt

   --------------------------------------------------------------------------
   Beispiele:

   CONS_Print(LOG_INFO,  "Video mode: %dx%d @ %d bpp\n", width, height, bpp);
   CONS_Print(LOG_DEBUG, "R_DrawColumn_16: x=%d, count=%d\n", dc_x, count);
   CONS_Print(LOG_ERROR, "Cannot allocate render buffer!\n");

   // Im Spiel tippen:
   loglevel 4        → nur bis DEBUG
   loglevel 6        → alles (inkl. LOG_ALL)
   loglevel -1       → Global-Off (alles durch, falls du das so definiert hast)

   Mit Farbe je nach Level  
   \2 = rot (ERROR)
   \3 = gelb (WARN)
   \4 = grün (INFO)
   \5 = grau/blau (DEBUG)
  
   ========================================================================== */
   
/// Makro, das CONS_Printf "überschreibt"
#undef CONS_Printf   // Falls schon definiert – löschen

#define CONS_Print(level, fmt, ...)\
    if (CurLogLevel == LOG_ALL)\
    {\
       LogLvl = level;\
    }\
    else\
    {\
       LogLvl = CurLogLevel;\
    }\
    \
    do { \
        if ((int)level > 1 && (int)level == LogLvl)\
        {\
            const char* file = strrchr(__FILE__, '\\'); \
            if (!file) file = strrchr(__FILE__, '/');   \
            file = file ? file + 1 : __FILE__;          \
            \
            static const char* tags_cmd[]  = {"", "", "", "",\
                                                "INFO ",     \
                                                "WARN ",     \
                                                "ERROR",     \
                                                "DEBUG",     \
                                                "ALL >"};    \
            \
            static const char* tags_game[] = {"", "", "", "", \
                                              "\4INFO\4 ",    \
                                              "\3WARN\3 ",    \
                                              "\2ERROR\2",    \
                                              "\5DEBUG\5",    \
                                              "\1ALL >\1"};   \
            \
            const char** tags = con.IsActive() ? tags_game : tags_cmd;\
            \
            if ((int)level == LOG_ERROR && (int)level <= LOG_ERROR)\
            {\
              ::I_Error("%-5s %-16s:%4d: " fmt, \
                            tags[level],        \
                            file, __LINE__,     \
                            ##__VA_ARGS__);     \
            }\
            \
            ::CONS_Printf("%-5s %-16s:%4d: " fmt,\
                          tags[level],           \
                          file, __LINE__,        \
                          ##__VA_ARGS__);        \
        }\
    } while(0)

#endif // __CONSOLE_LOG_H__
