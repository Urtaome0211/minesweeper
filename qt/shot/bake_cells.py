# bake_cells.py — 将 harness2 的 GDI 地面真相 BMP 烘焙为 qt/src/cellpix.h
# digit 合成图: 30x30 不透明 ARGB
# mine R2..R7 黑白变体 / wrongx: 透明底 ARGB（哨兵 (1,2,3) → alpha 0）
import os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.normpath(os.path.join(HERE, "..", "src"))

def load_bmp(name):
    img = Image.open(os.path.join(HERE, name)).convert("RGBA")
    return list(img.tobytes())

def fmt_arrays(name, arrays):
    lines = ["inline const uint32_t %s[%d] = {" % (name, len(arrays))]
    for i in range(0, len(arrays), 12):
        chunk = arrays[i:i+12]
        lines.append("    " + ", ".join("0x%08Xu" % v for v in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)

SENTINEL = (1, 2, 3, 255)  # (1,2,3) 哨兵

def rgba_to_u32(r, g, b, a):
    return (a << 24) | (r << 16) | (g << 8) | b

def pack(data, transparent_ok):
    out = []
    n = len(data) // 4
    for i in range(n):
        r, g, b, a = data[4*i], data[4*i+1], data[4*i+2], data[4*i+3]
        if transparent_ok and (r, g, b) == (1, 2, 3):
            out.append(0)
        else:
            out.append(rgba_to_u32(r, g, b, 255))
    return out

parts = []
parts.append("// cellpix.h — 由 qt/shot/bake_cells.py 从 harness2 GDI 地面真相 BMP 生成，勿手改")
parts.append("// digit[0..7] = 数字 1..8 的整格合成图（翻开格渐变+边框+数字，30x30 不透明）")
parts.append("// mineBlack[mineRed][R-2] = R=2..7 的地雷 14x14（透明底，有效区域为其中心 2R x 2R 子矩形）")
parts.append("// wrongX = 错旗红叉 30x30（透明底）")
parts.append("#pragma once")
parts.append("#include <cstdint>")
parts.append("namespace cellpix {")

# digits 1..8
digits = []
for n in range(1, 9):
    data = load_bmp("cell_digit%d.bmp" % n)
    digits.append(pack(data, False))
parts.append(fmt_arrays("digit", [v for d in digits for v in d]))
parts.append("inline const int digitW = 30, digitH = 30;")

# mines
for color in ("black", "red"):
    arrs = []
    for R in range(2, 8):
        data = load_bmp("cell_mine_%s_R%d.bmp" % (color, R))
        arrs.append(pack(data, True))
    parts.append(fmt_arrays("mine%s" % color.capitalize(), [v for a in arrs for v in a]))
parts.append("inline const int mineW = 14, mineH = 14;")

# wrong X
parts.append(fmt_arrays("wrongX", pack(load_bmp("cell_wrongx.bmp"), True)))
parts.append("inline const int wrongW = 30, wrongH = 30;")

parts.append("inline const uint32_t* digitPix(int n) { return digit + (n - 1) * (digitW * digitH); }")
parts.append("inline const uint32_t* minePix(bool red, int R) {")
parts.append("    return red ? mineRed + (R - 2) * (mineW * mineH) : mineBlack + (R - 2) * (mineW * mineH);")
parts.append("}")
parts.append("} // namespace cellpix")

out_path = os.path.join(SRC, "cellpix.h")
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(parts) + "\n")
print("wrote", out_path)
