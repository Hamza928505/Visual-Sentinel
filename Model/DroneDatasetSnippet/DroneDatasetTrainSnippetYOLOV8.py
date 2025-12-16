from ultralytics import YOLO

if __name__ == "__main__":
    model = YOLO("yolov8m.pt") # use X

    results = model.train(
        data="data.yaml",
        epochs=5,
        imgsz=640,        # BIG improvement for small drones
        # multi_scale=True,
        batch=8,           # reduce because of high imgsz set 4
        workers=0,
        device=0,
        name="DroneDatasetTrainSnippet_yolo_drone_m_1280",
        # split=0.6,         # 60% train, 40% val
        val=True,

        # --- Small object improvements ---
        mosaic=1.0,        # enable mosaic
        close_mosaic=10,   # turn off mosaic after 10 epochs
        degrees=5,         # small rotation
        scale=0.5,         # zoom augmentation
        translate=0.1,
        flipud=0.0,        # drones rarely upside down
        fliplr=0.5,        # horizontal flip
        hsv_h=0.015,
        hsv_s=0.7,
        hsv_v=0.4,
    )

    print("\n==== TRAIN METRICS ====")
    metrics = results.results_dict

    print("mAP@0.5:", metrics["metrics/mAP50(B)"])
    print("mAP@0.5:0.95:", metrics["metrics/mAP50-95(B)"])
    print("Precision:", metrics["metrics/precision(B)"])
    print("Recall:", metrics["metrics/recall(B)"])