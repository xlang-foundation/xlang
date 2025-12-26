# XLang Test: Numeric Literals and Bitwise Operations
# Tests hex, binary, octal literals with underscores and bitwise ops

# ============================================
# Basic Numeric Literals
# ============================================

sci1 = 1e10
# Decimal with underscores
dec_num = 1_000_000
dec_small = 123_456

# Hexadecimal
hex_num = 0xFF
hex_upper = 0XFF
hex_large = 0xDEAD_BEEF
hex_color = 0xFF_00_FF

# Binary
bin_num = 0b1010
bin_upper = 0B1010
bin_byte = 0b1111_0000
bin_word = 0b1010_1010_1010_1010

# Octal
oct_num = 0o755
oct_upper = 0O755
oct_perm = 0o777_000

# ============================================
# Bitwise Operations
# ============================================

# AND operation
mask = 0b1111_0000
value = 0b1010_1010
result_and = value & mask  # Should be 0b1010_0000 = 160

# OR operation
flags1 = 0b0000_1111
flags2 = 0b1111_0000
result_or = flags1 | flags2  # Should be 0b1111_1111 = 255

# XOR operation
a = 0b1100_1100
b = 0b1010_1010
result_xor = a ^ b  # Should be 0b0110_0110 = 102

# NOT operation (complement)
byte_val = 0b0000_1111
result_not = ~byte_val  # Bitwise NOT

# Left shift
shift_val = 0b0000_0001
result_lshift = shift_val << 4  # Should be 0b0001_0000 = 16

# Right shift
shift_val2 = 0b1000_0000
result_rshift = shift_val2 >> 4  # Should be 0b0000_1000 = 8

# ============================================
# Practical Examples
# ============================================

# RGB Color manipulation
red = 0xFF_00_00
green = 0x00_FF_00
blue = 0x00_00_FF
white = red | green | blue  # 0xFFFFFF

# Extract color components
color = 0xAB_CD_EF
r = (color >> 16) & 0xFF  # 0xAB = 171
g = (color >> 8) & 0xFF   # 0xCD = 205
b = color & 0xFF          # 0xEF = 239

# Permission flags (Unix-style)
READ = 0b100    # 4
WRITE = 0b010   # 2
EXEC = 0b001    # 1

user_perm = READ | WRITE | EXEC  # 7 = rwx
group_perm = READ | EXEC         # 5 = r-x
other_perm = READ                # 4 = r--

# Combine into octal-style permission
full_perm = (user_perm << 6) | (group_perm << 3) | other_perm
# Should be 0o754

# Bit flags for options
FLAG_VERBOSE = 0b0000_0001
FLAG_DEBUG = 0b0000_0010
FLAG_FORCE = 0b0000_0100
FLAG_RECURSIVE = 0b0000_1000

options = FLAG_VERBOSE | FLAG_DEBUG
has_verbose = (options & FLAG_VERBOSE) != 0  # True
has_force = (options & FLAG_FORCE) != 0      # False

# ============================================
# Complex Numbers (if supported)
# ============================================
complex1 = 3j
complex2 = 0xFFj
complex3 = 0b1010j

# ============================================
# Scientific Notation
# ============================================
sci1 = 1e10
sci2 = 1.5e-3
sci3 = 2.5E+10
sci4 = 1_000e3

# ============================================
# Hex Float (Python 3 style)
# ============================================
hex_float1 = 0x1p10      # 1 * 2^10 = 1024
hex_float2 = 0x1.5p10    # 1.3125 * 2^10 = 1344
hex_float3 = 0x1.Fp-3    # 1.9375 * 2^-3 = 0.2421875

# ============================================
# Print results (assuming print function exists)
# ============================================
print("=== Basic Literals ===")
print("Decimal: ", dec_num)
print("Hex: ", hex_num)
print("Binary: ", bin_num)
print("Octal: ", oct_num)

print("\n=== Bitwise Results ===")
print("AND result: ", result_and)
print("OR result: ", result_or)
print("XOR result: ", result_xor)
print("Left shift: ", result_lshift)
print("Right shift: ", result_rshift)

print("\n=== Color Example ===")
print("White color: ", white)
print("Red component: ", r)
print("Green component: ", g)
print("Blue component: ", b)

print("\n=== Permission Example ===")
print("Full permission: ", full_perm)
