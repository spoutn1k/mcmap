#ifndef _FILESYSTEM_
#define _FILESYSTEM_

#include <dirent.h>
typedef DIR *DIRHANDLE;

struct myFile {
  char name[300];
  bool isdir;
  unsigned long size;
};

namespace Dir {
DIRHANDLE open(char *path, myFile &file);
bool next(DIRHANDLE handle, char *path, myFile &file);
void close(DIRHANDLE handle);
} // namespace Dir

#endif
