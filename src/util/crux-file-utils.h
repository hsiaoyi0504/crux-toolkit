#ifndef CRUX_FILE_UTILS_H
#define CRUX_FILE_UTILS_H

#include <fstream>
#include <sys/stat.h>

/**
 * Open a file of the given name if it either does not exist or if we
 * have permission to overwrite.
 * \returns A file stream for the open file or NULL on failure.
 */
std::ofstream* create_file
(const char* filename, ///< create file with this name
 bool overwrite ///< replace any existing files with this name.
 );

#endif // CRUX_FILE_UTILS_H

