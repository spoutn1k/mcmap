#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BILLION 1000000000L

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <output.tsv> <program to profile>\n", argv[0]);
    return 1;
  }

  FILE *statusFile, *proFile;
  char procFile[80], buffer[1024];
  long size, peak, real, max;

  struct timespec delay, begin, now;

  clock_gettime(CLOCK_REALTIME, &begin);
  pid_t pid = fork();

  if (!pid)
    execvp(argv[2], argv + 2);

  delay.tv_sec = 0;
  delay.tv_nsec = .001 * BILLION;

  snprintf(procFile, 80, "/proc/%ld/status", (long)pid);

  if (!(proFile = fopen(argv[1], "w"))) {
    fprintf(stderr, "Error opening %s: %s\n", argv[1], strerror(errno));
    return 1;
  }

  fprintf(proFile, "Time (s)\tVirtual Memory (kB)\tVirtual Peak (kB)\tReal "
                   "Memory (kB)\tReal Peak (kB)\n");

  while (!waitpid(pid, NULL, WNOHANG)) {
    clock_gettime(CLOCK_REALTIME, &now);

    if (!(statusFile = fopen(procFile, "r"))) {
      fprintf(stderr, "Error opening %s: %s\n", procFile, strerror(errno));
      return 1;
    }

    while (fscanf(statusFile, " %1023s", buffer) == 1) {
      if (strcmp(buffer, "VmRSS:") == 0)
        fscanf(statusFile, " %ld", &real);
      if (strcmp(buffer, "VmHWM:") == 0)
        fscanf(statusFile, " %ld", &max);
      if (strcmp(buffer, "VmSize:") == 0)
        fscanf(statusFile, " %ld", &size);
      if (strcmp(buffer, "VmPeak:") == 0)
        fscanf(statusFile, " %ld", &peak);
    }

    fprintf(proFile, "%lf\t%ld\t%ld\t%ld\t%ld\n",
            now.tv_sec - begin.tv_sec +
                ((double)(now.tv_nsec - begin.tv_nsec)) / BILLION,
            size, peak, real, max);

    fclose(statusFile);

    nanosleep(&delay, NULL);
  }

  fclose(proFile);
}
