import sys
from PIL import Image

im = Image.open(sys.argv[1])
im = im.convert("RGBA")
buf = im.tobytes()

with open(sys.argv[2], 'wb') as f:
    f.write(buf)
