#!/bin/bash

# === CONFIG ===
PIPE="stream.ts"
PORT=8080
URL="$1"

# Kill previous Python server if running
PID_TO_KILL=$(lsof -ti tcp:$PORT)
if [[ ! -z "$PID_TO_KILL" ]]; then
  echo "Killing previous server on port $PORT (PID $PID_TO_KILL)..."
  kill -9 $PID_TO_KILL
  sleep 1
fi


if [[ -z "$URL" ]]; then
  echo "❌ No stream URL provided!"
  echo "Usage: ./stream.sh <YouTube or m3u8 URL>"
  exit 1
fi

# Detect YouTube vs direct stream
if [[ "$URL" == *"youtube.com"* || "$URL" == *"youtu.be"* ]]; then
  echo "🎯 Detected YouTube URL. Fetching direct stream..."
  if ! command -v yt-dlp &> /dev/null; then
    echo "❌ yt-dlp is not installed! Please install it first."
    exit 1
  fi
  STREAM_URL=$(yt-dlp -f 'best[ext=mp4]' -g "$URL")
else
  echo "🌐 Using direct stream URL."
  STREAM_URL="$URL"
fi

if [[ -z "$STREAM_URL" ]]; then
  echo "❌ Failed to retrieve stream URL."
  exit 1
fi

# Create the FIFO if it doesn't exist
if [[ ! -p "$PIPE" ]]; then
  echo "📦 Creating named pipe: $PIPE"
  mkfifo "$PIPE"
fi

# Start Python HTTP server in background
echo "🚀 Serving on http://$(hostname -I | awk '{print $1}'):$PORT/stream.ts"
python3 -m http.server $PORT --bind 0.0.0.0 &
HTTP_PID=$!

# Wait a sec for the server to spin up
sleep 1

# Start FFmpeg to downscale and stream
echo "🎬 Starting FFmpeg with live-edge settings..."
ffmpeg -y -fflags +nobuffer -flags low_delay -probesize 32 -analyzeduration 0 -re -i "$STREAM_URL" \
  -vf scale=640:480 -c:v libx264 -preset veryfast -tune zerolatency \
  -f mpegts "$PIPE"

# Kill the server on exit
echo "🛑 FFmpeg exited. Stopping HTTP server (PID $HTTP_PID)..."
kill $HTTP_PID
