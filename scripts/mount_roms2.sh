#!/bin/bash

# Mount the SD card partition using absolute paths
sudo /bin/mount -t exfat -o defaults,noatime,umask=000,nonempty /dev/mmcblk1p3 /roms2
#sudo /roms/ports/refresh.sh
if /bin/mountpoint -q /roms2; then
    echo "Mount successful!" | tee -a $LOG_FILE
else
    echo "Mount failed!" | tee -a $LOG_FILE
fi
