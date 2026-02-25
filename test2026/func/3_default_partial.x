
print("CHECKPOINTS: cp1, cp2, cp3")

# cp1: provide first arg only — second and third use defaults
def make_label(prefix, sep="-", suffix="end"):
    return prefix + sep + suffix

r1 = make_label("start")
print("(cp1) label:", r1)   # start-end

# cp2: provide first two args — third uses default
r2 = make_label("start", ":")
print("(cp2) label:", r2)   # start:end

# cp3: provide all args — no defaults used
r3 = make_label("start", ".", "finish")
print("(cp3) label:", r3)   # start.finish
