/*
 * Author: Matthieu Carteron <rubisetcie@gmail.com>
 * date:   2024-08-24
 *
 * Contains the entry point.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <shlwapi.h>
#else
#include <libgen.h>
#endif

#include "config.h"
#include "packer.h"
#include "format.h"

/* Version number */
#define VERSION "1.0"
#define REPOSITORY "https://github.com/RubisetCie/binary-include"

/* Global variables */
Config config;              /* Configuration */
const char *output = NULL;  /* Output file name */
const char *header = NULL;  /* Header file name */
char *symbolMacro = NULL;   /* Symbol name (macro) */
char *symbol = NULL;        /* Symbol name (general use) */

/* Check if a file name represents a C / C++ source file or a header (based on the extension) */
static void checkFiletype(const char *filename, bool *isHeader, bool *isCXX)
{
    const char *extension;

    *isHeader = false;
    *isCXX = false;

    if ((extension = strrchr(filename, '.')) == NULL)
        return;

    /* If the extension starts with 'h', it's considered as a header */
    *isHeader = (extension[1] == 'h' || extension[1] == 'H');

    if (extension[1] == '\0')
        return;

    /* If the following characters are 'x' or 'p', it's C++ */
    *isCXX = (extension[2] == 'p' || extension[2] == 'P' || extension[2] == 'x' || extension[2] == 'X');
}

/* Create the header guard from the filename */
static void getGuard(const char *filename, char *guard)
{
    size_t i = 0, j = 0;
    while (filename[i] != '\0')
    {
        if (j >= FILENAME_MAX)
            break;

        /* Replace dots with '_' */
        if (filename[i] == '.')
            guard[j] = '_';
        else
            guard[j] = toupper(filename[i]);

        i++;
        j++;
    }
}

/* Open an output file */
static void setOutput(const char *name)
{
    /* Be sure that a file name is given */
    if (!name || name[0] == '-')
    {
        fputs("Missing file name after output parameter!\n", stderr);
        return;
    }
    output = name;
}

/* Open a header file */
static void setHeader(const char *name)
{
    /* Be sure that a file name is given */
    if (!name || name[0] == '-')
    {
        fputs("Missing file name after header parameter!\n", stderr);
        return;
    }
    header = name;
}

/* Print the command-line usage */
static void usage(const char *program)
{
#ifdef _WIN32
    printf("Usage: %s [<options...>] <file1> <file2> <...>\n\n\
Options:\n\
  -h, --help           : Display command-line usage.\n\
  -o, --output <file>  : Specify the output file (can be source or header).\n\
  -d, --header <file>  : Specify a header file (won't be created otherwise).\n\
  -w, --no-warning     : Suppress the auto-generated warning comment in output.\n\
  -a, --no-allman      : Disable the Allman style of indentation and use the K&R.\n\
  -f, --decimal        : Format byte data as decimal rather than hexadecimal.\n\
  -t, --text           : Write data as a text form rather than byte per byte.\n\
  -m, --macro          : Create the size definition as a macro instead of a const.\n\
  -c, --camel-case     : Use the camel case for names instead of the snake case.\n\
  -s, --single-line    : Put all the data on a single line.\n\
  -v, --version        : Print program version.\n\n\
Examples:\n\
  %s -o foo.h bar.bin\n\
  %s -o foo.h file1 file2\n\
  %s -o foo.c -d foo.h bar.bin\n\n", program, program, program, program);
#else
    printf("Usage: %1$s [<options...>] <file1> <file2> <...>\n\n\
Options:\n\
  -h, --help           : Display command-line usage.\n\
  -o, --output <file>  : Specify the output file (can be source or header).\n\
  -d, --header <file>  : Specify a header file (won't be created otherwise).\n\
  -w, --no-warning     : Suppress the auto-generated warning comment in output.\n\
  -a, --no-allman      : Disable the Allman style of indentation and use the K&R.\n\
  -f, --decimal        : Format byte data as decimal rather than hexadecimal.\n\
  -t, --text           : Write data as a text form rather than byte per byte.\n\
  -m, --macro          : Create the size definition as a macro instead of a const.\n\
  -c, --camel-case     : Use the camel case for names instead of the snake case.\n\
  -s, --single-line    : Put all the data on a single line.\n\
  -v, --version        : Print program version.\n\n\
Examples:\n\
  %1$s -o foo.h bar.bin\n\
  %1$s -o foo.h file1 file2\n\
  %1$s -o foo.c -d foo.h bar.bin\n\n", program);
#endif
}

/* Print the program version */
static void version(const char *program)
{
    printf("%s version " VERSION "\n" REPOSITORY "\n\n", program);
}

/* Entry point function */
int main(int argc, char **argv)
{
    FILE *outputFile = NULL, *headerFile = NULL;
    const char *headerBasename = NULL;
    bool outputHeader, outputCXX, headerCXX;
    int i, files = -1, retval = 0, rv2;

    /* Display help if no command line arguments are given */
    if (argc <= 1)
    {
        usage(argv[0]);
        return 0;
    }

    /* Initialize the configuration to default state */
    config.createMacro = false;
    config.camelCase = false;
    config.singleLine = false;
    config.text = false;
    config.allman = true;
    config.warning = true;
    config.format = F_HEXADECIMAL;

    /* Parse command-line arguments */
    for (i = 1; i < argc; i++)
    {
        /* Check if it's an option or file name */
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
                /* Handle long options */
                case '-':
                    if (strcmp(argv[i], "--help") == 0)
                    {
                        usage(argv[0]);
                        return 0;
                    }
                    else if (strcmp(argv[i], "--version") == 0)
                    {
                        version(argv[0]);
                        return 0;
                    }
                    else if (strcmp(argv[i], "--output") == 0)
                        setOutput(argv[++i]);
                    else if (strcmp(argv[i], "--header") == 0)
                        setHeader(argv[++i]);
                    else if (strcmp(argv[i], "--no-allman") == 0)
                        config.allman = false;
                    else if (strcmp(argv[i], "--no-warning") == 0)
                        config.warning = false;
                    else if (strcmp(argv[i], "--single-line") == 0)
                        config.singleLine = true;
                    else if (strcmp(argv[i], "--text") == 0)
                        config.text = true;
                    else if (strcmp(argv[i], "--macro") == 0)
                        config.createMacro = true;
                    else if (strcmp(argv[i], "--camel-case") == 0)
                        config.camelCase = true;
                    else if (strcmp(argv[i], "--decimal") == 0)
                        config.format = F_DECIMAL;
                    else
                    {
                        fprintf(stderr, "Unrecognized parameter: %s\n", argv[i]);
                        return 1;
                    }
                    break;
                case 'h': usage(argv[0]);            return 0;
                case 'v': version(argv[0]);          return 0;
                case 'o': setOutput(argv[++i]);      break;
                case 'd': setHeader(argv[++i]);      break;
                case 'w': config.warning = false;    break;
                case 'a': config.allman = false;     break;
                case 't': config.text = true;        break;
                case 's': config.singleLine = true;  break;
                case 'c': config.camelCase = true;   break;
                case 'm': config.createMacro = true; break;
                case 'f': config.format = F_DECIMAL; break;
                default:
                    fprintf(stderr, "Unrecognized parameter: %s\n", argv[i]);
                    return 1;
            }
        }
        else
        {
            /* Define the start of the file list */
            files = i;
            break;
        }
    }

    /* Check if a file list has been specified */
    if (files < 0)
    {
        fputs("No files specified, at least one has to be specified!\n", stderr);
        return 2;
    }

    /* Check if the output file has been specified */
    if (!output)
    {
        fputs("No output file specified!\n", stderr);
        return 2;
    }

    /* Determine the output file type */
    checkFiletype(output, &outputHeader, &outputCXX);

    /* Determine if the output file is a header or a source file */
    if (!header)
    {
        /* If true, swap the output and the header */
        if (outputHeader)
        {
            header = output;
            output = NULL;
        }
    }

    /* Determine the header file type */
    if (header)
        checkFiletype(header, &outputHeader, &headerCXX);

#ifdef _WIN32
    headerBasename = PathFindFileName(header);
#else
    headerBasename = basename(header);
#endif

    /* Open the output file */
    if (output)
    {
        if ((outputFile = fopen(output, "w")) == NULL)
        {
            fprintf(stderr, "Failed to open the output file: %s!\n", output);
            return 3;
        }

        /* Write the warning comment */
        if (config.warning)
        {
            fputs(HEADER "\n\n", outputFile);
            if (ferror(outputFile))
            {
                fputs("Failed to write the warning comment!\n", stderr);
                return 4;
            }
        }

        /* Write the include of the header */
        if (header)
        {
            fprintf(outputFile, "#include \"%s\"\n\n", headerBasename);
            if (ferror(outputFile))
            {
                fputs("Failed to write the include!\n", stderr);
                retval = 4;
                goto RETURN;
            }
        }
    }

    /* Open the header file */
    if (header)
    {
        char guard[FILENAME_MAX] = { 0 };

        if ((headerFile = fopen(header, "w")) == NULL)
        {
            fprintf(stderr, "Failed to open the header file: %s!\n", header);
            retval = 3;
            goto RETURN;
        }

        /* Write the warning comment */
        if (config.warning)
        {
            fputs(HEADER "\n\n", headerFile);
            if (ferror(headerFile))
            {
                fputs("Failed to write the warning comment!\n", stderr);
                retval = 4;
                goto RETURN;
            }
        }

        /* Write the header guard */
        getGuard(headerBasename, guard);
#ifdef _WIN32
        fprintf(headerFile, "#ifndef %s\n#define %s\n\n", guard, guard);
#else
        fprintf(headerFile, "#ifndef %1$s\n#define %1$s\n\n", guard);
#endif
        if (ferror(headerFile))
        {
            fputs("Failed to write the header guard!\n", stderr);
            retval = 4;
            goto RETURN;
        }
    }

    /* Allocate the buffers for the symbols */
    if ((symbol = malloc(FILENAME_MAX)) == NULL)
    {
        fputs("Failed to allocate memory for the symbols!\n", stderr);
        retval = 6;
        goto RETURN;
    }
    if (config.createMacro)
    {
        if ((symbolMacro = malloc(FILENAME_MAX)) == NULL)
        {
            fputs("Failed to allocate memory for the symbols!\n", stderr);
            retval = 6;
            goto RETURN;
        }
    }

    /* Process all files */
    for (i = files; i < argc; i++)
    {
#ifdef _WIN32
        rv2 = process(argv[i], PathFindFileName(argv[i]), outputFile, headerFile, outputCXX, headerCXX);
#else
        rv2 = process(argv[i], basename(argv[i]), outputFile, headerFile, outputCXX, headerCXX);
#endif
        if (rv2 != 0 && retval == 0)
            retval = rv2;
    }

  RETURN:

    /* Close the opened files */
    if (outputFile)
        fclose(outputFile);

    if (headerFile)
    {
        /* Write the end of the header guard */
        fputs("#endif\n", headerFile);

        if (ferror(headerFile))
            fputs("Failed to write the header guard!\n", stderr);

        fclose(headerFile);
    }

    free(symbol);
    free(symbolMacro);

    return retval;
}
