#!/usr/bin/env python3
"""
Fast Protocol Extractor - Extract packet structures from method_bodies.txt

This tool finds Send_MSG_REQUEST methods and extracts their packet structures
by analyzing ARM64 assembly instructions.

Usage:
    python extract_protocols_fast.py <pattern> [count]

Examples:
    python extract_protocols_fast.py "HERO" 10      # Extract 10 hero protocols
    python extract_protocols_fast.py "MONSTER" 20   # Extract 20 monster protocols
    python extract_protocols_fast.py "QUEST" 15     # Extract 15 quest protocols
"""

import sys
import json
import re
from pathlib import Path

METHOD_INDEX = Path("full_dump/method_index.json")
METHOD_BODIES = Path("full_dump/method_bodies.txt")

def load_index():
    """Load method index for fast lookup"""
    with open(METHOD_INDEX) as f:
        return json.load(f)

def search_send_methods(index, pattern):
    """Find all Send_MSG_REQUEST methods matching pattern"""
    matches = []
    for method_name, info in index.items():
        if "Send_MSG_REQUEST" in method_name or "Send_MSG" in method_name:
            if pattern.upper() in method_name.upper():
                matches.append({
                    'name': method_name,
                    'class': info['cls'],
                    'rva': info['rva'],
                    'offset': info['offset'],
                    'fileline': info['fileline']
                })
    return matches

def extract_packet_type(method_body):
    """Extract packet type from method body (first mov w* instruction with immediate)"""
    # Look for patterns like: mov w8, #0x3001
    for line in method_body[:50]:  # Check first 50 lines
        match = re.search(r'mov\s+w\d+,\s+#(0x[0-9a-fA-F]+|#?\d+)', line)
        if match:
            value = match.group(1)
            if value.startswith('0x'):
                return int(value, 16)
            else:
                value = value.lstrip('#')
                return int(value)
    return None

def count_field_writes(method_body):
    """Count write operations to estimate field count"""
    writes = {
        'u8': 0,
        'u16': 0,
        'u32': 0,
        'u64': 0,
        'string': 0,
        'array': 0
    }

    for line in method_body:
        if 'strb' in line:
            writes['u8'] += 1
        elif 'strh' in line:
            writes['u16'] += 1
        elif re.search(r'str\s+w\d+', line):
            writes['u32'] += 1
        elif re.search(r'str\s+x\d+', line):
            writes['u64'] += 1
        elif 'writeString' in line or 'WriteString' in line:
            writes['string'] += 1
        elif 'memcpy' in line or 'WriteBytes' in line:
            writes['array'] += 1

    return writes

def extract_method_body(method_name, index):
    """Extract method body from method_bodies.txt"""
    if method_name not in index:
        return None

    fileline = index[method_name]['fileline']

    with open(METHOD_BODIES) as f:
        lines = []
        in_method = False
        line_num = 0

        for line in f:
            line_num += 1
            if line_num == fileline:
                in_method = True
                continue
            if in_method:
                if line.startswith("====="):
                    break
                lines.append(line.rstrip())

        return lines

def generate_c_protocol(method_info, packet_type, writes):
    """Generate C protocol function skeleton"""
    name = method_info['name']

    # Clean up method name for C function
    func_name = name.replace("Send_MSG_REQUEST_", "Request")
    func_name = func_name.replace("Send_MSG_", "Send")
    func_name = func_name.split('(')[0]  # Remove parameters

    # Generate C code
    c_code = []
    c_code.append(f"// Extracted from {method_info['class']}::{name}")
    c_code.append(f"// RVA: 0x{method_info['rva']:08X}")
    c_code.append(f"// Packet type: 0x{packet_type:04X} ({packet_type})")
    c_code.append(f"// Fields: {writes['u8']}×u8, {writes['u16']}×u16, {writes['u32']}×u32, {writes['u64']}×u64, {writes['string']}×string")
    c_code.append("")
    c_code.append(f"void {func_name}(Connection *c) {{")
    c_code.append(f"    uint16_t packet_type = 0x{packet_type:04X};")
    c_code.append(f"    ")
    c_code.append(f"    // TODO: Add parameters from method signature")
    c_code.append(f"    // TODO: Implement field writes based on ARM64 analysis")
    c_code.append(f"    ")

    if writes['u8'] > 0:
        c_code.append(f"    // {writes['u8']} × write_u8() calls")
    if writes['u16'] > 0:
        c_code.append(f"    // {writes['u16']} × write_u16() calls")
    if writes['u32'] > 0:
        c_code.append(f"    // {writes['u32']} × write_u32() calls")
    if writes['u64'] > 0:
        c_code.append(f"    // {writes['u64']} × write_u64() calls")
    if writes['string'] > 0:
        c_code.append(f"    // {writes['string']} × write_string() calls")

    c_code.append(f"    ")
    c_code.append(f"    // SendPacket(c, packet_type, buffer, size);")
    c_code.append(f"}}")
    c_code.append("")

    return '\n'.join(c_code)

def main():
    if len(sys.argv) < 2:
        print("Usage: python extract_protocols_fast.py <pattern> [count]")
        print("\nExamples:")
        print("  python extract_protocols_fast.py HERO 10")
        print("  python extract_protocols_fast.py MONSTER 20")
        print("  python extract_protocols_fast.py QUEST 15")
        sys.exit(1)

    pattern = sys.argv[1]
    max_count = int(sys.argv[2]) if len(sys.argv) > 2 else 10

    print(f"[*] Loading method index...")
    index = load_index()
    print(f"[*] Loaded {len(index)} methods")

    print(f"[*] Searching for {pattern} protocols...")
    methods = search_send_methods(index, pattern)
    print(f"[*] Found {len(methods)} matching methods")

    if not methods:
        print(f"[!] No methods found matching '{pattern}'")
        sys.exit(1)

    print(f"\n[*] Extracting top {min(max_count, len(methods))} protocols...\n")

    extracted = 0
    for method_info in methods[:max_count]:
        print(f"[+] {method_info['name']}")

        # Extract method body
        body = extract_method_body(method_info['name'], index)
        if not body:
            print(f"    [!] Failed to extract body")
            continue

        # Extract packet type
        packet_type = extract_packet_type(body)
        if not packet_type:
            print(f"    [!] Could not determine packet type")
            continue

        # Count field writes
        writes = count_field_writes(body)

        # Generate C code
        c_code = generate_c_protocol(method_info, packet_type, writes)

        # Save to file
        func_name = method_info['name'].replace("Send_MSG_REQUEST_", "Request")
        func_name = func_name.replace("Send_MSG_", "Send")
        func_name = func_name.split('(')[0]

        output_file = Path(f"extracted_protocols/{func_name}.c")
        output_file.parent.mkdir(exist_ok=True)

        with open(output_file, 'w') as f:
            f.write(c_code)

        print(f"    [OK] Packet: 0x{packet_type:04X}")
        print(f"    [OK] Fields: {sum(writes.values())} total")
        print(f"    [OK] Saved: {output_file}")
        print()

        extracted += 1

    print(f"\n[*] Extracted {extracted} protocols successfully!")
    print(f"[*] Output directory: extracted_protocols/")

if __name__ == "__main__":
    main()
