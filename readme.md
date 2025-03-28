First I made an m8 tracker sample kit creator: https://m8slice.glitch.me 

You can use it to make a merged wav file with cue points from up to 32 separate wav files to use as a custom sampler instrument.

I wanted to be able to do the whole process on an r36s.

There are a suite of tools here to make and upload m8 sample kits to the SD card of a teensy m8 tracker using an R36s.  Using them you can:

-Browse a library of samples, listen to them, and add them to a saved list of the ones you like

-Merge these wavs and slice cue points into them into a saved file

-Upload this file to your SD card to use on your M8 tracker

-Another app allows you to download files from https://m8slice.glitch.me that have been sliced already (you still need to upload them to the SD card with the upload tool

Check each sub folder for a description of each tool + code + executables (in build folders).
To make the most of this, you need a USB C to microSD card reader dongle for your OTG on your R36s.
To download files for m8slice.glitch.me you'll need to have an active internet connection on your r36s.  

You can build/upload apps if you enable remote management on your r36s.  I recommend using https://github.com/dov/r36s-programming for instructions on how to setup a new "system" folder that you can run them from.  Also cyberduck is a great tool for *finder*-like access to the files on the r36s.

**Additional files**

- Have got a few files to assist to run m8c on r36s (custom emulationstation theme, gamecontrollers.c, m8c_go.sh)
- ff_menu and ff_radio are small apps to stream Melbourne TV and Radio (.json files go in /roms/tvradio/)

**To enable SSH if turned off, copy the enable_ssh script to your ports folder.**
