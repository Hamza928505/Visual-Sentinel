import xml.etree.ElementTree as ET
import os

# paths
xml_dir = r"C:\Users\حمزة\PycharmProjects\KAFD\.venv\DroneDatasetSnippet\Train\Drone_TrainSet_XMLs_100Snippet"
label_dir = r"C:\Users\حمزة\PycharmProjects\KAFD\.venv\DroneDatasetSnippet\Train\Drone_TrainSet_100Snippet"

os.makedirs(label_dir, exist_ok=True)

# class mapping
classes = {
    "drone": 0
}

for xml_file in os.listdir(xml_dir):
    if not xml_file.endswith(".xml"):
        continue

    tree = ET.parse(os.path.join(xml_dir, xml_file))
    root = tree.getroot()

    img_width = int(root.find("size/width").text)
    img_height = int(root.find("size/height").text)

    label_file = xml_file.replace(".xml", ".txt")
    label_path = os.path.join(label_dir, label_file)

    with open(label_path, "w") as f:
        for obj in root.findall("object"):
            class_name = obj.find("name").text
            class_id = classes[class_name]

            bndbox = obj.find("bndbox")
            xmin = float(bndbox.find("xmin").text)
            ymin = float(bndbox.find("ymin").text)
            xmax = float(bndbox.find("xmax").text)
            ymax = float(bndbox.find("ymax").text)

            # YOLO format
            x_center = ((xmin + xmax) / 2) / img_width
            y_center = ((ymin + ymax) / 2) / img_height
            width = (xmax - xmin) / img_width
            height = (ymax - ymin) / img_height

            f.write(f"{class_id} {x_center} {y_center} {width} {height}\n")

print("Conversion completed!")
