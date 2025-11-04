# XLang Test Script for JSON

import json

print("=== Test 1: None string ===")
s = "None"
s1 = json.loads(s)
print("Input:", s)
print("Result:", s1)
print("Type:", type(s1))
print()

print("=== Test 2: Invalid JSON string ===")
s = "dasgfsgsdgsg"
s2 = json.loads(s)
print("Input:", s)
print("Result:", s2)
print("Type:", type(s2))
print()

print("=== Test 3: JSON object with nested arrays ===")
s = '''{
    "boxes": [
        [
            464.8609924316406,
            232.23272705078125,
            764.869384765625,
            593.9711303710938
        ],
        [
            470.4544677734375,
            244.43260192871094,
            755.2083740234375,
            589.3258666992188
        ]
    ],
    "keypoints": null,
    "scores": [
        0.7679688334465027,
        0.5135981440544128
    ],
    "classes": [
        6,
        5
    ]
}'''

print("Input JSON string length:", len(s))
js = json.loads(s)

js2 = json.from_json(s)

print("Result type:", type(js))
print("Number of boxes:", len(js['boxes']))
print("Keypoints:", js['keypoints'])
print("Scores:", js['scores'])
print("Classes:", js['classes'])
print()

print("=== Test 4: Converting back to JSON string ===")
strJs = json.dumps(js)
print("Serialized JSON :", strJs)

strJs2 = str(js)
print("Serialized JSON-use str :", strJs2)


print("Done")