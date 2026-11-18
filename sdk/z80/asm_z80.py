#!/usr/bin/env python3
import re
import sys

REG8 = {"B": 0, "C": 1, "D": 2, "E": 3, "H": 4, "L": 5, "(HL)": 6, "A": 7}
REG16 = {"BC": 0, "DE": 1, "HL": 2, "SP": 3}
REG16_PUSH = {"BC": 0, "DE": 1, "HL": 2, "AF": 3}
CC_JR = {"NZ": 0x20, "Z": 0x28, "NC": 0x30, "C": 0x38}
CC_JP = {"NZ": 0xC2, "Z": 0xCA, "NC": 0xD2, "C": 0xDA}


def parse_num(tok: str) -> int:
    tok = tok.strip()
    if tok.startswith("0X") or tok.startswith("0x"):
        return int(tok, 16)
    if tok.endswith("H") or tok.endswith("h"):
        return int(tok[:-1], 16)
    return int(tok, 10)


def split_operands(op_str: str) -> list[str]:
    if not op_str:
        return []
    return [p.strip() for p in op_str.split(",")]


def assemble(
    src_path: str,
    out_path: str,
    rom_size: int = 0x20000,
    header_path: str | None = None,
) -> None:
    with open(src_path, "r", encoding="utf-8") as f:
        raw_lines = f.readlines()

    parsed: list[tuple[int, str | None, str | None, list[str]]] = []
    for lineno, line in enumerate(raw_lines, 1):
        code = line.split(";", 1)[0].strip()
        if not code:
            continue
        label = None
        if ":" in code:
            lbl, rest = code.split(":", 1)
            label = lbl.strip()
            code = rest.strip()
        if not code:
            parsed.append((lineno, label, None, []))
            continue
        parts = re.split(r"\s+", code, maxsplit=1)
        mnem = parts[0].upper()
        ops = split_operands(parts[1]) if len(parts) > 1 else []
        parsed.append((lineno, label, mnem, ops))

    labels: dict[str, int] = {}

    def eval_expr(expr: str, pass_num: int) -> int:
        expr_u = expr.strip()
        for k, v in labels.items():
            if expr_u == k:
                return v
        if pass_num == 1 and re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", expr_u):
            return 0
        return parse_num(expr_u)

    def encode_line(lineno: int, pc: int, mnem: str, ops: list[str], pass_num: int) -> tuple[int, bytes]:
        if mnem == "ORG":
            return eval_expr(ops[0], pass_num), b""

        if mnem == "DI":
            return pc + 1, bytes([0xF3])
        if mnem == "EI":
            return pc + 1, bytes([0xFB])
        if mnem == "NOP":
            return pc + 1, bytes([0x00])
        if mnem == "IM" and ops[0] == "1":
            return pc + 2, bytes([0xED, 0x56])
        if mnem == "RET":
            if not ops:
                return pc + 1, bytes([0xC9])
            cc = ops[0].upper()
            if cc == "NZ":
                return pc + 1, bytes([0xC0])
            if cc == "Z":
                return pc + 1, bytes([0xC8])
        if mnem == "EX" and ops[0].upper() == "DE" and ops[1].upper() == "HL":
            return pc + 1, bytes([0xEB])

        if mnem == "PUSH":
            r = REG16_PUSH[ops[0].upper()]
            return pc + 1, bytes([0xC5 | (r << 4)])
        if mnem == "POP":
            r = REG16_PUSH[ops[0].upper()]
            return pc + 1, bytes([0xC1 | (r << 4)])

        if mnem == "INC":
            op = ops[0].upper()
            if op in REG8:
                return pc + 1, bytes([0x04 | (REG8[op] << 3)])
            if op in REG16:
                return pc + 1, bytes([0x03 | (REG16[op] << 4)])
        if mnem == "DEC":
            op = ops[0].upper()
            if op in REG8:
                return pc + 1, bytes([0x05 | (REG8[op] << 3)])
            if op in REG16:
                return pc + 1, bytes([0x0B | (REG16[op] << 4)])

        if mnem == "XOR":
            op = ops[0].upper()
            if op in REG8:
                return pc + 1, bytes([0xA8 | REG8[op]])
        if mnem == "OR":
            op = ops[0].upper()
            if op in REG8:
                return pc + 1, bytes([0xB0 | REG8[op]])
        if mnem == "AND":
            op = ops[0].upper()
            if op in REG8:
                return pc + 1, bytes([0xA0 | REG8[op]])
            val = eval_expr(ops[0], pass_num) & 0xFF
            return pc + 2, bytes([0xE6, val])
        if mnem == "CP":
            op = ops[0].upper()
            if op in REG8:
                return pc + 1, bytes([0xB8 | REG8[op]])
            val = eval_expr(ops[0], pass_num) & 0xFF
            return pc + 2, bytes([0xFE, val])
        if mnem == "ADD":
            dst, src = ops[0].upper(), ops[1].upper()
            if dst == "A":
                if src in REG8:
                    return pc + 1, bytes([0x80 | REG8[src]])
                val = eval_expr(ops[1], pass_num) & 0xFF
                return pc + 2, bytes([0xC6, val])
            if dst == "HL" and src in REG16:
                return pc + 1, bytes([0x09 | (REG16[src] << 4)])

        if mnem == "SUB":
            op = ops[0].upper()
            if op in REG8:
                return pc + 1, bytes([0x90 | REG8[op]])
            val = eval_expr(ops[0], pass_num) & 0xFF
            return pc + 2, bytes([0xD6, val])

        if mnem == "JR":
            if len(ops) == 1:
                target = eval_expr(ops[0], pass_num)
                disp = 0 if pass_num == 1 else (target - (pc + 2))
                if pass_num == 2 and not (-128 <= disp <= 127):
                    raise ValueError(f"Line {lineno}: JR target out of range ({disp})")
                return pc + 2, bytes([0x18, disp & 0xFF])
            cc = CC_JR[ops[0].upper()]
            target = eval_expr(ops[1], pass_num)
            disp = 0 if pass_num == 1 else (target - (pc + 2))
            if pass_num == 2 and not (-128 <= disp <= 127):
                raise ValueError(f"Line {lineno}: JR {ops[0]} out of range ({disp})")
            return pc + 2, bytes([cc, disp & 0xFF])

        if mnem == "JP":
            if len(ops) == 1:
                target = eval_expr(ops[0], pass_num)
                return pc + 3, bytes([0xC3, target & 0xFF, (target >> 8) & 0xFF])
            cc = CC_JP[ops[0].upper()]
            target = eval_expr(ops[1], pass_num)
            return pc + 3, bytes([cc, target & 0xFF, (target >> 8) & 0xFF])

        if mnem == "CALL":
            target = eval_expr(ops[0], pass_num)
            return pc + 3, bytes([0xCD, target & 0xFF, (target >> 8) & 0xFF])

        if mnem == "LD":
            dst, src = ops[0], ops[1]
            dst_u, src_u = dst.upper(), src.upper()

            if dst_u in REG8 and src_u in REG8:
                return pc + 1, bytes([0x40 | (REG8[dst_u] << 3) | REG8[src_u]])

            if dst_u == "A" and src_u.startswith("(") and src_u.endswith(")"):
                addr = eval_expr(src[1:-1], pass_num)
                return pc + 3, bytes([0x3A, addr & 0xFF, (addr >> 8) & 0xFF])

            if src_u == "A" and dst_u.startswith("(") and dst_u.endswith(")"):
                addr = eval_expr(dst[1:-1], pass_num)
                return pc + 3, bytes([0x32, addr & 0xFF, (addr >> 8) & 0xFF])

            if dst_u == "HL" and src_u.startswith("(") and src_u.endswith(")"):
                addr = eval_expr(src[1:-1], pass_num)
                return pc + 3, bytes([0x2A, addr & 0xFF, (addr >> 8) & 0xFF])

            if src_u == "HL" and dst_u.startswith("(") and dst_u.endswith(")"):
                addr = eval_expr(dst[1:-1], pass_num)
                return pc + 3, bytes([0x22, addr & 0xFF, (addr >> 8) & 0xFF])

            if dst_u == "(HL)":
                val = eval_expr(src, pass_num) & 0xFF
                return pc + 2, bytes([0x36, val])

            if dst_u in REG8:
                val = eval_expr(src, pass_num) & 0xFF
                return pc + 2, bytes([0x06 | (REG8[dst_u] << 3), val])

            if dst_u in REG16:
                val = eval_expr(src, pass_num) & 0xFFFF
                return pc + 3, bytes([0x01 | (REG16[dst_u] << 4), val & 0xFF, (val >> 8) & 0xFF])

        raise ValueError(f"Line {lineno}: unsupported Z80 instruction '{mnem} {', '.join(ops)}'")

    # Pass 1: collect labels
    pc = 0
    for lineno, label, mnem, ops in parsed:
        if label:
            labels[label] = pc
        if mnem:
            pc, _ = encode_line(lineno, pc, mnem, ops, 1)

    # Pass 2: emit bytes
    rom = bytearray(rom_size)
    pc = 0
    for lineno, label, mnem, ops in parsed:
        if mnem:
            next_pc, chunk = encode_line(lineno, pc, mnem, ops, 2)
            rom[pc : pc + len(chunk)] = chunk
            pc = next_pc

    with open(out_path, "wb") as f:
        f.write(rom)

    if header_path:
        used_len = pc if pc > 0 else 1
        trimmed = rom[:used_len]
        with open(header_path, "w", encoding="utf-8") as hf:
            hf.write("#pragma once\n#include <cstdint>\n\n")
            hf.write("inline constexpr uint8_t kDefaultZ80Driver[] = {\n")
            for i in range(0, len(trimmed), 16):
                chunk = ", ".join(f"0x{b:02x}u" for b in trimmed[i : i + 16])
                hf.write(f"    {chunk},\n")
            hf.write("};\n")


if __name__ == "__main__":
    if len(sys.argv) == 3:
        assemble(sys.argv[1], sys.argv[2])
    elif len(sys.argv) == 5 and sys.argv[3] == "--header":
        assemble(sys.argv[1], sys.argv[2], header_path=sys.argv[4])
    else:
        print(
            "Usage: asm_z80.py <input.s> <output.bin> [--header <output.h>]",
            file=sys.stderr,
        )
        sys.exit(1)
