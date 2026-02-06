#!/bin/bash
set -e  # Beendet bei Fehler
cd Source

LOGFILE="../logs/build_$(date +%Y-%m-%d_%H-%M-%S).log"

echo "Build started at $(date)" > "$LOGFILE"
echo "=====================================" >> "$LOGFILE"

make "$@" 2>&1 | tee -a "$LOGFILE"

echo "=====================================" >> "$LOGFILE"
echo "Build finished at $(date)" >> "$LOGFILE"

if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "SUCCESS - Log: $LOGFILE"
else
    echo "ERROR - siehe $LOGFILE"
    exit 1
fi
cd ..