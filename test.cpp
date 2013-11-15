#include <cstdio>
#include <stdint.h>

int main()
{
	const int width = 33033;
	const int height = 24007;
	uint32_t datasize1 = ((uint32_t(width) * 3u + 3u) & ~uint32_t(3)) * uint32_t(height);
	int datasize2 = (int(width * 3 + 3) & ~int(3)) * height;
	printf("Size: %u or %u\n", datasize2, datasize1);
	return 0;
}