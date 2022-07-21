from test_class import test_class

pred = None

def init():
    global pred
    pred = test_class()
def run():
    pred.test(1,2)
