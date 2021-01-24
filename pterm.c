// ------------------------------------------------------------------------------------
// Yet another program that 'draws' on the terminal
//
// -b   : color background instead of colored ASCII characters
// ------------------------------------------------------------------------------------



// --- External Includes ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

// --- STL Includes ---
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#if _WIN32
    #include <windows.h>    // <-- for getting the terminal's size in windows
#else
    #include <sys/ioctl.h>  // <--
    #include <unistd.h>     // <-- for getting the terminal's size on linux
#endif



// Compile options set in CMake
//#define PTERM_DEBUG     // <-- print debug output



// Type aliases
typedef int             Int;
typedef unsigned int    UInt;
typedef char            Char;
typedef unsigned char   UChar;
typedef float           Float;
typedef double          Double;
typedef UInt            Bool;


// Linear map: grayscale value -> ASCII character
const UInt asciiIntensityTableSize = 15;
const UChar asciiIntensityTable[]   = { ' ', '.', ',', ':', ';', 'i', 'l', 't', 'f', 'L', 'C', 'G', '0', '8', '@' };


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
    #define PTERM_DEBUG_PRINTF( ... )   \
    printf( __VA_ARGS__ )
#else
    #define PTERM_DEBUG_PRINTF( ... )
#endif


// ------------------------------------------------------------------------------------
// ENVIRONMENT
// ------------------------------------------------------------------------------------

/// Get parent terminal size on linux
void getTerminalSize( Int* rows, Int* columns )
{
    #if _WIN32
        CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
        HANDLE terminal = GetStdHandle( STD_OUTPUT_HANDLE );
        GetConsoleScreenBufferInfo( terminal, &bufferInfo );
        *rows = bufferInfo.srWindow.Bottom - bufferInfo.srWindow.Top;
        *columns = bufferInfo.srWindow.Right - bufferInfo.srWindow.Left + 1;
        
        DWORD drawMode = 0;
        GetConsoleMode( terminal, &drawMode );
        drawMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode( terminal, drawMode );
        //SetConsoleMode( ENABLE_VIRTUAL_TERMINAL_PROCESSING, 1 );
    #else
        struct winsize w;
        ioctl( STDOUT_FILENO, TIOCGWINSZ, &w );
        *rows = w.ws_row - 1;   // -1 for newline at the end
        *columns = w.ws_col;
    #endif
}


// ------------------------------------------------------------------------------------
// UTILITY
// ------------------------------------------------------------------------------------

/// Safe and slow string copy
void copyString( const Char* source, Char** destination )
{
    UInt sourceSize = 0;
    while( PTERM_TRUE ) { if ( source[sourceSize++] == '\0' ) break; }
    
    *destination = (Char*) malloc( sourceSize );
    strcpy( *destination, source );
}


/// Load file into memory
UChar* loadFile( const Char* fileName, UInt* size )
{
    UChar* data = NULL;
    *size = 0;

    if ( fileName )
    {
        errno = 0;
        FILE* file = fopen( fileName, "rb" );

        if ( file )
        {
            // Get file size
            fseek( file, 0, SEEK_END );
            Int fileSize = ftell(file);
            fseek( file, 0, SEEK_SET );

            if ( fileSize )
            {
                // Allocate memory
                data = (UChar*) malloc( fileSize * sizeof(UChar) );
                *size = (UInt) fread( data, sizeof(UChar), fileSize, file );
            }
            else
                PTERM_DEBUG_PRINTF( "%s\n", "WARNING: empty file" );
        } // if file
        else
        {
            printf( "Failed to open %s (%s)\n", fileName, strerror(errno) );
            exit( PTERM_INPUT_ERROR );
        } // if !file
    }
    else
    {
        puts( "No file name provided!" );
        exit( PTERM_INPUT_ERROR );
    }

    return data;
}


/// Get pointer to the file extension in a null-terminated string
const Char* fileExtension( const Char* fileName )
{
    const Char* extension = fileName;
    const Char* tmp = fileName;

    while ( *tmp != '\0' )
    {
        if ( *tmp == '.' )
            extension = tmp;
        ++tmp;
    }

    if ( extension == fileName ) // no extension
        extension = tmp;

    return extension;
}


void printHelp()
{
    /// TODO
    puts( "--help" );
    puts( "filename: path to image file" );
    puts( "[-h] print help" );
    puts( "[-b] color background instead of ASCII characters" );
}


Bool parseArguments( int argc, const char* argv[], Char** fileName, Bool* backgroundOnly, Bool* isGIF )
{
    // Check number of arguments
    if ( argc < 2 )
    {
        printf( "%s\n", "Expecting at least a path to an image file but got no arguments" );
        return PTERM_ARGUMENT_ERROR;
    }

    for ( Int i=1; i<argc; ++i )
    {
        Char token = argv[i][0];

        if ( token == '\0' ) // empty argument
            continue;

        if ( token != '-' ) // not a flag, not an argument-value pair -> must be a file name
        {
            copyString( argv[i], fileName );
            continue;
        }

        else // flag or argument-value pair
        {
            token = argv[i][1];

            if ( token == 'b' ) // flag: backgroundOnly
            {
                *backgroundOnly = PTERM_TRUE;
                continue;
            }
            if ( token == 'h' )
                return PTERM_FALSE;
            if ( token == '-' ) // argument-value pair
            {}
        }

        // Unhandled
        printf( "Unrecognized argument: %s\n", argv[i] );
        return PTERM_FALSE;
    } // for argc

    if ( strcmp(fileExtension(*fileName), ".gif") == 0 )
        *isGIF = PTERM_TRUE;

    return PTERM_TRUE;
}


// ------------------------------------------------------------------------------------
// ANSI STUFF
// ------------------------------------------------------------------------------------

// \e[38;2;255;255;255m     <-- \e(fixed?) [(fixed?) 38(fixed) ;2(fixed) ;R;G;B m(fixed)
const Int ansiColorSize = 19;

// \e[0m
const Int ansiColorResetSize = 4;
const UChar ansiColorReset[] = "\e[0m";


/// Note: ansi must be at least ansiColorSize long
void initializeANSIColorCode( UChar* ansi, Bool backgroundOnly )
{
    ansi[0] = '\e';
    ansi[1] = '[';

    if ( backgroundOnly )
        ansi[2] = '4';  // <-- colored background
    else
        ansi[2] = '3';  // <-- colored character

    ansi[3] = '8';
    ansi[4] = ';';
    ansi[5] = '2';
    ansi[6] = ';';

    ansi[10] = ';';
    ansi[14] = ';';
    ansi[18] = 'm';
}


/// Note: ansi must be at least ansiColorSize long
void updateANSIColorCode( UChar red, UChar green, UChar blue, UChar* ansi )
{
    ansi[7] = (red / 100)+'0';
    ansi[8] = ((red%100) / 10)+'0';
    ansi[9] = (red%10)+'0';
    
    ansi[11] = (green / 100)+'0';
    ansi[12] = ((green%100) / 10)+'0';
    ansi[13] = (blue%10)+'0';

    ansi[15] = (blue / 100)+'0';
    ansi[16] = ((blue%100) / 10)+'0';
    ansi[17] = (blue%10)+'0';
}


/// Note: ansi must be at least ansiColorResetSize long
void insertANSIReset( UChar* ansi )
{
    for ( const UChar* c=ansiColorReset; c!=ansiColorReset+ansiColorResetSize; ++c, ++ansi  )
        *ansi = *c;
}


// ------------------------------------------------------------------------------------
// CONVENIENCE
// ------------------------------------------------------------------------------------

/// Read pixel from loaded image
void getPixel( const UChar* image, UChar* pixel, Int rowIndex, Int columnIndex, Int imageWidth, Int imageHeight, Int numberOfChannels )
{
    const UChar* pixelBegin = image + (rowIndex * imageWidth + columnIndex) * numberOfChannels;
    const UChar* pixelEnd   = pixelBegin + numberOfChannels;

    for ( const UChar* it_component=pixelBegin; it_component != pixelEnd; ++it_component, ++pixel )
        *pixel = *it_component;
}


UChar getASCIIFromGrayScale( UChar value )
{
    UInt key = ((asciiIntensityTableSize-1) * value) / 256;
    return asciiIntensityTable[key];
}


UChar getASCIIFromRGB( UChar red, UChar green, UChar blue )
{
    return getASCIIFromGrayScale( 0.2989*red + 0.587*green + 0.114*blue );
}


/// Resize source image sizes such that they fit the target terminal size while keeping their aspect ratio
void fitImageSize( Int* width, Int* height, Int targetWidth, Int targetHeight )
{
    if ( targetWidth < *width )
    {
        *height = (targetWidth*(*height)) / (*width) / 2;
        *width = targetWidth;
    }
    if ( targetHeight < *height )
    {
        *width = (targetHeight*(*width)) / (*height);
        *height = targetHeight;
    }
}


Int resizeImage( UChar* image, UChar* newImage, Int width, Int height, Int numberOfChannels, Int newWidth, Int newHeight )
{
    PTERM_DEBUG_PRINTF( "Resizing image to %ix%i\n", newWidth, newHeight );

    Int resizeResult = stbir_resize_uint8(
        image, width, height, 0,
        newImage, newWidth, newHeight, 0, numberOfChannels
    );

    if ( !*newImage || !resizeResult )
    {
        printf( "Resizing image to %ix%i failed\n", newWidth, newHeight );
        return PTERM_FAIL;
    }

    return PTERM_SUCCESS;
}


// ------------------------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------------------------

int main( int argc, char const* argv[] )
{
    // Init
    Char* fileName = NULL;
    Bool backgroundOnly = PTERM_FALSE;
    Bool isGIF = PTERM_FALSE;

    if ( !parseArguments( argc, argv, &fileName, &backgroundOnly, &isGIF ) )
    {
        printHelp();
        return PTERM_ARGUMENT_ERROR;
    }

    // Load file
    UInt fileSize = 0;
    UChar* data = loadFile( fileName, &fileSize );

    if ( !data || !fileSize )
    {
        printf( "Failed to load %s\n", fileName );
        return PTERM_INPUT_ERROR;
    }

    PTERM_DEBUG_PRINTF( "Successfully loaded %s of size %i\n", fileName, fileSize );

    // Read image (and convert to RGB if necessary)
    Int imageWidth=0, imageHeight=0, numberOfChannels=0, numberOfFrames=0;
    Int* delays = NULL;

    // Get frame (for handling animated gifs)
    if ( isGIF )
    {
        UChar* tmp = stbi_load_gif_from_memory( data,
                                                fileSize,
                                                &delays,
                                                &imageWidth,
                                                &imageHeight,
                                                &numberOfFrames,
                                                &numberOfChannels,
                                                3 );
        free(data);
        data = tmp;
    } // isGIF
    else
    {
        UChar* tmp = stbi_load_from_memory( data, fileSize, &imageWidth, &imageHeight, &numberOfChannels, 3 );
        free(data);
        data = tmp;
        
        numberOfFrames = 1;
        delays = (Int*) malloc( sizeof(Int) );
        delays[0] = 0;
    } // isGIF else

    if ( !data )
    {
        printf( "Failed to load image from %s\n", fileName );
        return PTERM_INPUT_ERROR;
    }

    if ( !imageHeight || !imageWidth || !numberOfChannels || !numberOfFrames )
    {
        printf( "Empty image %ix%ix%i with %i frames\n", imageWidth, imageHeight, numberOfChannels, numberOfFrames );
        return PTERM_INPUT_ERROR;
    }

    if ( numberOfChannels != 3 )
    {
        PTERM_DEBUG_PRINTF( "Warning: source image has %i channels. Converted it to RGB!\n", numberOfChannels );
        numberOfChannels = 3;
    }

    PTERM_DEBUG_PRINTF( "Successfully loaded %i %ix%ix%i image(s)\n", numberOfFrames, imageWidth, imageHeight, numberOfChannels );

    free(fileName);
    fileName = NULL;

    // Load environment
    Int terminalRows=0, terminalColumns=0;
    getTerminalSize( &terminalRows, &terminalColumns );

    if ( !terminalRows || !terminalColumns )
    {
        printf( "Invalid terminal size: %ix%i\n", terminalColumns, terminalRows );
        return PTERM_ENVIRONMENT_ERROR;
    }

    PTERM_DEBUG_PRINTF( "Detected %ix%i terminal\n", terminalColumns, terminalRows );

    // Allocate and initialize
    UChar* frame = data;
    UChar* pixel = (UChar*) malloc( numberOfChannels );

    if ( !pixel )
    {
        printf( "Failed to allocate pixel of size %i\n", numberOfChannels );
        return PTERM_MEMORY_ERROR;
    }

    Int width=imageWidth, height=imageHeight;
    fitImageSize( &width, &height, terminalColumns, terminalRows );

    UChar* resizedImage = (UChar*) malloc( width * height * numberOfChannels );

    if ( !resizedImage )
    {
        printf( "Failed to allocate memory for resized image (%ib)", width*height*numberOfChannels );
        exit( PTERM_MEMORY_ERROR );
    }

    Int outputSize =
        width * height                      // <-- number of pixels
        * (ansiColorSize+1)                 // <-- payload
        + width * (1+ansiColorResetSize);   // <-- newlines with color resets
    UChar* output = (UChar*) malloc( outputSize );

    if ( !output )
    {
        printf( "Failed to allocate output of size %i\n", outputSize );
        exit( PTERM_MEMORY_ERROR );
    }

    // Loop through frames
    clock_t time = clock();

    for ( UInt frameIndex=0; frameIndex<numberOfFrames; ++frameIndex, frame+=imageWidth*imageHeight*numberOfChannels )
    {

        // Resize the image
        Int resizeOutput = resizeImage( frame,
                                        resizedImage,
                                        imageWidth,
                                        imageHeight,
                                        numberOfChannels,
                                        width,
                                        height );

        if ( resizeOutput )
            exit( resizeOutput );

        PTERM_DEBUG_PRINTF( "Allocated %i bytes for the output\n", outputSize );

        // Assemble output
        UChar* currentOutput = output;

        for ( UInt rowIndex=0; rowIndex<height; ++rowIndex )
        {
            for ( UInt columnIndex=0; columnIndex<width; ++columnIndex )
            {
                getPixel( resizedImage, pixel, rowIndex, columnIndex, width, height, numberOfChannels );
                initializeANSIColorCode( currentOutput, backgroundOnly );
                updateANSIColorCode( pixel[0], pixel[1], pixel[2], currentOutput );

                currentOutput += ansiColorSize;

                if ( backgroundOnly )
                    *currentOutput++ = ' ';
                else
                    *currentOutput++ = getASCIIFromRGB( pixel[0], pixel[1], pixel[2] );

            } // for column

            insertANSIReset( currentOutput );
            currentOutput += ansiColorResetSize;
            
            *currentOutput++ = '\n';

        } // for row

        *currentOutput = '\0';

        // Print
        while( difftime(clock(),time) < 1000*delays[frameIndex] ) {}
        time = clock();

        printf( "%s%s", output, ansiColorReset );
    }

    free(data);
    free(resizedImage);
    free(delays);
    free(pixel);
    free(output);

    return PTERM_SUCCESS;
}