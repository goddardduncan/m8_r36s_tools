#!/bin/bash

LOG_FILE="/tmp/refresh_gamelist.log"

# Ensure the log file is owned by the correct user
sudo chown ark:ark $LOG_FILE 2>/dev/null
sudo chmod 666 $LOG_FILE 2>/dev/null
sudo chown -R ark:ark /home/ark/.emulationstation /roms /etc/emulationstation
sudo chmod -R 755 /home/ark/.emulationstation /roms

echo "===== Restarting EmulationStation Gamelist =====" | tee -a $LOG_FILE

    echo "Stopping EmulationStation..." | tee -a $LOG_FILE
    
    # Use root for `pkill` to ensure EmulationStation is killed
    sudo pkill -9 -f emulationstation
    sleep 5
    if pgrep -x emulationstation > /dev/null; then
    echo "EmulationStation is still running! Killing it now..." | tee -a $LOG_FILE
    sudo pkill -9 -f emulationstation
    sleep 1
    fi
    sudo -u ark /bin/emulationstation/emulationstation > /tmp/es.log 2>&1 &

    echo "Refresh command sent!" | tee -a $LOG_FILE

echo "===== Refresh Completed =====" | tee -a $LOG_FILE
