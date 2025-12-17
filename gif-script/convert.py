from PIL import Image, ImageEnhance
import glob
import numpy as np
import os

# HEY THERE! There are some tunables!
imageName = "furby.gif"
rotation = 90 #DEG
contrast = 1.0
brightness = 1.0
cropArea = (
    0, # Left (px of original image)
    0, # Top
    
    25, # Right
    32  # Bottom
) 

# imageName = "furby.gif"
# rotation = 90 #DEG
# contrast = 2.5 # 2.5
# brightness = 0.5 #0.5
# cropArea = (
#     80, # Left (px of original image)
#     55, # Top
    
#     424, # Right
#     445  # Bottom
# ) 
# Script :)
def saveDebugImage(img, fn, step):
    if os.path.exists("debug/"):
        img.save(fn
                .replace("frames/","debug/")
                .replace(".bmp", "-DBG-%s.bmp"%step)
            )

imgGIF = Image.open(imageName)
frameArray = {}

kaas = open("../obergransad/src/frames.c","w")
kaas.write("#include <stdint.h>\n")
frames = imgGIF.n_frames

# Remove old frames/debug output
for fn in glob.glob("frames/*.bmp"):
    os.remove(fn)
for fn in glob.glob("debug/*.bmp"):
    os.remove(fn)

for frameID in range(frames):
    fr = "frames/frame-%d.bmp"%frameID
    imgGIF.seek(frameID)
    imgF = imgGIF.convert('RGB')
    imgF.save(fr)
    # Crop of area that needs to be retained
    imgF = imgF.crop( cropArea )
    saveDebugImage(imgF, fr, "1-cropped")

    # Enhance colour space
    imgF_ = ImageEnhance.Brightness(imgF)
    imgF = imgF_.enhance(brightness) # Brightness factor

    imgF_ = ImageEnhance.Contrast(imgF)
    imgF = imgF_.enhance(contrast) # Contrast factor

    saveDebugImage(imgF, fr, "2-colour")
    
    # Resize to 16x16 (writes in-place)
    imgF.thumbnail((16, 16))
    saveDebugImage(imgF, fr, "3-resized")
    
    # Resize to 16x16 (writes in-place)
    imgF = imgF.rotate(rotation)
    saveDebugImage(imgF, fr, "4-rotate")

    # Convert to grayscale
    imgF = imgF.convert('L')
    saveDebugImage(imgF, fr, "5-grayscale")

    img = np.array(imgF)

    # Center image in the most complicated way known to human kind.
    width = len(img)
    widthSpace = 16-width
    widthShift = 0
    if widthSpace > 1:
        widthShift = widthSpace//2

    height = len(img[0])
    heightSpace = 16-height
    heightShift = 0
    if heightSpace > 1:
        heightShift = heightSpace//2
    
    name = str(frameID)# fr.split("frame-")[1].split(".bmp")[0]
    nameInt = frameID

    s = ""
    for y in range(16):
        s += "\t"
        for x in range(16):
            try:
                if widthShift > x or heightShift > y:
                    raise
                s += "0x%02X, "%img[x-widthShift,y-heightShift]
            except:
                s += "0x00, "
            # s += "0x%02X, "%(y*16+x)
        s += "\n"
    kaas.write("uint8_t frame_%s[256] = {\n%s};\n"%(name, s))
    frameArray[nameInt] = "frame_%s"%name

# Sort the frames by key (glob gives random list of files)
frameArray = {k: v for k, v in sorted(list(frameArray.items()))}

frameArrayC = ", ".join([v for k,v in frameArray.items()])
frameCount = len(frameArray.keys())
kaas.write("\n\nconst intptr_t frames[] = { %s };"%frameArrayC)
kaas.write("\nconst uint16_t frameCount = %d;"%frameCount)
kaas.close()

print("Saved %d frames"%frameCount)