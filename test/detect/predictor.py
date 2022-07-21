import numpy as np
import multiprocessing as mp
import cv2
import torch
import os
from pathlib import Path

import detectron2
from detectron2.data import MetadataCatalog
from detectron2.engine.defaults import DefaultPredictor
from detectron2.utils.visualizer import ColorMode, Visualizer
from detectron2.config import get_cfg
from detectron2.structures import Boxes, Keypoints,RotatedBoxes

def setup_cfg():
    confidence_threshold =0.5
    detectron_path = Path(os.path.dirname(os.path.realpath(detectron2.__file__)))
    config_path = detectron_path.parent
    config_file = os.path.join(config_path, "configs", "COCO-Keypoints","keypoint_rcnn_R_50_FPN_3x.yaml")
    opts =list(("MODEL.WEIGHTS","detectron2://COCO-Keypoints/keypoint_rcnn_R_50_FPN_3x/137849621/model_final_a6e10b.pkl"))
    cfg = get_cfg()
    cfg.merge_from_file(config_file)
    cfg.merge_from_list(opts)
    # Set score_threshold for builtin models
    cfg.MODEL.RETINANET.SCORE_THRESH_TEST = confidence_threshold
    cfg.MODEL.ROI_HEADS.SCORE_THRESH_TEST = confidence_threshold
    cfg.MODEL.PANOPTIC_FPN.COMBINE.INSTANCES_CONFIDENCE_THRESH = confidence_threshold
    cfg.freeze()
    return cfg

class Predict(object):
    def __init__(self):
        cfg = setup_cfg()
        self.metadata = MetadataCatalog.get(
            cfg.DATASETS.TEST[0] if len(cfg.DATASETS.TEST) else "__unused"
        )
        self.cpu_device = torch.device("cpu")
        self.predictor = DefaultPredictor(cfg)
        self.predictions = None

    def Inference(self, image):
        res = None
        self.predictions = self.predictor(image)
        if "instances" in self.predictions:
            instances = self.predictions["instances"].to(self.cpu_device)
            res = self.parse_instance_predictions(instances)
        return res
    
    def convert_boxes(self,boxes):
        if isinstance(boxes, Boxes) or isinstance(boxes, RotatedBoxes):
            return boxes.tensor.numpy()
        else:
            return np.asarray(boxes)

    def convert_keypoints(self,keypoints):
        if isinstance(keypoints, Keypoints):
            keypoints = keypoints.tensor
        keypoints = np.asarray(keypoints)
        return keypoints

    def parse_instance_predictions(self,predictions):
        boxes = predictions.pred_boxes if predictions.has("pred_boxes") else None
        scores = predictions.scores if predictions.has("scores") else None
        classes = predictions.pred_classes if predictions.has("pred_classes") else None
        #labels = _create_text_labels(classes, scores, self.metadata.get("thing_classes", None))
        keypoints = predictions.pred_keypoints if predictions.has("pred_keypoints") else None
        boxes = self.convert_boxes(boxes)
        keypoints =self.convert_keypoints(keypoints)
        data ={
            "boxes":boxes,
            "keypoints":keypoints,
            "scores":np.asarray(scores),
            "classes":np.asarray(classes)
            }
        return data

    def show(self,img1,title):
        vis_output = None
        # Convert image from OpenCV BGR format to Matplotlib RGB format.
        visualizer = Visualizer(img1, self.metadata, instance_mode=ColorMode.IMAGE)
        if "panoptic_seg" in self.predictions:
            panoptic_seg, segments_info = self.predictions["panoptic_seg"]
            vis_output = visualizer.draw_panoptic_seg_predictions(
                panoptic_seg.to(self.cpu_device), segments_info
            )
        else:
            if "sem_seg" in self.predictions:
                vis_output = visualizer.draw_sem_seg(
                    self.predictions["sem_seg"].argmax(dim=0).to(self.cpu_device)
                )
            if "instances" in self.predictions:
                instances = self.predictions["instances"].to(self.cpu_device)
                vis_output = visualizer.draw_instance_predictions(predictions=instances)
        if vis_output != None:
            img_show =vis_output.get_image()[:, :, ::-1]
        else:
            img_show = img1
        cv2.imshow(title,img_show)
        cv2.waitKey(1)


