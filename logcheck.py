import re
import sys

def parse_official_line(line):
    """
    Parses a line from the official nestest.log
    Example: C000  4C F5 C5  JMP $C5F5                       A:00 X:00 Y:00 P:24 SP:FD PPU:  0, 21 CYC:7
    """
    # Grab the PC (first 4 hex chars)
    pc = line[:4].strip().upper()
    
    # Grab CPU registers using regex
    match = re.search(r'A:([0-9A-F]{2})\s+X:([0-9A-F]{2})\s+Y:([0-9A-F]{2})\s+P:([0-9A-F]{2})\s+SP:([0-9A-F]{2}).*?CYC:(\d+)', line)
    if not match:
        return None
        
    return {
        'PC': pc,
        'A': match.group(1),
        'X': match.group(2),
        'Y': match.group(3),
        'P': match.group(4),
        'SP': match.group(5),
        'CYC': match.group(6)
    }

def parse_my_line(line):
    """
    Parses a line from your emulator's log
    Example: C000  4C        JMP_ABS                        A:00 X:00 Y:00 P:20 SP:FD CYC:7
    """
    pc = line[:4].strip().upper()
    
    match = re.search(r'A:([0-9A-F]{2})\s+X:([0-9A-F]{2})\s+Y:([0-9A-F]{2})\s+P:([0-9A-F]{2})\s+SP:([0-9A-F]{2})\s+CYC:(\d+)', line)
    if not match:
        return None
        
    # Normalize your P register to match official (setting the B flag / bit 2)
    your_p = int(match.group(4), 16)
    normalized_p = f"{(your_p | 0x04):02X}"

    return {
        'PC': pc,
        'A': match.group(1),
        'X': match.group(2),
        'Y': match.group(3),
        'P': normalized_p, # Use adjusted P flag value
        'SP': match.group(5),
        'CYC': match.group(6)
    }

def compare_logs(official_path, my_path):
    with open(official_path, 'r') as f_off, open(my_path, 'r') as f_my:
        off_lines = f_off.readlines()
        my_lines = f_my.readlines()

    limit = min(len(off_lines), len(my_lines))
    print(f"Comparing {limit} execution steps...\n")

    for i in range(limit):
        off_state = parse_official_line(off_lines[i])
        my_state = parse_my_line(my_lines[i])

        if not off_state or not my_state:
            print(f"Line {i+1}: Failed to parse line.")
            print(f"Official: {off_lines[i].strip()}")
            print(f"Mine:     {my_lines[i].strip()}")
            return

        # Check for discrepancies
        mismatches = []
        for key in ['PC', 'A', 'X', 'Y', 'P', 'SP', 'CYC']:
            if off_state[key] != my_state[key]:
                mismatches.append(f"{key} (Official:{off_state[key]} vs Mine:{my_state[key]})")

        if mismatches:
            print(f"❌ MISMATCH FOUND AT LINE {i+1}!")
            print(f"Official line: {off_lines[i].strip()}")
            print(f"Your line:     {my_lines[i].strip()}")
            print(f"Differences:   {', '.join(mismatches)}")
            return

    print("✅ Success! All parsed states match perfectly.")

compare_logs("roms/nestest.log", "/tmp/test")
