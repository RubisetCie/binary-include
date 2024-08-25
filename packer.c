/*
 * Author: Matthieu Carteron <rubisetcie@gmail.com>
 * date:   2024-08-24
 *
 * Provides functions to process the actual files.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "packer.h"
#include "config.h"
#include "format.h"

/* Configuration */
extern Config config;

/* Make symbol name from file name (snake case) */
static void setupSymbolSnake(const char *name, char *symbol)
{
    size_t i = 0, j = 0;

    /* Walk through the name characters */
    while (name[i] != '\0')
    {
        if (j == FILENAME_MAX - 1)
            break;

        /* Only pick the alphanumerical character */
        if ((name[i] >= 'a' && name[i] <= 'z') || 
            (name[i] >= 'A' && name[i] <= 'Z') || 
            (name[i] >= '0' && name[i] <= '9'))
            symbol[j] = tolower(name[i]);
        else
            symbol[j] = '_';

        i++;
        j++;
    }

    /* Append the null byte */
    symbol[j] = '\0';

    /* Replace the first character with '_' if it's a digit */
    if (symbol[0] >= '0' && symbol[0] <= '9') {
        symbol[0] = '_';
    }
}

/* Make symbol name from file name (camel case) */
static void setupSymbolCamel(const char *name, char *symbol)
{
    size_t i = 0, j = 0;
    bool nextUpper = false;

    /* Walk through the name characters */
    while (name[i] != '\0')
    {
        if (j == FILENAME_MAX - 1)
            break;

        /* Only pick the alphanumerical character */
        if ((name[i] >= 'a' && name[i] <= 'z') || 
            (name[i] >= 'A' && name[i] <= 'Z') || 
            (name[i] >= '0' && name[i] <= '9'))
        {
            symbol[j++] = nextUpper ? toupper(name[i]) : tolower(name[i]);
            nextUpper = false;
        }
        /* Others are not picked up, but remember to put the next one in upper case */
        else
            nextUpper = true;

        i++;
    }

    /* Append the null byte */
    symbol[j] = '\0';

    /* Replace the first character with '_' if it's a digit */
    if (symbol[0] >= '0' && symbol[0] <= '9') {
        symbol[0] = '_';
    }
}

/* Make symbol name from file name (macro) */
static void setupSymbolMacro(const char *name, char *symbol)
{
    size_t i = 0, j = 0;

    /* Walk through the name characters */
    while (name[i] != '\0')
    {
        if (j == FILENAME_MAX - 1)
            break;

        /* Only pick the alphanumerical character */
        if ((name[i] >= 'a' && name[i] <= 'z') || 
            (name[i] >= 'A' && name[i] <= 'Z') || 
            (name[i] >= '0' && name[i] <= '9'))
            symbol[j] = toupper(name[i]);
        else
            symbol[j] = '_';

        i++;
        j++;
    }

    /* Append the null byte */
    symbol[j] = '\0';
}

/* Write all the content of the input file to the output, in numerical form */
static bool writeDataNumerical(FILE *input, FILE *output, const size_t length)
{
    int i, n, byte;
    bool first = true;
    for (i = 0, n = 0; i < length; i++)
    {
        byte = fgetc(input);
        if (byte == EOF)
            return false;

        /* Write the separator */
        if (!first)
        {
            fputs(",", output);
            if (!config.singleLine && n >= DATA_PER_LINE)
            {
                fputs("\n" DATA_INDENT, output);
                n = 0;
            }
            else
                fputs(" ", output);

            if (ferror(output))
                return false;
        }

        /* Format the byte */
        switch (config.format)
        {
            case F_DECIMAL: fprintf(output, "%d", byte);     break;
            default:        fprintf(output, "0x%02x", byte); break;
        }
        if (ferror(output))
            return false;

        first = false;
        n++;
    }
    return true;
}

/* Write all the content of the input file to the output, in text form */
static bool writeDataText(FILE *input, FILE *output, const size_t length)
{
    int i, byte;
    bool newLine = false;
    for (i = 0; i < length; i++)
    {
        byte = fgetc(input);
        if (byte == EOF)
            return false;

        /* Handle the escape sequences */
        switch (byte)
        {
            case '\n':
                fputs("\\n", output);
                newLine = true;
                break;
            case '\r':
                fputs("\\r", output);
                newLine = true;
                break;
            case '\a': fputs("\\a", output);  break;
            case '\b': fputs("\\b", output);  break;
            case '\f': fputs("\\f", output);  break;
            case '\t': fputs("\\t", output);  break;
            case '\v': fputs("\\v", output);  break;
            case '"' : fputs("\\\"", output); break;
            case '\\': fputs("\\\\", output); break;
            default  : fputc(byte, output);   break;
        }

        if (ferror(output))
            return false;

        /* Handle new lines */
        if (!config.singleLine && newLine)
        {
            fputs("\"\n" DATA_INDENT "\"", output);
            if (ferror(output))
                return false;
            newLine = false;
        }
    }
    return true;
}

/* Write all the content of the input file to the output, in text form */
static bool writeData(FILE *input, FILE *output, const size_t length)
{
    if (config.text)
        return writeDataText(input, output, length);

    return writeDataNumerical(input, output, length);
}

/* Process an input file */
int process(const char *filename, const char *name, FILE *output, FILE *header, bool outputCXX, bool headerCXX)
{
    extern char *symbol, *symbolMacro;
    long length;
    int retval = 0;
    FILE *input = NULL;

    /* If both the source and the header are specified, only implement in one file */
    const bool implementation = ((output == NULL) != (header == NULL));

    /* Write the file name comment */
    if (output)
    {
        if (outputCXX)
            fprintf(output, "// " NAME_COMMENT "\n", name);
        else
            fprintf(output, "/* " NAME_COMMENT " */\n", name);

        if (ferror(output))
        {
            fputs("Failed to write the file name comment!\n", stderr);
            return 5;
        }
    }
    if (header)
    {
        if (headerCXX)
            fprintf(header, "// " NAME_COMMENT "\n", name);
        else
            fprintf(header, "/* " NAME_COMMENT " */\n", name);

        if (ferror(header))
        {
            fputs("Failed to write the file name comment!\n", stderr);
            return 5;
        }
    }

    /* Make up a suitable symbol name */
    if (config.camelCase)
        setupSymbolCamel(name, symbol);
    else
        setupSymbolSnake(name, symbol);

    /* Make up the macro symbol name if needed */
    if (config.createMacro)
        setupSymbolMacro(name, symbolMacro);

    /* Open the input file */
    if ((input = fopen(filename, "rb")) == NULL)
    {
        fprintf(stderr, "Failed to open the input file: %s!\n", filename);
        retval = 4;
        goto RETURN;
    }

    /* Compute the length of the file */
    fseek(input, 0, SEEK_END);
    length = ftell(input);

    /* Rewind to the start of the file */
    rewind(input);

    /* Write the size definition */
    if (output && !config.createMacro)
    {
        if (config.camelCase)
            fprintf(output, "const " SIZE_TYPE " %s" SIZE_SUFFIX_CAMEL " = %d;\n", symbol, length);
        else
            fprintf(output, "const " SIZE_TYPE " %s_" SIZE_SUFFIX " = %d;\n", symbol, length);

        if (ferror(output))
        {
            fputs("Failed to write the size definition!\n", stderr);
            retval = 5;
            goto RETURN;
        }
    }
	if (header)
    {
        if (config.createMacro)
            fprintf(header, "#define %s_" SIZE_SUFFIX_MACRO " %d\n", symbolMacro, length);
        else
        {
            if (!implementation)
                fputs("extern ", header);

            if (config.camelCase)
                fprintf(header, "const " SIZE_TYPE " %s" SIZE_SUFFIX_CAMEL, symbol);
            else
                fprintf(header, "const " SIZE_TYPE " %s_" SIZE_SUFFIX, symbol);

            if (implementation)
                fprintf(header, " = %d;\n", length);
            else
                fputs(";\n", header);
        }

        if (ferror(header))
        {
            fputs("Failed to write the size definition!\n", stderr);
            retval = 5;
            goto RETURN;
        }
    }

    /* Write the content of the input */
    if (output)
    {
        /* Write the definition */
        if (config.createMacro)
            fprintf(output, "const " DATA_TYPE " %s[%s_" SIZE_SUFFIX_MACRO "] =", symbol, symbolMacro);
        else
            fprintf(output, "const " DATA_TYPE " %s[%d] =", symbol, length);

        if (!config.text)
        {
            if (config.singleLine)
                fputs(" { ", output);
            else if (!config.allman)
                fputs(" {\n" DATA_INDENT, output);
            else
                fputs("\n{\n" DATA_INDENT, output);
        }
        else
        {
            if (config.singleLine)
                fputs(" \"", output);
            else
                fputs("\n" DATA_INDENT "\"", output);
        }

        if (ferror(output))
        {
            fputs("Failed to write the definition!\n", stderr);
            retval = 5;
            goto RETURN;
        }

        /* Write the data */
        if (!writeData(input, output, length))
        {
            fputs("Failed to write the data!\n", stderr);
            retval = 5;
            goto RETURN;
        }

        /* Close the bracket */
        if (!config.text)
        {
            if (config.singleLine)
                fprintf(output, " };\n\n", symbol);
            else
                fprintf(output, "\n};\n\n", symbol);
        }
        else
            fprintf(output, "\";\n\n", symbol);

        if (ferror(output))
        {
            fputs("Failed to write the closure!\n", stderr);
            retval = 5;
            goto RETURN;
        }
    }
    if (header)
    {
        /* Write the definition */
        if (!implementation)
            fputs("extern ", header);

        if (config.createMacro)
            fprintf(header, "const " DATA_TYPE " %s[%s_" SIZE_SUFFIX_MACRO "]", symbol, symbolMacro);
        else
            fprintf(header, "const " DATA_TYPE " %s[%d]", symbol, length);

        if (implementation)
        {
            if (!config.text)
            {
                if (config.singleLine)
                    fputs(" { ", header);
                else if (!config.allman)
                    fputs(" {\n" DATA_INDENT, header);
                else
                    fputs("\n{\n" DATA_INDENT, header);
            }
            else
            {
                if (config.singleLine)
                    fputs(" \"", header);
                else
                    fputs("\n" DATA_INDENT "\"", header);
            }

            if (ferror(header))
            {
                fputs("Failed to write the definition!\n", stderr);
                retval = 5;
                goto RETURN;
            }

            /* Write the data */
            if (!writeData(input, header, length))
            {
                fputs("Failed to write the data!\n", stderr);
                retval = 5;
                goto RETURN;
            }

            /* Close the bracket */
            if (!config.text)
            {
                if (config.singleLine)
                    fprintf(header, " };\n\n", symbol);
                else
                    fprintf(header, "\n};\n\n", symbol);
            }
            else
                fprintf(output, "\";\n\n", symbol);
        }
        else
            fputs(";\n\n", header);

        if (ferror(header))
        {
            fputs("Failed to write the closure!\n", stderr);
            retval = 5;
            goto RETURN;
        }
    }

  RETURN:

    if (input)
        fclose(input);

    return retval;
}
