args = get_args()
print(args)
import sys
args = get_args()
for arg in args:
    sys.argv.append(arg)
x = "1234\t\nnew line\tafter tab"
print(x)
