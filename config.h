/*
 * Author: Matthieu Carteron <rubisetcie@gmail.com>
 * date:   2024-08-24
 *
 * Provides the configuration structure.
 */

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include <stdbool.h>

typedef enum NumberFormat
{
    F_HEXADECIMAL,
    F_DECIMAL
} NumberFormat;

typedef struct Config
{
    NumberFormat format; /* Format of numbers if write byte per byte */
    bool createMacro;    /* Create a macro instead of a const */
    bool camelCase;      /* Use the camel case instead of snake case */
    bool allman;         /* Use the "allman" style instead of the K&R */
    bool text;           /* Put data line by line as a text */
    bool singleLine;     /* Write the data as a single line */
    bool warning;        /* Write the auto-generated warning */
} Config;

#endif
