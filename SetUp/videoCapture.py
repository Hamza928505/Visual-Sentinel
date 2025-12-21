import subprocess
import os
import tkinter as tk
import sys
import re
from tkinter import simpledialog, messagebox

# ---------------- CONFIG ----------------
DURATION_SECONDS = 20
DEFAULT_FILENAME = "EXP-000 B5 F18 D20 H20"
CAMERA_DEVICE = "/dev/video0"

IMU_SOURCE = "/var/log/imu/ypr.jsonl"
GPS_SOURCE = "/var/log/gps/gps.jsonl"
# ---------------------------------------


def usb_camera_connected():
    try:
        result = subprocess.run(
            ["lsusb"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True
        )

        for k in ["elgato", "cam link", "video capture", "hdmi"]:
            if k in result.stdout.lower():
                return True
        return False
    except Exception:
        return False


def check_video_signal(device_path):
    command = [
        "ffmpeg",
        "-f", "v4l2",
        "-i", device_path,
        "-frames:v", "1",
        "-vf", "signalstats,metadata=print:key=lavfi.signalstats.YAVG:file=-",
        "-f", "null", "-",
        "-v", "error"
    ]

    try:
        result = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=8
        )

        match = re.search(r"lavfi.signalstats.YAVG=([\d\.]+)", result.stdout)
        return bool(match)

    except Exception:
        return False


def start_sensor_capture(source_file, output_file, duration):
    """
    Runs:
    timeout <duration>s stdbuf -oL tail -n 0 -F source_file
    Writes output to output_file in real time
    """
    command = [
        "timeout", f"{duration}s",
        "stdbuf", "-oL",
        "tail", "-n", "0", "-F", source_file
    ]

    f = open(output_file, "w")
    proc = subprocess.Popen(command, stdout=f, stderr=subprocess.DEVNULL)
    return proc, f


def record_video(output_path, duration):
    command = [
        "ffmpeg",
        "-f", "v4l2",
        "-framerate", "25",
        "-video_size", "1920x1080",
        "-i", CAMERA_DEVICE,
        "-t", str(duration),
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-pix_fmt", "yuv420p",
        output_path
    ]
    subprocess.run(command, check=True)


def record(provided_id=None):
    if not usb_camera_connected():
        messagebox.showerror("Error", "Capture card not detected.")
        return

    if not check_video_signal(CAMERA_DEVICE):
        if not messagebox.askyesno("Warning", "No video signal detected.\nRecord anyway?"):
            return

    if provided_id:
        base = provided_id
    else:
        base = simpledialog.askstring(
            "Filename",
            "Enter Test ID:",
            initialvalue=DEFAULT_FILENAME
        )

    if not base:
        return

    cwd = os.getcwd()
    video_path = os.path.abspath(f"{cwd}/{base}.mp4")
    imu_out = os.path.abspath(f"{cwd}/{base}_imu.jsonl")
    gps_out = os.path.abspath(f"{cwd}/{base}_gps.jsonl")

    print("Starting IMU & GPS capture...")
    imu_proc, imu_file = start_sensor_capture(IMU_SOURCE, imu_out, DURATION_SECONDS)
    gps_proc, gps_file = start_sensor_capture(GPS_SOURCE, gps_out, DURATION_SECONDS)

    print("Recording video...")
    record_video(video_path, DURATION_SECONDS)

    # Wait for sensor capture to finish
    imu_proc.wait()
    imu_file.close()
    gps_proc.wait()
    gps_file.close()

    print("\n===== FILE LOCATIONS =====")
    print("Video:", video_path)
    print("IMU  :", imu_out)
    print("GPS  :", gps_out)
    print("==========================\n")

    messagebox.showinfo(
        "Success",
        f"Files saved:\n\n"
        f"Video:\n{video_path}\n\n"
        f"IMU:\n{imu_out}\n\n"
        f"GPS:\n{gps_out}"
    )


# ---------------- MAIN ----------------
if __name__ == "__main__":
    root = tk.Tk()
    root.withdraw()

    try:
        if len(sys.argv) > 1:
            record(sys.argv[1])
        else:
            record()
    except Exception as e:
        messagebox.showerror("Fatal Error", str(e))
