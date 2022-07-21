from detectron2.data.detection_utils import read_image
from predictor import Predict

pred = None

def init():
    global pred
    pred = Predict()

def run_one():
    print("in run_one")
    img = read_image("C:/Dev/X/test/detect/1.jpg", format="BGR")
    data = pred.Inference(img)
    print("data=",data)
    pred.show(img,"test")
    return data
