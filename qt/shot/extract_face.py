# 生成 facepix.h：把 GDI 地面真相（face_harness 离线渲染的三种状态笑脸 BMP）
# 烘焙为 2bit/px 像素数据。每行 42 像素 → 11 字节（末字节只用低 4 位）。
# 编码：0=透明 1=黄 2=黑 3=白；字节内像素 x=4k..4k+3 依次占低 2 位到高 2 位。
import sys
sys.stdout.reconfigure(encoding="utf-8")
from PIL import Image
import numpy as np

names = {"playing": "facePlaying", "lost": "faceLost", "won": "faceWon"}
out = ["// facepix.h — 由 GDI 地面真相（face_harness 离线渲染）烘焙的笑脸像素，2bit/px：",
       "// 0=透明 1=黄 2=黑 3=白；每行 42 像素占 11 字节（末字节只用低 4 位），",
       "// 字节内像素 x=4k..4k+3 依次占低 2 位到高 2 位。",
       "// 原因：GDI 小椭圆光栅化存在退化行为（3x3 椭圆画不出、圆环上下不对称），",
       "// QPainter 无法用常规几何复现，故直接烘焙原版渲染结果以保证逐像素一致。",
       "#pragma once",
       "#include <cstdint>",
       ""]
BYTES = (42 + 3) // 4  # 11

for key, cname in names.items():
    a = np.asarray(Image.open(f"face_{key}.bmp").convert("RGB"))
    counts = [0, 0, 0, 0]
    data = []
    for y in range(42):
        for x in range(0, 42, 4):
            byte = 0
            for k in range(4):
                if x + k < 42:
                    r, g, b = a[y, x + k]
                    if (r, g, b) == (255, 255, 0):
                        v = 1
                    elif (r, g, b) == (0, 0, 0):
                        v = 2
                    elif (r, g, b) == (255, 255, 255):
                        v = 3
                    else:
                        v = 0
                else:
                    v = 0
                counts[v] += 1
                byte |= v << (2 * k)
            data.append(byte)
    print(f"{key}: 透明{counts[0]} 黄{counts[1]} 黑{counts[2]} 白{counts[3]}")
    out.append(f"inline const uint8_t {cname}[42 * {BYTES}] = {{")
    for i in range(0, len(data), 12):
        out.append("    " + ",".join(f"0x{b:02x}" for b in data[i:i + 12]) + ",")
    out.append("};")
    out.append("")

with open("qt/src/facepix.h", "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(out))
print("已生成 qt/src/facepix.h")
