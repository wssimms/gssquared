#pragma once

#ifdef APPLEIIGS
#define RAM_MB 1
#define RAM_SIZE RAM_MB * 1024 * 1024
#else
#define RAM_KB 48 * 1024
#define IO_KB 4 * 1024
#define ROM_KB 12 * 1024
#define MEMORY_SIZE 64 * 1024
#endif

#define GS2_PAGE_SIZE 256
