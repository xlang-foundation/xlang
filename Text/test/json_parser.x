import json

s ="None"
s1 = json.loads(s)

s ="dasgfsgsdgsg"
s2 = json.loads(s)


s = '"{\\n    \"boxes\": [\\n        [\\n            464.8609924316406,\\n            232.23272705078125,\\n            764.869384765625,\\n            593.9711303710938\\n        ],\\n        [\\n            470.4544677734375,\\n            244.43260192871094,\\n            755.2083740234375,\\n            589.3258666992188\\n        ]\\n    ],\\n    \"keypoints\": null,\\n    \"scores\": [\\n        0.7679688334465027,\\n        0.5135981440544128\\n    ],\\n    \"classes\": [\\n        6,\\n        5\\n    ]\\n}"'


js = json.loads(s,normalize=True)
print(js)
print(js["boxes"])
print("Done")