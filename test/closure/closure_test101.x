def make_counter():
    n = 0
    def inc():
        extern n
        n = n + 1
        return n
    return inc

c1 = make_counter()
c2 = make_counter()

print("c1:", c1())  # expect 1
print("c1:", c1())  # expect 2
print("c2:", c2())  # expect 1
print("c2:", c2())  # expect 2
print("c1:", c1())  # expect 3
