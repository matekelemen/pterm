// --- External Includes ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

// --- STL Includes ---
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


// Define PTERM_IMPLEMENTATION for a single source
// if you wish to include pterm in your project:
// #define PTERM_IMPLEMENTATION


// Compile options set in CMake
//#define PTERM_DEBUG     // <-- print debug output

// Type aliases
typedef int             Int;
typedef unsigned int    UInt;
typedef char            Char;
typedef unsigned char   UChar;
typedef UInt            Bool;


// ------------------------------------------------------------------------------------
// INTERFACE
// ------------------------------------------------------------------------------------

/// Load an image and get its relevant properties
/**
 * Load a file into memory and decode it with stb_image. The returned unsigned char
 * array contains all frames of the input file with 8 bits per channel in the
 * following layout: [frame,row,column,channel]
 *
 * The original frames may have 1..4 channels, but are ultimately converted to RGBA, so the
 * output always has 4 channels.
 *
 * Memory is allocated for reading the file, and for converting it to image frames
 * (the former is deallocated internally). An additional chunk of memory is allocated
 * for frame delays as well, even if the image file consists of a single frame
 * (meaning that the frame delays will always have to be deallocated too).
 *
 * Supported image formats: JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC, PNM
 * (see details at https://github.com/nothings/stb/blob/master/stb_image.h)
 *
 * @param fileName path to an image file
 * @param numberOfFrames number of frames in the source image (only animated GIFs have more than 1)
 * @param frameDelaysMS delays between consecutive frames in milliseconds (always stores at least 1 value)
 * @param width width of the source image
 * @param height height of the source image
 * @param numberOfOriginalChannels number of channels in the source image
 * @param numberOfOutputChannels number of channels in the loaded image (always set to 4 for RGBA)
 */
UChar* loadImageFile(const Char* fileName,
                      UInt* numberOfFrames,
                      Int** frameDelaysMS,
                      UInt* width,
                      UInt* height,
                      UInt* numberOfOriginalChannels,
                      UInt* numberOfOutputChannels);

/// Convert image to text
/**
 * Convert an 8-bit-per-channel image into ANSI-colored text. The function allocates memory
 * for the output internally. An additional internal allocation happens if the user requests
 * a resized image, which is then deallocated before returning.
 *
 * @param image image with 8 bits per channel and [row,column,channel] layout (origin in the top left corner)
 * @param width image width (number of columns)
 * @param height image height (number of rows)
 * @param numberOfChannels number of components per pixel
 * @param targetWidth width of the desired output 'image' (without ANSI sequences and new lines).
 * @param targetHeight height of the desired output 'image'
 * @param backgroundOnly fill text with colored background instead of ASCII characters
 */
UChar* textFromImageInMemory(const UChar* image,
                              UInt width,
                              UInt height,
                              UInt numberOfChannels,
                              UInt targetWidth,
                              UInt targetHeight,
                              Bool backgroundOnly);

/// Allocate memory for an ANSI colored text 'image'
/**
 * @param textImage pointer to unsigned char array
 * @param size allocated memory in bytes
 * @param width text image width
 * @param height text image height
 */
void allocateANSITextImage(UChar** textImage,
                            UInt* size,
                            UInt width,
                            UInt height);

/// Convert image to text
/**
 * Convert an 8-bit-per channel image into ANSI-colored text. No allocations/deallocations
 * are performed internally, so the destination array must have enough space for the output.
 * This function is called from @ref{textFromImageInMemory}, and can be used to make the
 * conversion of animated GIFs more efficient.
 * If you want to call this function yourself, allocate with @ref{allocateANSITextImage} first.
 *
 * @note the image is expected to have 4 channels for RGBA
 *
 * @param image image with 8 bits per channel and [row,column,channel] layout (origin in the top left corner)
 * @param destination output array (must have proper size)
 * @param width input image width
 * @param height input image height
 * @param numberOfChannels number of pixel components in the input image
 * @param backgoundOnly fill text with colored background instead of ASCII characters
 */
void _textFromImageInMemory(const UChar* image,
                             UChar* destination,
                             UInt width,
                             UInt height,
                             UInt numberOfChannels,
                             Bool backgroundOnly);


// ------------------------------------------------------------------------------------
// PREPROCESSOR
// ------------------------------------------------------------------------------------

// Exit codes
#define PTERM_SUCCESS           0
#define PTERM_FAIL              1
#define PTERM_ARGUMENT_ERROR    2
#define PTERM_INPUT_ERROR       3
#define PTERM_MEMORY_ERROR      4
#define PTERM_ENVIRONMENT_ERROR 5
#define PTERM_IO_ERROR          6

// Other
#define PTERM_TRUE              1
#define PTERM_FALSE             0

// Macro switch for debug printing
#ifdef PTERM_DEBUG
    #define PTERM_DEBUG_PRINTF(...)   \
    printf(__VA_ARGS__)
    #define PTERM_INLINE
#else
    #define PTERM_DEBUG_PRINTF(...)
    #define PTERM_INLINE inline
#endif


// ------------------------------------------------------------------------------------
// INLINE IMPLEMENTATION
// ------------------------------------------------------------------------------------

void ansiColorCode(UChar red, UChar green, UChar blue, UChar* ansi, Bool backgroundOnly);

void ansiReset(UChar* ansi);

void ansiPadding(UChar* ansi);

void getPixel(const UChar* image, UChar* pixel, Int rowIndex, Int columnIndex, Int imageWidth, Int imageHeight, Int numberOfChannels);


/// --- ANSI STUFF --- ///

// Example: \e[38;2;255;255;255m
const Int ansiColorSize = 19;

// \e[0m
const Int ansiColorResetSize = 4;
const UChar ansiColorReset[] = "\e[0m";


/// Note: ansi must be allocated and at least [ansiColorSize] long
PTERM_INLINE void ansiColorCode(UChar red, UChar green, UChar blue, UChar* ansi, Bool backgroundOnly)
{
    *ansi++ = '\e';
    *ansi++ = '[';

    if (backgroundOnly)
        *ansi++ = '4';  // <-- colored background
    else
        *ansi++ = '3';  // <-- colored character

    *ansi++ = '8';
    *ansi++ = ';';
    *ansi++ = '2';
    *ansi++ = ';';

    *ansi++ = (red / 100)+'0';
    *ansi++ = ((red%100) / 10)+'0';
    *ansi++ = (red%10)+'0';
    *ansi++ = ';';

    *ansi++ = (green / 100)+'0';
    *ansi++ = ((green%100) / 10)+'0';
    *ansi++ = (blue%10)+'0';
    *ansi++ = ';';

    *ansi++ = (blue / 100)+'0';
    *ansi++ = ((blue%100) / 10)+'0';
    *ansi++ = (blue%10)+'0';
    *ansi = 'm';
}


/// Note: ansi must be allocated and at least [ansiColorResetSize] long
PTERM_INLINE void ansiReset(UChar* ansi)
{
    for (const UChar* c=ansiColorReset; c!=ansiColorReset+ansiColorResetSize; ++c, ++ansi )
        *ansi = *c;
}


/**
 * Fill up the array with characters that won't be displayed.
 * @note ansi must be allocated and at least [ansiColorSize] long.
 */
PTERM_INLINE void ansiPadding(UChar* ansi)
{
    *ansi++ = '\e';
    *ansi++ = '[';
    *ansi++ = '1';
    *ansi++ = '1';
    *ansi++ =  ';';
    *ansi++ =  '3';
    *ansi++ =  '1';
    *ansi++ =  ';';
    *ansi++ =  '4';
    *ansi++ =  '9';
    *ansi++ = 'm';
    *ansi++ = ' ';
    *ansi++ = '\b';
    *ansi++ = ' ';
    *ansi++ = '\b';
    *ansi++ = ' ';
    *ansi++ = '\b';
    *ansi++ = ' ';
    *ansi = '\b';
}


PTERM_INLINE void getPixel(const UChar* image, UChar* pixel, Int rowIndex, Int columnIndex, Int imageWidth, Int imageHeight, Int numberOfChannels)
{
    const UChar* pixelBegin = image + (rowIndex * imageWidth + columnIndex) * numberOfChannels;
    const UChar* pixelEnd   = pixelBegin + numberOfChannels;

    for (const UChar* it_component=pixelBegin; it_component != pixelEnd; ++it_component, ++pixel)
        *pixel = *it_component;
}


// ------------------------------------------------------------------------------------
// IMPLEMENTATION
// ------------------------------------------------------------------------------------

#ifdef PTERM_IMPLEMENTATION


/// --- Utility --- ///

void copyString(const Char* source, Char** destination)
{
    UInt sourceSize = 0;
    while(PTERM_TRUE) { if (source[sourceSize++] == '\0') break; }

    *destination = (Char*) malloc(sourceSize);
    strcpy(*destination, source);
}


UChar* loadFile(const Char* fileName, UInt* size)
{
    UChar* data = NULL;
    *size = 0;

    if (fileName)
    {
        errno = 0;
        FILE* file = fopen(fileName, "rb");

        if (file)
        {
            // Get file size
            fseek(file, 0, SEEK_END);
            Int fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            if (fileSize)
            {
                // Allocate memory
                data = (UChar*) malloc(fileSize * sizeof(UChar));
                *size = (UInt) fread(data, sizeof(UChar), fileSize, file);
            }
            else
                PTERM_DEBUG_PRINTF("%s\n", "WARNING: empty file");
        } // if file
        else
        {
            printf("Failed to open %s (%s)\n", fileName, strerror(errno));
            exit(PTERM_INPUT_ERROR);
        } // if !file
    }
    else
    {
        puts("No file name provided!");
        exit(PTERM_INPUT_ERROR);
    }

    return data;
}


const Char* fileExtension(const Char* fileName)
{
    const Char* extension = fileName;
    const Char* tmp = fileName;

    while (*tmp != '\0')
    {
        if (*tmp == '.')
            extension = tmp;
        ++tmp;
    }

    if (extension == fileName) // no extension
        extension = tmp;

    return extension;
}


/// --- IMAGE UTILITIES --- ///

// Linear map: grayscale value -> ASCII character
const UInt asciiIntensityTableSize = 15;
const UChar asciiIntensityTable[] = { ' ', '.', ',', ':', ';', 'i', 'l', 't', 'f', 'L', 'C', 'G', '0', '8', '@' };

UChar getASCIIFromGrayScale(UChar value)
{
    UInt key = ((asciiIntensityTableSize-1) * value) / 256;
    return asciiIntensityTable[key];
}


UChar getASCIIFromRGB(UChar red, UChar green, UChar blue)
{
    return getASCIIFromGrayScale(0.2989*red + 0.587*green + 0.114*blue);
}


void fitImageSize(Int* width, Int* height, Int targetWidth, Int targetHeight)
{
    if (targetWidth < *width) {
        *height = (targetWidth*(*height)) / (*width);
        *width = targetWidth;
    }
    if (targetHeight < *height) {
        *width = (targetHeight*(*width)) / (*height);
        *height = targetHeight;
    }
}


Int resizeImage(const UChar* image, UChar* newImage, Int width, Int height, Int numberOfChannels, Int newWidth, Int newHeight)
{
    PTERM_DEBUG_PRINTF("Resizing image to %ix%i\n", newWidth, newHeight);

    Int resizeResult = stbir_resize_uint8(
        image, width, height, 0,
        newImage, newWidth, newHeight, 0, numberOfChannels
    );

    if (!resizeResult)
    {
        PTERM_DEBUG_PRINTF("Image resizing errored out (error %i)\n", resizeResult);
        return PTERM_FAIL;
    }

    return PTERM_SUCCESS;
}


Int convertImage(UChar** data,
                 UInt size,
                 const Char* extension,
                 UInt* numberOfFrames,
                 Int** frameDelaysMS,
                 UInt* width,
                 UInt* height,
                 UInt* numberOfOriginalChannels,
                 UInt* numberOfOutputChannels)
{
    // Init
    *numberOfFrames           = 0;
    *frameDelaysMS            = NULL;
    *width                    = 0;
    *height                   = 0;
    *numberOfOriginalChannels = 0;

    // Check file extension
    Bool isGIF = PTERM_FALSE;
    if (strcmp(extension, ".gif") == 0)
        isGIF = PTERM_TRUE;

    // Set output channels
    *numberOfOutputChannels = 4; // <-- Convert to RGBA no matter how many channels the input has

    // Decode frames
    if (isGIF == PTERM_TRUE)
    {
        UChar* tmp = stbi_load_gif_from_memory(
            *data,
            size,
            frameDelaysMS,
            width,
            height,
            numberOfFrames,
            numberOfOriginalChannels,
            *numberOfOutputChannels
        );
        free(*data);
        *data = tmp;
    } // isGIF
    else
    {
        UChar* tmp = stbi_load_from_memory(
            *data,
            size,
            width,
            height,
            numberOfOriginalChannels,
            *numberOfOutputChannels
        );
        free(*data);
        *data = tmp;

        *numberOfFrames = 1;
        *frameDelaysMS = (Int*) malloc(sizeof(Int));

        if (!*frameDelaysMS)
        {
            PTERM_DEBUG_PRINTF("Failed to allocate memory for frame delays (%ib)\n", 1);
            exit(PTERM_MEMORY_ERROR);
        }

        (*frameDelaysMS)[0] = 0;
    } // !isGIF

    if (!*data)
    {
        PTERM_DEBUG_PRINTF("Failed to load image from %s\n", fileName);
        free(*frameDelaysMS);
        return PTERM_INPUT_ERROR;
    }

    if (!*width || !*height || !*numberOfOriginalChannels || !*numberOfFrames)
    {
        PTERM_DEBUG_PRINTF("Empty image %ix%ix%i with %i frames\n", *width, *height, *numberOfOriginalChannels, *numberOfFrames);
        free(*data);
        free(*frameDelaysMS);
        return PTERM_INPUT_ERROR;
    }

    if (*numberOfOriginalChannels != *numberOfOutputChannels)
        PTERM_DEBUG_PRINTF("Warning: source image has %i channels. Converted it to RGBA!\n", *numberOfOriginalChannels);

    return PTERM_SUCCESS;
}


UChar* loadImageFile(const Char* fileName,
                      UInt* numberOfFrames,
                      Int** frameDelaysMS,
                      UInt* width,
                      UInt* height,
                      UInt* numberOfOriginalChannels,
                      UInt* numberOfOutputChannels)
{
    // Load file
    UInt fileSize = 0;
    UChar* data = loadFile(fileName, &fileSize);

    if (!data)
    {
        PTERM_DEBUG_PRINTF("Failed to load %s\n", fileName);
        return NULL;
    }

    if (!fileSize)
        PTERM_DEBUG_PRINTF("%s is empty!\n", fileName);

    Int conversionOutput = convertImage(
        &data,
        fileSize,
        fileExtension(fileName),
        numberOfFrames,
        frameDelaysMS,
        width,
        height,
        numberOfOriginalChannels,
        numberOfOutputChannels
    );

    if (conversionOutput != PTERM_SUCCESS)
    {
        puts("Failed to convert image");
        exit(conversionOutput);
    }

    return data;
}


void allocateANSITextImage(UChar** textImage,
                           UInt* size,
                           UInt width,
                           UInt height)
{
    *textImage = NULL;
    *size      = 0;

    *size =
        width * height                      // <-- number of 'pixels'
        * (ansiColorSize + 1)               // <-- payload (ANSI color and a character)
        + height * (ansiColorResetSize + 1) // <-- new line and color reset at line ends
        + 1;                                // <-- \0
    *textImage = (UChar*) malloc(*size);

    if (!*textImage)
    {
        PTERM_DEBUG_PRINTF("Failed to allocate memory for text image (%ib)\n", *size);
        *size = 0;
    }
}


UChar* textFromImageInMemory(const UChar* image,
                              UInt width,
                              UInt height,
                              UInt numberOfChannels,
                              UInt targetWidth,
                              UInt targetHeight,
                              Bool backgroundOnly)
{
    // Resize image if necessary
    const UChar* frame = image;
    if (width != targetWidth || height != targetHeight)
    {
        UChar* tmp = (UChar*) malloc(targetWidth * targetHeight * numberOfChannels);
        if (!frame)
        {
            PTERM_DEBUG_PRINTF("Failed to allocate memory for resized image (%ib)\n", targetWidth*targetHeight*numberOfChannels);
            exit(PTERM_MEMORY_ERROR);
        }

        Int resizeOutput = resizeImage(image,
                                        tmp,
                                        width,
                                        height,
                                        numberOfChannels,
                                        targetWidth,
                                        targetHeight);
        if (resizeOutput)
            exit(resizeOutput);

        frame = tmp;
    } // if resize

    // Allocate output
    UInt outputSize = 0;
    UChar* output = NULL;
    allocateANSITextImage(&output, &outputSize, targetWidth, targetHeight);

    if (!output)
        return NULL;

    // Assemble output
    _textFromImageInMemory(frame,
                            output,
                            targetWidth,
                            targetHeight,
                            numberOfChannels,
                            backgroundOnly);

    // Deallocate if the image was resized
    if (width != targetWidth || height != targetHeight)
        free((UChar*)frame);

    return output;
}


void _textFromImageInMemory(const UChar* image,
                             UChar* destination,
                             UInt width,
                             UInt height,
                             UInt numberOfChannels,
                             Bool backgroundOnly)
{
    // Init
    UChar pixel[4];

    if (!pixel)
    {
        PTERM_DEBUG_PRINTF("Failed to allocate pixel of size %i\n", numberOfChannels);
        exit(PTERM_MEMORY_ERROR);
    }

    // Assemble output
    UChar* cursor = destination;

    for (UInt rowIndex=0; rowIndex<height; ++rowIndex)
    {
        for (UInt columnIndex=0; columnIndex<width; ++columnIndex)
        {
            getPixel(image,
                      pixel,
                      rowIndex,
                      columnIndex,
                      width,
                      height,
                      numberOfChannels);

            if (0 < pixel[3])
            {
                ansiColorCode(pixel[0], pixel[1], pixel[2], cursor, backgroundOnly);

                cursor += ansiColorSize;

                if (backgroundOnly)
                    *cursor++ = ' ';
                else
                    *cursor++ = getASCIIFromRGB(pixel[0], pixel[1], pixel[2]);
            }
            else
            {
                ansiPadding(cursor);
                cursor += ansiColorSize;
                *cursor++ = ' ';
            }
        } // for columnIndex

        ansiReset(cursor);
        cursor += ansiColorResetSize;

        *cursor++ = '\n';

    } // for rowIndex

    *cursor = '\0';
}

#endif // PTERM_IMPLEMENTATION