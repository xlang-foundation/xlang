import time

# Module-level base ID - generated once when module loads
_BASE_ID = None

def get_base_id():
    """
    Get base ID (3 bytes / 24 bits) from epoch milliseconds.
    Generated once per module load, cached for reuse.
    Unique window: ~4.6 hours
    """
    global _BASE_ID
    if _BASE_ID is None:
        _BASE_ID = int(time.time() * 1000) & 0xFFFFFF
    return _BASE_ID

def reset_base_id():
    """Force regenerate base ID (call on module reload if needed)"""
    global _BASE_ID
    _BASE_ID = int(time.time() * 1000) & 0xFFFFFF
    return _BASE_ID

def build_unique_tracking_id(base_id, tracking_id):
    """
    Combine base ID (3 bytes) + tracking ID (5 bytes) into 8-byte unique ID.
    
    Args:
        base_id: 24-bit base identifier (from get_base_id())
        tracking_id: 40-bit tracking sequence (from tracker)
    
    Returns:
        64-bit unique tracking ID
    """
    return ((base_id & 0xFFFFFF) << 40) | (tracking_id & 0xFFFFFFFFFF)

def extract_ids(unique_id):
    """
    Extract base ID and tracking ID from combined unique ID.
    
    Returns:
        (base_id, tracking_id)
    """
    base_id = (unique_id >> 40) & 0xFFFFFF
    tracking_id = unique_id & 0xFFFFFFFFFF
    return base_id, tracking_id


# Test code
print("=== Testing Unique Tracking ID ===")

base = get_base_id()
print("Base ID:", base, "Hex: 0x" + format(base, "06X"))

# Test with different tracking IDs
test_ids = [1, 2, 100, 9999, 1099511627775]

for track_id in test_ids:
    unique = build_unique_tracking_id(base, track_id)
    extracted_base, extracted_track = extract_ids(unique)
    print("")
    print("Track ID:", track_id)
    print("  Unique ID:", unique, "Hex: 0x" + format(unique, "016X"))
    print("  Extracted Base:", extracted_base, "Track:", extracted_track)
    print("  Match:", extracted_base == base and extracted_track == track_id)

# Test reset
print("")
print("=== Testing Reset ===")
time.sleep(0.01)
new_base = reset_base_id()
print("New Base ID:", new_base, "Hex: 0x" + format(new_base, "06X"))
print("Changed:", new_base != base)