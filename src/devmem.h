#ifndef __DEVMEM_H
#define __DEVMEM_H

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
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

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * struct mmap
 * @base: pointer to start address of interest (i.e. page_base + offset)
 * @off: offset
 * @len: length in bytes
 */
struct mmap {
	uint8_t *base;
	off_t off;
	size_t len;
};

#endif /* __DEVMEM_H */
