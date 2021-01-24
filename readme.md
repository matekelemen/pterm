# Overview

Yet another piece of code that draws images in the terminal. Now written in C!

Bonus support for animated GIFs.

![](https://github.com/matekelemen/pterm/blob/master/.github/animated_terminal.gif)
![](https://github.com/matekelemen/pterm/blob/master/.github/colored_background.png)

## Usage

```
pterm FILE [-b]
```

- ```FILE```: path to an RGB-convertible image file

- ```-b```: color 'background' instead of ASCII characters

Supported image formats:
JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC, PNM (see details in [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h))

## Header

If you wish to include ```pterm``` in your project:
```
#define PTERM_IMPLEMENTATION
```
before including ```pterm.h``` to create the implementation.

## Notes

Please don't judge the code, it's just an excercise to prevent me from completely forgetting how to C.

# Setup

## Linux

Clone & make. Of course you can use ```CMakeLists.txt``` as well but it's just there for future Windows support.

## Windows

Not supported yet; not until I figure out how to convince a Windows terminal to recognize ANSI colors.

## Dependencies

None (apart from a C compiler and ```make```/```CMake```). Included external sources: 
- [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)
- [stb_image_resize.h](https://github.com/nothings/stb/blob/master/stb_image_resize.h)