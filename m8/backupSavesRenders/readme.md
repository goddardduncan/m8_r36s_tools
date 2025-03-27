**Backup songs and renders from M8 SD card to r36s and upload backup** <br>
Mounts SD card to /USB <br>
Copies /renders and /songs from SD card to /roms/drums/renders and /roms/drums/songs <br>
Converts rendered .WAV files to .MP3 <br>
Deletes .WAV files in /roms/drums/renders <br>
Unmounts /USB/ <br>
Adds /roms/drums/renders and /roms/drums/songs to timestamped zipfile <br>
Uploads timestamped zipfile to github repository <br>
Exits <br>

Github repo is custom for me so HTTP upload token and git location removed in this code. <br>

NEED TO MODIFY HERE TO CUSTOM FOR YOU <br>
fprintf(sh, <br>
    "cd /roms/drums/backups && " <br>
    "git config --global user.email 'you@example.com' && " <br>
    "git config --global user.name 'r36s-upload' && " <br>
    "git init && " <br>
    "(git remote | grep origin || git remote add origin YOURTOKEN@github.com/YOURGIT) && " <br>
    "git add . && git commit -m 'Auto Backup' && git push -u origin master --force" <br>
); <br>

