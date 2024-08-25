/*
 * Author: Matthieu Carteron <rubisetcie@gmail.com>
 * date:   2024-08-24
 *
 * Provides functions to process the actual files.
 */

#ifndef PACKER_H_INCLUDED
#define PACKER_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

int process(const char *filename, const char *name, FILE *output, FILE *header, bool outputCXX, bool headerCXX);

#endif
