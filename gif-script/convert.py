from PIL import Image, ImageEnhance, ImageChops
import glob
import numpy as np
import os

# HEY THERE! There are some tunables!
framerate = 2 # FPS
displaytime = 300 # time in seconds to display this gif.., use -1 for permanent display
imageName = "heart.webp"
rotation = 90 #DEG
contrast = 2 #1.0 = 100%
brightness = 2.5 #1.0 = 100%
cropArea = (
    0, # Left (px of original image)
    0, # Top
    
    200, # Right
    200  # Bottom
) 

# Alpha key (so you can remove filled background colours)
alphaKey = (255,255,255) # R,G,B (use GIMP to probe)
alphaTolerance = 150 # Margin band for alphaKey to be used, set to -1 to disable alpha key altogether

# Script :)
def saveDebugImage(img, fn, step):
    if os.path.exists("debug/"):
        img.save(fn
                .replace("frames/","debug/")
                .replace(".bmp", "-DBG-%s.bmp"%step)
            )

imgGIF = Image.open("gifs/%s"%imageName)
frameArray = {}

imageFn = imageName.split(".")[0]
kaas = open("../obergransad/src/frames_%s.c"%imageFn,"w")
kaas.write("#include <stdint.h>\n")
kaas.write("#include \"GifDescriptor.h\"\n")
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
    saveDebugImage(imgF, fr, "0-cropped")

    imgA = np.array(imgF)
    for x in range(len(imgA)):
        for y in range(len(imgA[0])):
            if abs(int(imgA[x,y][0]) - alphaKey[0]) <= alphaTolerance and abs(int(imgA[x,y][1]) - alphaKey[1]) <= alphaTolerance and abs(int(imgA[x,y][2]) - alphaKey[2]) <= alphaTolerance:
                imgA[x,y] = 0
    imgF = Image.fromarray(imgA)
    saveDebugImage(imgF, fr, "1-alphakeyed")

    # Enhance colour space
    imgF_ = ImageEnhance.Brightness(imgF)
    imgF = imgF_.enhance(brightness) # Brightness factor

    imgF_ = ImageEnhance.Contrast(imgF)
    imgF = imgF_.enhance(contrast) # Contrast factor

    saveDebugImage(imgF, fr, "2-colour")
    
    # Resize to 16x16 (writes in-place)
    imgF = imgF.rotate(rotation)
    saveDebugImage(imgF, fr, "3-rotate")

    # Convert to grayscale
    imgF = imgF.convert('L')
    saveDebugImage(imgF, fr, "4-grayscale")
    
    # Resize to 16x16 (writes in-place)
    imgF.thumbnail((16, 16))
    saveDebugImage(imgF, fr, "5-resized")
    
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
    kaas.write("static const uint8_t frame_%s[256] = {\n%s};\n"%(name, s))
    frameArray[nameInt] = "frame_%s"%name

# Sort the frames by key (glob gives random list of files)
frameArray = {k: v for k, v in sorted(list(frameArray.items()))}

frameArrayC = ", ".join(["\n\t(intptr_t) (%s)"%v for k,v in frameArray.items()])
frameCount = len(frameArray.keys())
kaas.write("\n\nstatic const intptr_t frames[] = {%s\n};"%frameArrayC)
kaas.write("\nconst GifDescriptor frames_%s = {\n\t.DisplayTime = %.3f,\n\t.FrameRate = %.3f,\n\t.FrameCount = %d,\n\t.FrameList = (intptr_t) &frames\n};\n"%(imageFn, displaytime, framerate, frameCount))
kaas.close()

print("Saved %d frames"%frameCount)