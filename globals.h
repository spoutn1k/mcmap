#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <cstdlib>
#include <stdint.h>

#define UNDEFINED 0x7FFFFFFF
#define MAX_MARKERS 200

extern int g_Noise;

// If output is to be split up (for google maps etc) this contains the path to
// output to, NULL otherwise
extern char *g_TilePath;

#endif
