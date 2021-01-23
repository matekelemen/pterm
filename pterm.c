// --- External Includes ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --- STL Includes ---
#include <stdio.h>

#include <sys/ioctl.h>  // <--
#include <unistd.h>     // <-- for getting the terminal's size on linux




#define PTERM_DEBUG




// Type aliases
typedef int             Int;
typedef unsigned int    UInt;
typedef char            Char;
typedef unsigned char   UChar;
typedef float           Float;
typedef double          Double;


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

// Get parent terminal size on linux
void getTerminalSize( Int* width, Int* height )
{
    struct winsize w;
    ioctl( STDOUT_FILENO, TIOCGWINSZ, &w );
    *width = w.ws_row;
    *height = w.ws_col;
}


// ------------------------------------------------------------------------------------
// ANSI STUFF
// ------------------------------------------------------------------------------------

// \e[38;2;255;255;255m     <-- \e(fixed?) [(fixed?) 38(fixed) ;2(fixed) ;R;G;B m(fixed)
const Int ansiColorSize = 19;

// \e[0m
const Int ansiColorResetSize = 4;
const Char ansiColorReset[] = "\e[0m";


/// Note: ansi must be at least ansiColorMaxSize long
void initializeANSIColorCode( UChar* ansi )
{
    ansi[0] = '\e';     ansi[1] = '[';
    ansi[2] = '3';      ansi[3] = '8';      ansi[4] = ';';
    ansi[5] = '2';      ansi[6] = ';';

    ansi[10] = ';';
    ansi[14] = ';';

    ansi[18] = 'm';
}


/// Note: ansi must be at least ansiColorMaxSize long
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


// ------------------------------------------------------------------------------------
// CONVENIENCE
// ------------------------------------------------------------------------------------

// Read pixel from loaded image
void getPixel( const UChar* image, UChar* pixel, Int rowIndex, Int columnIndex, Int numberOfRows, Int numberOfColumns, Int numberOfChannels )
{
    const Char* pixelBegin = image + rowIndex * numberOfColumns*numberOfChannels + columnIndex;
    const Char* pixelEnd   = pixelBegin + numberOfChannels;

    for ( const Char* it_component=pixelBegin; it_component != pixelEnd; ++it_component, ++pixel )
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


// ------------------------------------------------------------------------------------
// MAIN
// ------------------------------------------------------------------------------------

int main( int argc, char const* argv[] )
{
    // Check number of arguments
    if ( argc != 2 )
    {
        printf( "Expecting a path to an image file but got %i arguments instead\n", argc-1 );
        return PTERM_ARGUMENT_ERROR;
    }

    // Load image
    const Char* fileName = argv[1];
    Int numberOfRows=0, numberOfColumns=0, numberOfChannels=0;
    UChar* image = stbi_load( fileName, &numberOfRows, &numberOfColumns, &numberOfChannels, 0 );

    if ( !image )
    {
        printf( "Failed to load %s\n", fileName );
        return PTERM_INPUT_ERROR;
    }

    if ( !numberOfRows || !numberOfColumns || !numberOfChannels )
    {
        printf( "Empty image with %i rows, %i columns, and %i channels\n", numberOfRows, numberOfColumns, numberOfChannels );
        return PTERM_INPUT_ERROR;
    }

    if ( numberOfChannels != 3 )
    {
        printf( "Unsupported number of channels (%i). RGB only!", numberOfChannels );
        return PTERM_INPUT_ERROR;
    }

    PTERM_DEBUG_PRINTF( "Successfully loaded image with %i rows, %i columns, and %i channels\n", numberOfRows, numberOfColumns, numberOfChannels );

    // Load environment
    Int terminalWidth=0, terminalHeight=0;
    getTerminalSize( &terminalWidth, &terminalHeight );

    if ( !terminalWidth || !terminalHeight )
    {
        printf( "Invalid terminal size: %ix%i", terminalWidth, terminalHeight );
        return PTERM_ENVIRONMENT_ERROR;
    }

    PTERM_DEBUG_PRINTF( "Detected %ix%i terminal\n", terminalWidth, terminalHeight );

    // Allocate and initialize
    Char character = ' ';

    UChar* pixel = (UChar*) malloc( numberOfChannels + 1 );
    pixel[numberOfChannels] = '\0';

    if ( !pixel )
    {
        printf( "Failed to allocate pixel of size %i\n", numberOfChannels + 1 );
        return PTERM_MEMORY_ERROR;
    }

    Int outputSize = numberOfRows * numberOfColumns * (ansiColorSize+1) + numberOfColumns;
    UChar* output = (UChar*) malloc( outputSize );

    if ( !pixel || !output )
    {
        printf( "Failed to allocate output of size %i\n", outputSize );
        return PTERM_MEMORY_ERROR;
    }

    PTERM_DEBUG_PRINTF( "Allocated %i bytes for the output\n", outputSize );

    // Assemble output
    UChar* currentOutput = output;

    for ( UInt rowIndex=0; rowIndex<numberOfRows; ++rowIndex )
    {
        for ( UInt columnIndex=0; columnIndex<numberOfColumns; ++columnIndex )
        {
            getPixel( image, pixel, rowIndex, columnIndex, numberOfRows, numberOfColumns, numberOfChannels );
            initializeANSIColorCode( currentOutput );
            updateANSIColorCode( pixel[0], pixel[1], pixel[2], currentOutput );

            currentOutput += ansiColorSize;

            *(currentOutput++) = getASCIIFromRGB( pixel[0], pixel[1], pixel[2] );

        } // for column

        *(currentOutput++) = '\n';

    } // for row

    *currentOutput = '\0';

    // Print
    puts( output );
    printf( "%s\n", ansiColorReset );

    return PTERM_SUCCESS;
}