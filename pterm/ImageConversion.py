# --- External Imports ---
from PIL import Image
import numpy

# --- Internal Imports ---
from .ColorConversion import ansiColored, rgbToGrayScale

# --- STL Imports ---
import pathlib


intensityMap = " .,:;i1tfLCG08@"


def loadImageToArray( filePath: str,
                      terminalSize: list,
                      size=None ):

    filePath = pathlib.Path( filePath )

    try:
        image    = Image.open( filePath )
    except Exception as exception:
        raise RuntimeError( "Failed to load image\n" + str(exception) )

    # Check whether an image size was provided
    if size == None:
        size = list(image.size)

    # Fit size to terminal
    if terminalSize[0] < size[0]:
        size[1] = int( float(terminalSize[0]) / size[0] * size[1] / 2.0 )
        size[0] = terminalSize[0]
    if terminalSize[1] < size[1]:
        size[0] = int( float(terminalSize[1]) / size[1] * size[0] )
        size[1] = terminalSize[1]

    image = Image.open( filePath )
    image.load()

    return image, size


def imageToArray( image ):
    return numpy.array( image, dtype=numpy.uint8 )


def valueToASCII( value: int ):
    return intensityMap[int(round(
        (len(intensityMap) - 1) * value / 256.0
    ))]


def rgbImageToASCII( image ):
    message = ""
    for pixelRow in numpy.array( image, dtype=numpy.uint8 ):
        for pixel in pixelRow:
            message += ansiColored(
                valueToASCII( rgbToGrayScale(pixel) ),
                tuple(pixel)
            )
        message += "\n"
    return message[:-1]