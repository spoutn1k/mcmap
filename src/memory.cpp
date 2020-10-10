#include "memory.h"

auto begin = std::chrono::high_resolution_clock::now();
FILE *statusFile, *proFile;

void profileSetup() {
  proFile = fopen("./memProfile.tsv", "w");
  fprintf(proFile, "Time\tMemory\tPeak\n");
}

void getMem() {
  statusFile = fopen("/proc/self/status", "r");
  auto end = std::chrono::high_resolution_clock::now();
  char vmSize[1000], vmPeak[20];
  fseek(statusFile, 0, SEEK_SET);

  for (int i = 0; i < 16; i++)
    fgets(vmSize, 1000, statusFile);

  fscanf(statusFile, "%*s %s %*s", vmPeak);
  fscanf(statusFile, "%*s %s", vmSize);

  fprintf(proFile, "%lf\t%s\t%s\n",
          std::chrono::duration<double, std::milli>(end - begin).count(),
          vmSize, vmPeak);

  fclose(statusFile);
}

void profileTeardown() { fclose(proFile); }
