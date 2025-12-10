from PIL import Image
import glob
import numpy as np
import os

kaas = open("furby.c","w")
kaas.write("#include <stdint.h>\n")
for fr in glob.glob("frames/*.bmp"):
    img = np.array(Image.open(fr).resize((16,13), Image.Resampling.LANCZOS).convert('L'))

    name = fr.split("frame-")[1].split(".bmp")[0]
    s = ""
    for y in range(16):
        s += ""
        for x in range(16):
            try:
                s += "0x%02X, "%img[x,y]
            except:
                s += "0x00, "
            # s += "0x%02X, "%(y*16+x)
        s += "\n"
    kaas.write("uint8_t frame_%s[256] = { %s };\n"%(name, s))
kaas.close()