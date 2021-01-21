# --- External Imports ---
from PIL import Image
import PIL.GifImagePlugin

# --- Internal Imports ---
import pterm
from pterm.ImageConversion import loadImageToArray, imageToArray, rgbImageToASCII

# --- STL Imports ---
import sys
import os
import pathlib


if __name__ == "__main__" and len(sys.argv) > 1 :

    # Init
    filePath = pathlib.Path( sys.argv[1] )

    imageSize = None

    terminalSize = os.get_terminal_size()
    terminalSize = ( terminalSize.columns, terminalSize.lines - 1 )

    if len(sys.argv) > 2:
        imageSize = [ int(sys.argv[2]), int(sys.argv[2]) ]
        if len(sys.argv) > 3:
            imageSize[1] = int(sys.argv[3])

    # Load image
    image, imageSize = loadImageToArray( sys.argv[1], terminalSize, size=imageSize )

    # Animate
    try:
        if isinstance( image, (PIL.GifImagePlugin.GifImageFile) ):
            for frameIndex in range( image.n_frames ):

                image.seek(frameIndex)

                print( rgbImageToASCII(image.resize(imageSize).convert("RGB")) )
        else:
            print( rgbImageToASCII(image.resize(imageSize).convert("RGB")) )

    except Exception as exception:
        print( exception )
    finally:
        pass