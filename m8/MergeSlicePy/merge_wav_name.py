import os
import struct
import numpy as np
import soundfile as sf
import argparse

# Define directories
INPUT_FOLDER = "/roms/drums/saved/"
OUTPUT_FOLDER = "/roms/drums/sliced/"

parser = argparse.ArgumentParser()
parser.add_argument("--prefix", type=str, default="MERGED", help="Prefix for the output file")
args = parser.parse_args()

output_file = os.path.join(OUTPUT_FOLDER, f"{args.prefix}_merged_sliced.wav")
print(f"Saving merged file as: {output_file}")

def merge_wav_files(input_folder, output_file):
    """Merges WAV files from input_folder and embeds cue points in the output WAV file."""
    wav_files = sorted([
        os.path.join(input_folder, f)
        for f in os.listdir(input_folder)
        if f.lower().endswith(".wav") and not f.startswith("._")
    ])

    if not wav_files:
        print("No WAV files found in", input_folder)
        return

    audio_data = []
    cue_points = []
    sample_rate = None
    current_frame = 0

    for i, file_path in enumerate(wav_files):
        data, sr = sf.read(file_path, dtype="int16")

        if sample_rate is None:
            sample_rate = sr  # Use the first file's sample rate

        # Convert mono to stereo
        if len(data.shape) == 1:
            data = np.column_stack((data, data))

        audio_data.append(data)

        # Store cue marker positions
        cue_points.append(current_frame)
        current_frame += len(data)

        print(f"Added: {file_path} (Frames: {len(data)})")

    # Merge all audio data
    merged_audio = np.vstack(audio_data)

    # Write merged WAV file
    sf.write(output_file, merged_audio, sample_rate, format='WAV', subtype='PCM_16')

    # Write cue markers
    write_cue_markers(output_file, cue_points)
    print(f"Saved merged file with {len(cue_points)} cue markers: {output_file}")

def write_cue_markers(wav_path, cue_points):
    """Adds cue markers to a WAV file for M8 Tracker."""
    with open(wav_path, "r+b") as f:
        f.seek(0, os.SEEK_END)
        file_size = f.tell()

        num_cues = len(cue_points)
        cue_chunk_size = 4 + (num_cues * 24)
        cue_chunk = b'cue ' + struct.pack('<I', cue_chunk_size) + struct.pack('<I', num_cues)

        for i, sample_offset in enumerate(cue_points):
            cue_entry = struct.pack(
                '<IIIIII', i, sample_offset, 0x64617461, 0, 0, sample_offset
            )
            cue_chunk += cue_entry

        # Append cue chunk at end of file
        f.write(cue_chunk)

        # Update RIFF header size
        f.seek(4)
        riff_size = struct.pack('<I', file_size + len(cue_chunk) - 8)
        f.write(riff_size)

# Ensure output directory exists
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

# Run the script with the filename from the C program
merge_wav_files(INPUT_FOLDER, output_file)
