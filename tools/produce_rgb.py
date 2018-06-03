

#!/usr/bin/python
from PIL import Image
import sys


rawData = open("voc_test_input.bin" ,'rb').read()
#rawData = open("out_32_937178.bin" ,'rb').read()

imgSize = (448,448)
img = Image.frombytes('RGB', imgSize, rawData)
img.save("24.bmp")
img.show()


rawData = open("out_32_346820.bin" ,'rb').read()


img32 = Image.frombytes('RGBA', imgSize, rawData)

img32.convert('RGB').save('24_out.bmp')

img32.save("32.bmp")
img32.show()