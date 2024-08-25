cmake_minimum_required(VERSION 3.10)

# Project name
project(binary-include VERSION 1.0)

# Set the build flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi")

# Set the format options
set(FMT_HEADER "/*\\n * The following content has been auto-generated.\\n * Do not modify it manually.\\n */" CACHE STRING "The header warning written in every output file.")
set(FMT_NAME_COMMENT "File: %s" CACHE STRING "The comment written naming asset entry.")
set(FMT_SIZE_SUFFIX "size" CACHE STRING "The suffix for every size constant.")
set(FMT_SIZE_TYPE "long" CACHE STRING "The data type for every size constant.")
set(FMT_DATA_TYPE "unsigned char" CACHE STRING "The data type for the data symbol.")
set(FMT_DATA_INDENT "    " CACHE STRING "The indentation string (four spaces).")
set(FMT_DATA_PER_LINE 12 CACHE STRING "The number of byte per line for the data.")

# Compute the lower / upper case versions of the size suffix
string(TOLOWER "${FMT_SIZE_SUFFIX}" FMT_SIZE_SUFFIX_LOWER)
string(TOUPPER "${FMT_SIZE_SUFFIX}" FMT_SIZE_SUFFIX_UPPER)

# Compute the camel case version of the size suffix
string(SUBSTRING "${FMT_SIZE_SUFFIX_LOWER}" 0 1 FMT_SIZE_SUFFIX_LOWER_FIRST)
string(TOUPPER "${FMT_SIZE_SUFFIX_LOWER_FIRST}" FMT_SIZE_SUFFIX_LOWER_FIRST)
string(REGEX REPLACE "^.(.*)" "${FMT_SIZE_SUFFIX_LOWER_FIRST}\\1" FMT_SIZE_SUFFIX_CAMEL "${FMT_SIZE_SUFFIX_LOWER}")

# Configure the format file
configure_file("format.h.in" "format.h")

# Binclude executable
add_executable(binclude main.c packer.c)

# Include build directory
target_include_directories(binclude PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

# Link some windows libraries
if (WIN32)
    target_link_libraries(binclude PRIVATE shlwapi)
endif()
