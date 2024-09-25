#
# Copyright (C) 2024 The XLang Foundation
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# <END>

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
