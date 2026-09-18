#!/usr/bin/env python3
"""
Extract C protocol implementations from ARM64 method bodies.

This script reads method_bodies.txt (disassembled ARM64 from libil2cpp.so)
and generates C code for packet senders by analyzing the actual binary code.

Usage:
    python extract_protocol_from_bodies.py "Send_MSG_REQUEST_HERO" > hero_protocol.c
"""

import sys
import json
import re
from pathlib import Path

BODIES_PATH = Path(__file__).parent / "full_dump" / "method_bodies.txt"
INDEX_PATH = Path(__file__).parent / "full_dump" / "method_index.json"

# ARM64 instruction patterns for network serialization
WRITE_PATTERNS = {
    'write_u8': r'strb',
    'write_u16': r'strh',
    'write_u32': r'str\s+w\d+',
    'write_u64': r'str\s+x\d+',
    'write_string': r'bl.*String',
}

def load_index():
    """Load method index for quick lookup."""
    with open(INDEX_PATH) as f:
        return json.load(f)

def extract_method_body(method_name, index):
    """Extract the full method body from method_bodies.txt."""
    if method_name not in index:
        return None

    fileline = index[method_name]['fileline']

    # Read the method body starting from the fileline
    with open(BODIES_PATH) as f:
        lines = []
        in_method = False
        for i, line in enumerate(f, 1):
            if i == fileline:
                in_method = True
            if in_method:
                lines.append(line.rstrip())
                # Next method starts with "====="
                if line.startswith("=====") and len(lines) > 1:
                    break

    return lines[:-1] if lines else None  # Remove the next method header

def analyze_write_sequence(body_lines):
    """Analyze ARM64 instructions to identify packet field writes."""
    writes = []

    for line in body_lines:
        # Look for store instructions (strb, strh, str)
        if 'strb' in line:
            writes.append({'type': 'u8', 'instr': line.strip()})
        elif 'strh' in line:
            writes.append({'type': 'u16', 'instr': line.strip()})
        elif re.search(r'str\s+w\d+', line):
            writes.append({'type': 'u32', 'instr': line.strip()})
        elif re.search(r'str\s+x\d+', line):
            writes.append({'type': 'u64', 'instr': line.strip()})
        # Look for function calls (might be write_string, memcpy, etc.)
        elif 'bl' in line and '0x' in line:
            writes.append({'type': 'call', 'instr': line.strip()})

    return writes

def generate_c_function(method_name, signature, body_lines, cls_name):
    """Generate C function from ARM64 method body analysis."""

    # Parse parameters from signature
    params_match = re.search(r'\((.*?)\)', signature)
    params = []
    if params_match:
        param_str = params_match.group(1).strip()
        if param_str:
            # Simple parsing - may need enhancement
            for param in param_str.split(','):
                param = param.strip()
                if param:
                    params.append(param)

    # Analyze write sequence
    writes = analyze_write_sequence(body_lines)

    # Generate C code
    c_code = []
    c_code.append(f"// Extracted from {cls_name}::{method_name}")
    c_code.append(f"// ARM64 body size: {len(body_lines)} lines")

    # Function signature
    func_name = method_name.replace("Send_MSG_REQUEST_", "Request")
    func_name = func_name.replace("Send_MSG_", "Send")

    c_params = ["Connection *c"]
    for param in params:
        # Convert C# types to C types
        c_type = param.replace("byte", "uint8_t").replace("ushort", "uint16_t")
        c_type = c_type.replace("uint", "uint32_t").replace("ulong", "uint64_t")
        c_type = c_type.replace("CString", "const char*").replace("string", "const char*")
        c_type = c_type.replace("bool", "bool")
        c_params.append(c_type)

    c_code.append(f"void {func_name}({', '.join(c_params)})")
    c_code.append("{")
    c_code.append("    PacketBuffer pb = {0};")
    c_code.append("    // TODO: Determine packet_type from method analysis")
    c_code.append("    uint16_t packet_type = 0xXXXX;  // FIXME")
    c_code.append("")
    c_code.append("    // Field writes detected from ARM64:")

    for i, write in enumerate(writes[:20]):  # Limit to first 20 for readability
        if write['type'] == 'u8':
            c_code.append(f"    // write_u8: {write['instr']}")
        elif write['type'] == 'u16':
            c_code.append(f"    // write_u16: {write['instr']}")
        elif write['type'] == 'u32':
            c_code.append(f"    // write_u32: {write['instr']}")
        elif write['type'] == 'u64':
            c_code.append(f"    // write_u64: {write['instr']}")
        elif write['type'] == 'call':
            c_code.append(f"    // function call: {write['instr']}")

    if len(writes) > 20:
        c_code.append(f"    // ... and {len(writes) - 20} more writes")

    c_code.append("")
    c_code.append("    // TODO: Implement actual serialization based on analysis")
    c_code.append("    // SendPacket(c, packet_type, pb.data, pb.size);")
    c_code.append("}")
    c_code.append("")

    return '\n'.join(c_code)

def search_methods(pattern):
    """Search for methods matching pattern."""
    index = load_index()
    matches = []

    for method_name in index:
        if pattern.lower() in method_name.lower():
            matches.append(method_name)

    return matches

def main():
    if len(sys.argv) < 2:
        print("Usage: python extract_protocol_from_bodies.py <search_pattern>")
        print("\nExample:")
        print("  python extract_protocol_from_bodies.py \"HERO\"")
        print("  python extract_protocol_from_bodies.py \"REQUEST_ADVENTURE\"")
        sys.exit(1)

    pattern = sys.argv[1]
    index = load_index()

    # Search for matching methods
    matches = search_methods(pattern)

    if not matches:
        print(f"No methods found matching: {pattern}", file=sys.stderr)
        sys.exit(1)

    print(f"// Found {len(matches)} methods matching '{pattern}'", file=sys.stderr)
    print(f"// Extracted from method_bodies.txt (ARM64 disassembly)", file=sys.stderr)
    print(file=sys.stderr)

    # Generate C code for each match
    for method_name in matches[:10]:  # Limit to first 10
        method_info = index[method_name]
        body_lines = extract_method_body(method_name, index)

        if body_lines:
            cls_name = method_info['cls']
            c_code = generate_c_function(method_name, method_name, body_lines, cls_name)
            print(c_code)

        print(f"// Processed: {method_name}", file=sys.stderr)

    if len(matches) > 10:
        print(f"// ... and {len(matches) - 10} more methods", file=sys.stderr)
        print(f"// Use more specific pattern to narrow down", file=sys.stderr)

if __name__ == "__main__":
    main()
