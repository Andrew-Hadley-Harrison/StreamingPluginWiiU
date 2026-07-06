// Compat shim: old nsysnet symbol used by libutilswut/restored code.
#include <unistd.h>
extern "C" int socketclose(int fd) { return close(fd); }
