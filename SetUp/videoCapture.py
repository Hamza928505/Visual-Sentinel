import subprocess
import datetime
import os
import tkinter as tk
import sys
import re
from tkinter import simpledialog, messagebox

# --- FFmpeg Settings ---
DEFAULT_DURATION_SECONDS = 20
DEFAULT_FILENAME = "EXP-000 B5 F18 D20 H20"
CAMERA_DEVICE = "/dev/video0" 

def check_video_signal(device_path):
    """
    Analyzes the video feed to ensure it's not just a black screen.
    Uses 'signalstats' to get brightness (YAVG).
    """
    print(f"Analyzing video signal on {device_path}...")
    
    # This command captures 1 frame and prints video statistics to STDOUT
    # lavfi.signalstats.YAVG = Average Luma (Brightness) [0-255]
    command = [
        "ffmpeg",
        "-f", "v4l2",
        "-i", device_path,
        "-frames:v", "1",                    # Analyze only 1 frame
        "-vf", "signalstats,metadata=print:key=lavfi.signalstats.YAVG:file=-", 
        "-f", "null", "-",                   # Don't save a file, just output data
        "-v", "error"                        # Hide normal logs
    ]

    try:
        result = subprocess.run(
            command, 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE, 
            text=True, 
            timeout=8 
        )

        output = result.stdout
        
        # Parse YAVG (Average Brightness) using Regex
        yavg_match = re.search(r"lavfi.signalstats.YAVG=([\d\.]+)", output)

        if yavg_match:
            yavg = float(yavg_match.group(1))
            
            # Fixed Syntax Error: avoiding backslashes inside f-strings
            print("Signal Brightness (YAVG): " + str(yavg))

            # CONDITION: Is it too dark? (Black screen / Lens cap)
            # A completely black screen (no signal) usually has YAVG < 16.
            if yavg < 20: 
                print("FAIL: Image is too dark (Black screen detected).")
                return False
            
            return True
        else:
            print("FAIL: Could not parse video statistics.")
            return False

    except subprocess.TimeoutExpired:
        print("FAIL: Camera timed out (Device unresponsive).")
        return False
    except Exception as e:
        print("FAIL: Signal check error: " + str(e))
        return False
        
def run_ffmpeg_recording(output_path, duration_seconds):
    """Constructs and runs the FFmpeg command (Linux version)."""

    command = [
        "ffmpeg",
        "-f", "v4l2",
        "-framerate", "25",
        "-video_size", "1920x1080",
        "-i", CAMERA_DEVICE,
        "-t", str(duration_seconds),
        "-vf", "showinfo",
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-pix_fmt", "yuv420p",
        output_path
    ]

    print(f"Recording started... Duration: {duration_seconds}s. Saving to {output_path}")
    try:
        subprocess.run(command, check=True)
        messagebox.showinfo("Success", f"Recording finished and saved to:\n{output_path}")
    except subprocess.CalledProcessError as e:
        messagebox.showerror("Error", f"FFmpeg failed with exit code {e.returncode}.\nCheck console for details.")
    except FileNotFoundError:
        messagebox.showerror("Error", "FFmpeg not found.\nPlease ensure FFmpeg is installed.")
    except Exception as e:
        messagebox.showerror("Error", f"An unexpected error occurred: {e}")

def usb_camera_connected():
    """Check if any USB capture card is connected using lsusb."""
    try:
        result = subprocess.run(
            ["lsusb"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True
        )
        output = result.stdout.lower()
        # Add keywords for your specific capture card
        camera_keywords = ["Elgato", "Cam Link", "Video Capture", "HDMI"] 
        
        for keyword in camera_keywords:
            if keyword.lower() in output:
                print("[USB Check] Capture card detected: " + keyword)
                return True

        print("[USB Check] No Capture card detected via lsusb.")
        return False
    except Exception as e:
        print("[USB Check] lsusb failed: " + str(e))
        return False
        
def record(provided_id=None):
    """Handles the Tkinter GUI logic for file selection and recording."""
    
    # --- 1. Hardware Check (Is the USB dongle plugged in?) ---
    if not usb_camera_connected():
        messagebox.showerror("Connection Error", "Capture Card not found in USB ports.")
        return
    
    # --- 2. Signal Check (Is the HDMI actually sending video?) ---
    if not check_video_signal(CAMERA_DEVICE):
        response = messagebox.askyesno(
            "No Video Signal", 
            "The camera seems to be showing a BLACK SCREEN.\n\n"
            "1. Is the HDMI connected?\n"
            "2. Is the Camera turned ON?\n"
            "3. Is the Lens Cap off?\n\n"
            "Do you want to record anyway?"
        )
        if not response:
            return # Stop if user clicks No

    save_dir = os.getcwd()

    # --- 3. Determine Filename ---
    if provided_id:
        filename_base = provided_id
        print(f"Using provided Test ID: {filename_base}")
    else:
        filename_base = simpledialog.askstring(
            "Filename",
            "Enter the Test ID (e.g., EEXP-000 B5 F18 D20 H20):",
            initialvalue=DEFAULT_FILENAME
        )
    
    if not filename_base:
        return 

    # --- 4. Construct final filename (NO DATE) ---
    # I removed the datetime logic here
    final_filename = f"{filename_base}.mp4"
    output_path = os.path.join(save_dir, final_filename)

    # --- 5. Run recording ---
    run_ffmpeg_recording(output_path, DEFAULT_DURATION_SECONDS)
    

if __name__ == "__main__":
    root = tk.Tk()
    root.withdraw()

    try:
        # Check command line arguments
        if len(sys.argv) > 1:
            input_id = sys.argv[1]
            record(input_id)
        else:
            record() 
            
    except Exception as e:
        messagebox.showerror("Initialization Error", str(e))
