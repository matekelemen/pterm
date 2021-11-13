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
#include <stdio.h>

#if _WIN32
    #include <windows.h>    // <-- for getting the terminal's size in windows
#else
    #include <sys/ioctl.h>  // <--
    #include <unistd.h>     // <-- for getting the terminal's size on linux
#endif



/// Get parent terminal size on linux
void getTerminalSize(Int* rows, Int* columns)
{
    #if _WIN32
        CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
        HANDLE terminal = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(terminal, &bufferInfo);
        *rows = bufferInfo.srWindow.Bottom - bufferInfo.srWindow.Top;
        *columns = bufferInfo.srWindow.Right - bufferInfo.srWindow.Left + 1;

        DWORD drawMode = 0;
        GetConsoleMode(terminal, &drawMode);
        drawMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(terminal, drawMode);
        //SetConsoleMode(ENABLE_VIRTUAL_TERMINAL_PROCESSING, 1);
    #else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        *rows = w.ws_row - 1;   // -1 for newline at the end
        *columns = w.ws_col;
    #endif
}


void printHelp()
{
    /// TODO
    puts("--help");
    puts("filename: path to image file");
    puts("[-w <width>] output width");
    puts("[-h <height>] output height");
    puts("[-b] color background instead of ASCII characters");
}


struct parameters
{
    char* fileName;
    Bool  backgroundOnly;
    Bool  isGIF;
    Int   width;
    Int   height;
};

typedef struct parameters Parameters;


void initializeParameters(Parameters* p_parameters)
{
    p_parameters->fileName       = NULL;
    p_parameters->backgroundOnly = PTERM_FALSE;
    p_parameters->isGIF          = PTERM_FALSE;
    p_parameters->width          = 0;
    p_parameters->height         = 0;
}


Int parseInteger(const char* p_string)
{
    Char* p_end = NULL;
    Int output = (Int)strtol(p_string, &p_end, 10);
    return output;
}


Bool parseArguments(int argc, const char* argv[], Parameters* p_parameters)
{
    // Argument-parameter maps
    const int NULL_FLAG = 0;

    int intFlag = NULL_FLAG;
    Int* intArguments[] = {
        NULL,
        &p_parameters->width,
        &p_parameters->height
    };

    int stringFlag = NULL_FLAG;
    Char** stringArguments[] = {
        NULL,
        &p_parameters->fileName
    };

    // Parse arguments
    for (Int i=1; i<argc; ++i)
    {
        Char token = argv[i][0];

        if (token == '\0') // empty argument
            continue;

        if (token != '-') // value argument
        {
            if (intFlag)
            {
                *intArguments[intFlag] = parseInteger(argv[i]);
                intFlag = NULL_FLAG;
                continue;
            }
            else if (stringFlag)
            {
                copyString(argv[i], stringArguments[stringFlag]);
                stringFlag = NULL_FLAG;
                continue;
            }
            else // no flag set before value => must be file name
            {
                copyString(argv[i], &p_parameters->fileName);
                continue;
            }
        }

        else // flag argument
        {
            // Check whether we're expecting a value
            if (intFlag || stringFlag)
            {
                printf("Argument error: expecting a value but got a flag: %s", argv[i]);
                return PTERM_FALSE;
            }

            token = argv[i][1];
            if (token == 'b') // flag: backgroundOnly
            {
                p_parameters->backgroundOnly = PTERM_TRUE;
                continue;
            }
            if (token == 'w') // width => expecting an integer value
            {
                intFlag = 1;
                continue;
            }
            if (token == 'h') // height => expecting an integer value
            {
                intFlag = 2;
                continue;
            }
        }

        // Unhandled
        printf("Unrecognized argument: %s\n", argv[i]);
        return PTERM_FALSE;
    } // for argc

    // Postprocess
    if (!p_parameters->fileName) // Default to empty file name => ""
    {
        p_parameters->fileName = (Char*) malloc(1 * sizeof(Char));
        *p_parameters->fileName = '\0';
    }

    if (strcmp(fileExtension(p_parameters->fileName), ".gif") == 0)
        p_parameters->isGIF = PTERM_TRUE;

    if (p_parameters->width && p_parameters->height)
    {
        printf("Width and height cannot be specified at the same time");
        return PTERM_FALSE;
    }

    return PTERM_TRUE;
}


void getFinalImageSize(Parameters* p_parameters, Int originalWidth, Int originalHeight)
{
    Int targetWidth = originalWidth, targetHeight = originalHeight;

    if (p_parameters->width || p_parameters->height)
    {
        if (p_parameters->width && !p_parameters->height)
        {
            targetWidth = p_parameters->width;
            targetHeight = p_parameters->width / (double)originalWidth * originalHeight;
        }
        else if (!p_parameters->width && p_parameters->height)
        {
            targetWidth = p_parameters->height / (double)originalHeight * originalWidth;
            targetHeight = p_parameters->height;
        }
        else
        {
            targetWidth = p_parameters->width;
            targetHeight = p_parameters->height;
        }
    }
    else // no requested size => fit to terminal size
    {
        getTerminalSize(&targetWidth, &targetHeight);

        if (!targetWidth || !targetHeight)
        {
            printf("Invalid terminal size: %ix%i\n", targetHeight, targetWidth);
            exit(PTERM_ENVIRONMENT_ERROR);
        }

        PTERM_DEBUG_PRINTF("Detected %ix%i terminal\n", targetHeight, targetWidth);
    }

    p_parameters->width = originalWidth;
    p_parameters->height = originalHeight;
    fitImageSize(&p_parameters->width, &p_parameters->height, targetWidth, targetHeight);
}



int main(int argc, char const* argv[])
{
    // Init
    Parameters parameters;
    initializeParameters(&parameters);

    if (!parseArguments(argc, argv, &parameters))
    {
        printHelp();
        return PTERM_ARGUMENT_ERROR;
    }

    // Read image (and convert to RGB if necessary)
    UInt imageWidth=0, imageHeight=0, numberOfChannels=0, numberOfSourceChannels=0, numberOfFrames=0;
    Int* delays = NULL;

    UChar* data = loadImageFile(
        parameters.fileName,
        &numberOfFrames,
        &delays,
        &imageWidth,
        &imageHeight,
        &numberOfSourceChannels,
        &numberOfChannels
    );

    free(parameters.fileName);
    parameters.fileName = NULL;

    if (numberOfChannels != 4)
    {
        PTERM_DEBUG_PRINTF("Invalid number of channels (%i)\n", numberOfChannels);
        exit(PTERM_FAIL);
    }

    // Get target image sizes
    getFinalImageSize(&parameters, imageWidth, imageHeight);
    parameters.height /= 2; // adjust for terminal cell skewness

    UChar* frame = data;
    UChar* resizedImage;
    UChar* resizedFrame;
    UInt resizedFrameSize = parameters.width * parameters.height * numberOfChannels;

    // Resize frames if necessary
    if (parameters.width!=imageWidth || parameters.height!=imageHeight)
    {
        resizedImage = (UChar*) malloc(resizedFrameSize * numberOfFrames);
        resizedFrame = resizedImage;

        if (!resizedImage)
        {
            printf("Failed to allocate memory for resized image (%ib)", resizedFrameSize * numberOfFrames);
            exit(PTERM_MEMORY_ERROR);
        }

        UInt frameSize = imageWidth*imageHeight*numberOfChannels;
        for (UInt frameIndex=0; frameIndex<numberOfFrames; ++frameIndex, frame+=frameSize, resizedFrame+=resizedFrameSize)
        {
            Int resizeOutput = resizeImage(
                frame,
                resizedFrame,
                imageWidth,
                imageHeight,
                numberOfChannels,
                parameters.width,
                parameters.height
            );

            if (resizeOutput)
                exit(resizeOutput);
        }

        free(data);
    }
    else // no resizing needed
    {
        resizedImage = data;
        resizedFrame = resizedImage;
    }

    UInt outputSize = 0;
    UChar* output = NULL;
    allocateANSITextImage(&output, &outputSize, parameters.width, parameters.height);

    if (!output)
    {
        printf("Failed to allocate output of size %i\n", outputSize);
        exit(PTERM_MEMORY_ERROR);
    }

    // Loop through frames
    setvbuf(stdout, NULL, _IONBF, 0);
    clock_t time     = clock();
    resizedFrame     = resizedImage;
    UInt* frameDelay = (UInt*)delays;

    for (UInt frameIndex=0; frameIndex<numberOfFrames; ++frameIndex, resizedFrame+=resizedFrameSize, ++frameDelay)
    {
        _textFromImageInMemory(
            resizedFrame,
            output,
            parameters.width,
            parameters.height,
            numberOfChannels,
            parameters.backgroundOnly
        );

        // Print
        double sleepTime = (*frameDelay) - difftime(clock(), time)/1000.0;
        if (0 < sleepTime)
            usleep(1000 * sleepTime);
        time = clock();

        fwrite(output, sizeof(UChar), outputSize, stdout);
        fflush(stdout);
    }

    // Clear color
    fwrite(ansiColorReset, sizeof(UChar), ansiColorResetSize, stdout);

    // Release resources
    free(delays);
    free(output);

    if (parameters.width!=imageWidth || parameters.height!=imageHeight)
        free(resizedImage);
    else
        free(data);

    return PTERM_SUCCESS;
}