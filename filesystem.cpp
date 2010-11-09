#include "filesystem.h"

#ifdef MSVCP
#include <direct.h>
// See http://en.wikipedia.org/wiki/Stdint.h#External_links
#include <stdint.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#endif

#include <cstdarg>
#include <cstdlib>
#include <cstring>

#define CCEND ((char*)0)

#ifdef MSVCP
// For UTF8 conversion
#define LOWER_6_BIT(u)    ((u) & 0x003f)
#define LOWER_7_BIT(u)    ((u) & 0x007f)
#define BIT7(a)           ((a) & 0x80)
#define BIT6(a)           ((a) & 0x40)
#define BIT5(a)           ((a) & 0x20)
#define BIT54(a)          ((a) & 0x30)
#define BIT543(a)         ((a) & 0x38)
#define BIT2(a)           ((a) & 0x04)
#define BIT1(a)           ((a) & 0x02)
#define BIT0(a)           ((a) & 0x01)

// Return: -1 = at least one nullpointer, 1 = success, 0 = outbuffer too small
static int Utf8ToWideChar(char *pUTF8, size_t cchSrc, wchar_t *pWCHAR, size_t cchDest, bool *isvalid)
{
	if (!pUTF8 || !pWCHAR) {
		return -1;   // Valid pointers?
	}
	int nTB = 0; // Number of bytes left for current char
	wchar_t *pDestEnd = pWCHAR + cchDest;
	char *pSrcEnd = pUTF8 + cchSrc;
	char UTF8;
	if (isvalid != NULL) {
		*isvalid = true;
	}
	while ((pUTF8 < pSrcEnd) && (pWCHAR < pDestEnd)) {
		if (BIT7(*pUTF8) == 0) { // normal ASCII
			if (nTB) { // last mulibyte char not complete, insert '?'
				nTB = 0;
				*pWCHAR++ = 63;
				if (isvalid != NULL) {
					*isvalid = false;
				}
			} else { // just convert
				*pWCHAR++ = (wchar_t)*pUTF8++;
			}
		} else if (BIT6(*pUTF8) == 0) { // utf8 sequence byte (not first)
			if (nTB != 0) {
				*pWCHAR <<= 6;
				*pWCHAR |= LOWER_6_BIT(*pUTF8);
				if (--nTB == 0) {
					++pWCHAR;
				}
			} else { // No more trailing bytes expected, insert '?'
				*pWCHAR++ = 63;
				if (isvalid != NULL) {
					*isvalid = false;
				}
			}
			++pUTF8;
		} else { // No ASCII and no trailing byte
			if (nTB) { // but last char was multibyte and not complete yet, insert '?'
				nTB = 0;
				*pWCHAR++ = 63;
				if (isvalid != NULL) {
					*isvalid = false;
				}
			} else { // OK, check how many bytes
				UTF8 = *pUTF8;
				while (BIT7(UTF8) != 0) { // count number of bytes for this char
					UTF8 <<= 1;
					nTB++;
				}
				if (nTB > 4) { // too long, utf8 specs allow only up to 4 bytes per char
					nTB = 0;
					*pWCHAR++ = 63; // time for a '?'
					if (isvalid != NULL) {
						*isvalid = false;
					}
				} else { // just shift bits back and assign
					*pWCHAR = UTF8 >> nTB--;
				}
			}
			++pUTF8;
		}
	}
	if (nTB != 0 && isvalid != NULL) {
		*isvalid = false;
	}
	if (pWCHAR < pDestEnd) {
		*pWCHAR = 0;
		return 1;
	}
	*(pWCHAR-1) = 0;
	return 0;
}

// Return: -1 = at least one nullpointer, 1 = success, 0 = outbuffer too small
template <class T>
int WideCharToUtf8(T pWCHAR, size_t cchSrc, uint8_t *pUTF8, size_t cchDest)
{
	if (!pUTF8 || !pWCHAR) {
		return -1;   // Valid pointers?
	}
	uint8_t *pDestEnd = pUTF8 + cchDest;
	T pSrcEnd = pWCHAR + cchSrc;
	uint8_t UTF8[4];

	while ((pWCHAR < pSrcEnd) && (pUTF8 < pDestEnd)) {
		if (LOWER_7_BIT(*pWCHAR) == *pWCHAR) { // normal ASCII
			*pUTF8++ = (uint8_t)*pWCHAR++;
		} else { // utf8 encode!
			int i;
			for (i = 0; i < 4; ++i) {
				UTF8[i] = LOWER_6_BIT(*pWCHAR) | 0x80;
				*pWCHAR >>= 6;
				if (*pWCHAR == 0) {
					break;
				}
			}
			bool exp = false;
			if (i == 1 && BIT5(UTF8[1])) {
				exp = true;
			} else if (i == 2 && BIT54(UTF8[2])) {
				exp = true;
			} else if (i == 3 && BIT543(UTF8[3])) {
				exp = true;
			}

			if (exp) {
				++i;
				UTF8[i] = (0xff) << (7 - i);
			} else if (i == 1) {
				UTF8[1] |= 0xc0;
			} else if (i == 2) {
				UTF8[2] |= 0xe0;
			} else if (i == 3) {
				UTF8[3] |= 0xf0;
			}
			do {
				*pUTF8++ = UTF8[i];
				if (pUTF8 >= pDestEnd) {
					*(pUTF8-1) = '\0';
					return 0;
				}
			} while (i-- > 0);
			++pWCHAR;
		}
	}
	if (pUTF8 >= pDestEnd) {
		*(pUTF8-1) = '\0';
		return 0;
	}
	*pUTF8 = 0;
	return 1;
}
#endif

static size_t concat(char *buffer, const size_t len, char *source, ...)
{
	if (len <= 0) {
		return 0;
	}
	va_list parg;
	size_t count = 0;

	va_start(parg, source);
	if (source != CCEND) do {
			while (*source != 0) {
				*buffer++ = *source++;
				if (++count >= len) {
					*(buffer-1) = 0;
					va_end(parg);       /* End variable argument process      */
					return count;
				}
			}
		} while((source = va_arg(parg, char *)) != CCEND);
	va_end(parg);                /* End variable argument process      */
	*buffer = 0;
	return count;
}

namespace Dir
{
	DIRHANDLE open(char *path, myFile &file)
	{
		if (path == NULL) {
			return NULL;
		}
#ifdef MSVCP
		char buffer[1000];
		wchar_t wbuffer[1000];
		_WIN32_FIND_DATAW ffd;
		concat(buffer, 1000, path, "/*", CCEND);
		bool b;
		Utf8ToWideChar(buffer, strlen(buffer), wbuffer, 1000, &b);
		HANDLE h = FindFirstFileW(wbuffer, &ffd);
		if (h == INVALID_HANDLE_VALUE) {
			return NULL;
		}
		WideCharToUtf8(ffd.cFileName, wcslen(ffd.cFileName), (uint8_t *)file.name, sizeof(file.name));
		file.isdir = ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
		file.size = ffd.nFileSizeLow;
#else
		DIR *h = opendir(path);
		if (h == NULL) {
			return NULL;
		}
		dirent *dirp = readdir(h);
		if (dirp == NULL) {
			closedir(h);
			return NULL;
		}
		char buffer[1000];
		concat(buffer, 1000, path, "/", dirp->d_name, CCEND);
		struct stat stDirInfo;
		if (stat(buffer, &stDirInfo) < 0) {
			closedir(h);
			return NULL;
		}
		strncpy(file.name, dirp->d_name, sizeof(file.name));
		file.isdir = S_ISDIR(stDirInfo.st_mode);
		file.size = stDirInfo.st_size;
#endif
		return h;
	}

	bool next(DIRHANDLE handle, char *path, myFile &file)
	{
#ifdef MSVCP
		_WIN32_FIND_DATAW ffd;
		bool ret = FindNextFileW(handle, &ffd) == TRUE;
		if (!ret) {
			return false;
		}
		WideCharToUtf8(ffd.cFileName, wcslen(ffd.cFileName), (uint8_t *)file.name, sizeof(file.name));
		file.isdir = ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
		file.size = ffd.nFileSizeLow;
#else
		dirent *dirp = readdir(handle);
		if (dirp == NULL) {
			return false;
		}
		char buffer[1000];
		concat(buffer, 1000, path, "/", dirp->d_name, CCEND);
		struct stat stDirInfo;
		if (stat(buffer, &stDirInfo) < 0) {
			return false;
		}
		strncpy(file.name, dirp->d_name, sizeof(file.name));
		file.isdir = S_ISDIR(stDirInfo.st_mode);
		file.size = stDirInfo.st_size;
#endif
		return true;
	}

	void close(DIRHANDLE handle)
	{
#ifdef MSVCP
		FindClose(handle);
#else
		closedir(handle);
#endif
	}

}
