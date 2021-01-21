# --- Internal Imports ---
import pterm

# --- STL Imports ---
import sys

if __name__ == "__main__" and len(sys.argv) > 1 :

    imageSize = [70, 35]

    if len(sys.argv) > 2:
        imageSize[0] = int(sys.argv[2])
        if len(sys.argv) > 3:
            imageSize[1] = int(sys.argv[3])

    image     = pterm.ImageConversion.loadImageToArray( sys.argv[1], imageSize )
    print( pterm.ImageConversion.rgbImageToASCII(image) )