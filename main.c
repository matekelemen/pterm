// ------------------------------------------------------------------------------------
// Yet another program that 'draws' on the terminal
//
// -b   : color background instead of colored ASCII characters
// ------------------------------------------------------------------------------------

// --- Internal Includes ---
#define PTERM_IMPLEMENTATION
#include "pterm.h"

// --- STL Includes ---
#include <time.h>

#if _WIN32
    #include <windows.h>    // <-- for getting the terminal's size in windows
#else
    #include <sys/ioctl.h>  // <--
    #include <unistd.h>     // <-- for getting the terminal's size on linux
#endif



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

    // Read image (and convert to RGB if necessary)
    UInt imageWidth=0, imageHeight=0, numberOfChannels=0, numberOfSourceChannels=0, numberOfFrames=0;
    Int* delays = NULL;

    UChar* data = loadImageFile( fileName,
                                 &numberOfFrames,
                                 &delays,
                                 &imageWidth,
                                 &imageHeight,
                                 &numberOfSourceChannels,
                                 &numberOfChannels );

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

    UInt outputSize = 0;
    UChar* output = NULL;
    allocateANSITextImage( &output, &outputSize, width, height );

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

        _textFromImageInMemory( resizedImage,
                                output,
                                width,
                                height,
                                numberOfChannels,
                                backgroundOnly );

        // Print
        while( difftime(clock(),time) < 1000*delays[frameIndex] ) {}
        time = clock();

        printf( "%s%s\n", output, ansiColorReset );
    }

    free(data);
    free(resizedImage);
    free(delays);
    free(pixel);
    free(output);

    return PTERM_SUCCESS;
}