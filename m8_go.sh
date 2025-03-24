#!/bin/bash

echo "[INFO] Setup for M8 audio input..."

    alsaloop -P hw:0,0 -C hw:1,0 -t 200000 -A 5 --rate 44100 --sync=0 -T -1 -d
    sleep 1
    alsaloop -P hw:0,0 -C hw:7,0 -t 200000 -A 5 --rate 44100 --sync=0 -T -1 -d
    sleep 1
    /home/ark/m8c/m8c/m8c
        

