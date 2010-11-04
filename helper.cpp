#include "helper.h"
#include <cstring>
#include <ctime>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef S_ISREG
#define	S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif

uint8_t clamp(int32_t val)
{
	if (val < 0) {
		return 0;
	}
	if (val > 255) {
		return 255;
	}
	return (uint8_t)val;
}

string base36(int val)
{
	if (val < 0) {
		return string("-") + base36(-val);
	}
	if (val / 36 == 0) {
		if (val < 10) {
			char x = '0' + val;
			return string(&x, 1);
		}
		char x = 'a' + (val - 10);
		return string(&x, 1);
	}
	return base36(val / 36) + base36(val % 36);
}

int base10(char *val)
{
	//printf("Turning %s into ", val);
	int res = 0;
	bool neg = false;
	if (*val == '-') {
		neg = true;
		++val;
	}
	for (;;) {
		if (*val >= '0' && *val <= '9') {
			res = res * 36 + (*val++ - '0');
			continue;
		}
		if (*val >= 'a' && *val <= 'z') {
			res = res * 36 + 10 + (*val++ - 'a');
			continue;
		}
		if (*val >= 'A' && *val <= 'Z') {
			res = res * 36 + 10 + (*val++ - 'A');
			continue;
		}
		break;
	}
	if (neg) {
		res *= -1;
	}
	//printf("%d\n", res);
	return res;
}

void printProgress(const size_t current, const size_t max)
{
	static float lastp = -10;
	static time_t lastt = 0;
	if (current == 0) { // Reset
		lastp = -10;
		lastt = 0;
	}
	time_t now = time(NULL);
	if (now > lastt || current == max) {
		float proc = (float(current) / float(max)) * 100.0f;
		if (proc > lastp + 0.99f || current == max) {
			// Keep user updated but don't spam the console
			printf("[%.2f%%]\n", proc);
			fflush(stdout);
			lastt = now;
			lastp = proc;
		}
	}
}

bool fileExists(const char *strFilename)
{
	struct stat stFileInfo;
	int ret;
	ret = stat(strFilename, &stFileInfo);
	if(ret == 0) {
		return S_ISREG(stFileInfo.st_mode);
	}
	return false;
}

bool dirExists(const char *strFilename)
{
	struct stat stFileInfo;
	int ret;
	ret = stat(strFilename, &stFileInfo);
	if(ret == 0) {
		return S_ISDIR(stFileInfo.st_mode);
	}
	return false;
}

bool isNumeric(char *str)
{
	if (str[0] == '-' && str[1] != '\0') {
		++str;
	}
	while (*str != '\0') {
		if (*str < '0' || *str > '9') {
			return false;
		}
		++str;
	}
	return true;
}

bool isAlphaWorld(char *path)
{
	// Check if this path is a valid minecraft world... in a pretty sloppy way
	char *tmp = new char[strlen(path) + 20];
	strcpy(tmp, path);
	strcat(tmp, "/level.dat");
	bool b = fileExists(tmp);
	delete[] tmp;
	return b;
}
