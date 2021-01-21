# --- External Imports ---
from colors import color as _coloredText
import numpy


def rgbToGrayScale( rgb: tuple ):
    return 0.2989 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]


def grayScaleImage( pixelArray: numpy.ndarray ):
    return numpy.dot(
        pixelArray[...,:3],
        [0.2989, 0.587, 0.114]
    )


def ansiColored( string: str,
                 rgb: tuple ):
    return _coloredText( string, rgb )