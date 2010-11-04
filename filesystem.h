#ifndef _FILESYSTEM_
#define _FILESYSTEM_

#if defined(_WIN32) && !defined(__GNUC__)
#	define MSVCP
#	include <windows.h>
typedef HANDLE DIRHANDLE;
#else
#	include <dirent.h>
typedef DIR *DIRHANDLE;
#endif

struct myFile {
	char name[300];
	bool isdir;
	unsigned long size;
};

namespace Dir
{
	DIRHANDLE open(char *path, myFile &file);
	bool next(DIRHANDLE handle, char *path, myFile &file);
	void close(DIRHANDLE handle);
}

#endif
