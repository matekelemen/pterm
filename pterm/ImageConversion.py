# --- External Imports ---
from PIL import Image
import numpy

# --- Internal Imports ---
from .ColorConversion import ansiColored, rgbToGrayScale

# --- STL Imports ---
import pathlib


intensityMap = " .,:;i1tfLCG08@"


def loadImageToArray( filePath: str,
                      size=None ):

    filePath = pathlib.Path( filePath )
    image    = Image.open( filePath )

    if size == None:
        size = image.size

    return numpy.asarray(
        Image.open( filePath ).resize( size ),
        dtype=numpy.uint8
    )


def valueToASCII( value: int ):
    return intensityMap[int(round(
        (len(intensityMap) - 1) * value / 256.0
    ))]


def rgbImageToASCII( image: numpy.ndarray ):
    message = ""

    for pixelRow in image:
        for pixel in pixelRow:
            message += ansiColored(
                valueToASCII( rgbToGrayScale(pixel) ),
                tuple(pixel)
            )
        message += '\n'

    return message