#ifndef __DEVMEM_H
#define __DEVMEM_H

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <fcntl.h>

#include "math.h"

#include "lua.h"
#include "lauxlib.h"

#define MMAP_MT		"mmap_mt"

#if LUA_VERSION_NUM == 501
# include "lua5.1/compat-5.3.h"
#elif LUA_VERSION_NUM == 502
# include "lua5.2/compat-5.3.h"
#endif

#ifdef DEBUG
# define dbg(fmt, args...) ( fprintf(stderr, "%s: ", __FUNCTION__), \
			     fprintf(stderr, fmt, ##args),	    \
			     fprintf(stderr, "\n") )
#else
# define dbg(fmt, args...)  do {} while (0)
#endif

#define rounddown(x, y) (((x) / (y)) * (y))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * struct mmap
 * @pg_base: pointer to first page that contains the region
 * @off_in_pg: offset from pg_base to start of region
 * @pg_len: len rounded up to page boundaries
 * @off: user supplied base offset into file
 * @len: user supplied desired length [B] of region
 * @file: filename for pretty printing
 */
struct mmap {
	uint8_t *pg_base;
	off_t off_in_pg;
	size_t pg_len;

	off_t off;
	size_t len;
	char *file;
};

#endif /* __DEVMEM_H */
