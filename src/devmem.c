/* SPDX-License-Identifier: LGPL-2.0+ */

#include "devmem.h"

static int mmap_gc(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	if (m->pg_base) munmap(m->pg_base, m->pg_len);
	if (m->file) free(m->file);
	return 0;
}

/**
 * map_new: create a new memory mapping
 * @param off: offset
 */
static int mmap_new(lua_State *L)
{
	const size_t pg_size = sysconf(_SC_PAGESIZE);

	int fd;
	off_t pg_off;
	const char* file;
	struct mmap* m = NULL;
	lua_Integer user_off, user_len;

	dbg("pg_size: 0x%zx", pg_size);

	file = luaL_checkstring(L, 1);
	user_off = luaL_checkinteger(L, 2);
	user_len = luaL_optinteger(L, 3, pg_size);

	luaL_argcheck(L, user_off >= 0, 2, "offset must be >= 0");
	/* keep off_in_pg + len + pg_size within size_t */
	luaL_argcheck(L, user_len > 0 && (uint64_t)user_len <= SIZE_MAX - 2 * pg_size,
		      3, "invalid length");

	m = (struct mmap*) lua_newuserdata(L, sizeof(struct mmap));
	memset(m, 0, sizeof(*m));	/* __gc runs on all error paths below */
	luaL_getmetatable(L, MMAP_MT);
	lua_setmetatable(L, -2);

	m->len = user_len;
	m->off = user_off;
	m->file = strdup(file);
	if (!m->file)
		luaL_error(L, "out of memory");

	fd = open(file, O_RDWR | O_SYNC);

	if(fd<0)
		luaL_error(L, "failed to open %s: %s", file, strerror(errno));

	pg_off = rounddown(m->off, pg_size);			/* offset rounded down to pg boundary */
	m->off_in_pg = m->off - pg_off;				/* offset within a page */
	m->pg_len = roundup(m->off_in_pg + m->len, pg_size);	/* length rounded up to pg boundary */

	dbg("mmap %s: length: 0x%zx, offset: 0x%llx (off_in_pg: 0x%llx, user off: 0x%llx, user len: 0x%zx)",
	    m->file, m->pg_len, (unsigned long long) pg_off, (unsigned long long) m->off_in_pg,
	    (unsigned long long) m->off, m->len);

	m->pg_base = mmap(0, m->pg_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pg_off);

	if (m->pg_base == MAP_FAILED) {
		m->pg_base = NULL;
		close(fd);
		luaL_error(L, "mmap failed: %s", strerror(errno));
	}

	close(fd);
	return 1;
}

/* get position from arg, check [pos, pos + len) lies within the region
 * and is aligned to len */
static size_t check_pos(lua_State *L, struct mmap *m, int arg, size_t len)
{
	lua_Integer pos = luaL_checkinteger(L, arg);

	if (pos < 0 || (uint64_t)pos > m->len || len > m->len - (size_t)pos)
		luaL_error(L, "access out of range");
	/* unaligned device memory access traps (SIGBUS) on most archs */
	if ((uintptr_t)(m->pg_base + m->off_in_pg + pos) & (len - 1))
		luaL_error(L, "unaligned access");
	return pos;
}

static int mmap_tostr(lua_State *L)
{
	char buf[128];
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	snprintf(buf, sizeof(buf), "mmap %s at %p, off: 0x%llx (0x%llx), len: 0x%zx (0x%zx)",
		 m->file, m->pg_base, (unsigned long long) m->off,
		 (unsigned long long) (m->off - m->off_in_pg), m->len, m->pg_len);
	lua_pushstring(L, buf);
	return 1;
}

static uint8_t* __addrof(struct mmap *m, size_t pos)
{
	return m->pg_base + m->off_in_pg + pos;
}

static int addrof(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	lua_pushinteger(L, (uintptr_t) __addrof(m, pos));
	return 1;
}

/* read functions */
static int read_u8(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(uint8_t));
	uint8_t val = *((volatile uint8_t*) __addrof(m, pos));
	lua_pushinteger(L, val);
	return 1;
}

static int read_u16(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(uint16_t));
	uint16_t val = *((volatile uint16_t*) __addrof(m, pos));
	lua_pushinteger(L, val);
	return 1;
}

static int read_u32(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(uint32_t));
	uint32_t val = *((volatile uint32_t*) __addrof(m, pos));
	lua_pushinteger(L, val);
	return 1;
}

static int read_u64(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(uint64_t));
	uint64_t val = *((volatile uint64_t*) __addrof(m, pos));
	lua_pushinteger(L, val);
	return 1;
}

static int read_float(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(float));
	float val = *((volatile float*) __addrof(m, pos));
	lua_pushnumber(L, val);
	return 1;
}

/* write functions */
static int write_u8(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(uint8_t));
	uint8_t	val = luaL_checkinteger(L, 3);
	*((volatile uint8_t*) __addrof(m, pos)) = val;
	return 0;
}

static int write_u16(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(uint16_t));
	uint16_t val = luaL_checkinteger(L, 3);
	*((volatile uint16_t*) __addrof(m, pos)) = val;
	return 0;
}

static int write_u32(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(uint32_t));
	uint32_t val = luaL_checkinteger(L, 3);
	*((volatile uint32_t*) __addrof(m, pos)) = val;
	return 0;
}

static int write_u64(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(uint64_t));
	uint64_t val = luaL_checkinteger(L, 3);
	*((volatile uint64_t*) __addrof(m, pos)) = val;
	return 0;
}

static int write_float(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = check_pos(L, m, 2, sizeof(float));
	float val = luaL_checknumber(L, 3);
	*((volatile float*) __addrof(m, pos)) = val;
	return 0;
}

static const luaL_Reg mmap_m [] = {
	  { "read_u8", read_u8 },
	  { "read_u16", read_u16 },
	  { "read_u32", read_u32 },
	  { "read_u64", read_u64 },
	  { "read_float", read_float },
	  { "write_u8", write_u8 },
	  { "write_u16", write_u16 },
	  { "write_u32", write_u32 },
	  { "write_u64", write_u64 },
	  { "write_float", write_float },
	  { "addrof", addrof },
	  { "__tostring", mmap_tostr },
	  { "__gc", mmap_gc },
	  { NULL, NULL },
};

static const luaL_Reg devmem_f [] = {
	{ "new", mmap_new },
	{ NULL, NULL },
};

int luaopen_devmem(lua_State *L)
{
	luaL_newmetatable(L, MMAP_MT);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");
	luaL_setfuncs(L, mmap_m, 0);

	luaL_newlib(L, devmem_f);
	return 1;
};
