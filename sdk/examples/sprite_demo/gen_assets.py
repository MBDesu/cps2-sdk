#!/usr/bin/env python3
import os, struct, zlib

def write_png(path, w, h, pixels_rgba):
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        row = pixels_rgba[y * w * 4 : (y + 1) * w * 4]
        raw.extend(row)
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)

def make_hero():
    # 32x16: two 16x16 frames (Frame 0 = stand, Frame 1 = stride)
    w, h = 32, 16
    px = bytearray(w * h * 4)
    pal = {
        '.': (0, 0, 0, 0),
        'K': (20, 20, 35, 255),
        'B': (50, 140, 255, 255),
        'C': (120, 230, 255, 255),
        'W': (245, 245, 255, 255),
        'R': (240, 70, 90, 255),
        'Y': (255, 210, 60, 255),
    }
    frame0 = [
        "....KKKKKK......",
        "...KBBBBBBK.....",
        "..KBBBBBBBBK....",
        "..KBCCCCCCBK....",
        "..KBCCCCCCBK....",
        "..KBBBBBBBBK....",
        "...KKKKKKKK.....",
        "..KRRRRRRRRK....",
        ".KBRRRRRRRRBK...",
        ".KBKRRRRRRKBK...",
        ".KWKRRRRRRKWK...",
        "..K.KKKKKK.K....",
        "....KBBBBK......",
        "...KBBKKBBK.....",
        "...KKK..KKK.....",
        "................",
    ]
    frame1 = [
        "....KKKKKK......",
        "...KBBBBBBK.....",
        "..KBBBBBBBBK....",
        "..KBCCCCCCBK....",
        "..KBCCCCCCBK....",
        "..KBBBBBBBBK....",
        "...KKKKKKKK.....",
        "..KRRRRRRRRK....",
        ".KKBRRRRRRBKK...",
        ".KWKRRRRRRKWK...",
        "..K.KKKKKK.K....",
        "...KBBBBBBK.....",
        "..KBBK..KBBK....",
        ".KKK......KKK...",
        "................",
        "................",
    ]
    for f_idx, rows in enumerate([frame0, frame1]):
        ox = f_idx * 16
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                r, g, b, a = pal[ch]
                off = (y * w + (ox + x)) * 4
                px[off:off+4] = bytes([r, g, b, a])
    write_png("assets/hero.png", w, h, px)

def make_bg():
    # 32x16: Tile 0 = starfield (Scroll 3), Tile 1 = neon grid block (Scroll 2)
    w, h = 32, 16
    px = bytearray(w * h * 4)
    # Tile 0: deep space + twinkling pixels
    for y in range(16):
        for x in range(16):
            off = (y * w + x) * 4
            if (x == 3 and y == 4) or (x == 11 and y == 12):
                px[off:off+4] = bytes([240, 240, 255, 255])
            elif (x == 13 and y == 2) or (x == 6 and y == 10):
                px[off:off+4] = bytes([100, 170, 255, 255])
            else:
                px[off:off+4] = bytes([16, 20, 48, 255])
    # Tile 1: beveled metallic grid floor
    for y in range(16):
        for x in range(16):
            off = (y * w + 16 + x) * 4
            if x == 0 or y == 0:
                px[off:off+4] = bytes([80, 210, 230, 255])
            elif x == 15 or y == 15:
                px[off:off+4] = bytes([25, 65, 95, 255])
            else:
                px[off:off+4] = bytes([35, 95, 130, 255])
    write_png("assets/bg_tiles.png", w, h, px)

if __name__ == "__main__":
    os.makedirs("assets", exist_ok=True)
    make_hero()
    make_bg()

