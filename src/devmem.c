/* SPDX-License-Identifier: GPL-2.0 */

#define DEBUG

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
	int fd;
	off_t pg_off;
	const char* file;
	struct mmap* m = NULL;

	const size_t pg_size = sysconf(_SC_PAGESIZE);
	dbg("pg_size: 0x%x", pg_size);

	m = (struct mmap*) lua_newuserdata(L, sizeof(struct mmap));
	luaL_getmetatable(L, MMAP_MT);
	lua_setmetatable(L, -2);

	file = luaL_checkstring(L, 1);
	m->len = luaL_checkinteger(L, 2);	/* user len */
	m->off = luaL_checkinteger(L, 3);	/* user offset in file */
	m->file = strdup(file);

	m->len = (m->len != 0) ? m->len : pg_size;

	fd = open(file, O_RDWR | O_SYNC);

	if(fd<0)
		luaL_error(L, "failed to open %s: %s", file, strerror(errno));

	pg_off = rounddown(m->off, pg_size);			/* offset rounded down to pg boundary */
	m->off_in_pg = m->off - pg_off;				/* offset within a page */
	m->pg_len = roundup(m->off_in_pg + m->len, pg_size);	/* length rounded up to pg boundary */

	dbg("mmap %s: length: 0x%x, offset: 0x%x (off_in_pg: 0x%x, user off: 0x%lx, user len: 0x%lx)",
	    m->file, m->pg_len, pg_off, m->off_in_pg, m->off, m->len);

	m->pg_base = mmap(0, m->pg_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pg_off);

	if (m->pg_base == MAP_FAILED)
		luaL_error(L, "mmap failed failed: %s", strerror(errno));

        close(fd);
	return 1;
}

static void check_access(lua_State *L, struct mmap *m, size_t pos, size_t len)
{
	if (pos + len > m->len)
		luaL_error(L, "access out of range");
}

static int mmap_tostr(lua_State *L)
{
	char buf[128];
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	snprintf(buf, sizeof(buf), "mmap %s at %p, off: 0x%lx (0x%lx), len: 0x%lx (0x%lx)",
		 m->file, m->pg_base, m->off, (m->off - m->off_in_pg), m->len, m->pg_len);
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
	lua_pushinteger(L, (uint64_t) __addrof(m, pos));
	return 1;
}

/* read functions */
static int read_u8(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	check_access(L,	m, pos,	sizeof(uint8_t));
	uint8_t val = *((volatile uint8_t*) __addrof(m, pos));
	lua_pushinteger(L, val);
	return 1;
}

static int read_u16(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	check_access(L,	m, pos,	sizeof(uint16_t));
	uint16_t val = *((volatile uint16_t*) __addrof(m, pos));
	lua_pushinteger(L, val);
	return 1;
}

static int read_u32(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	check_access(L,	m, pos,	sizeof(uint32_t));
	uint32_t val = *((volatile uint32_t*) __addrof(m, pos));
	lua_pushinteger(L, val);
	return 1;
}

static int read_u64(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	check_access(L,	m, pos,	sizeof(uint64_t));
	uint64_t val = *((volatile uint64_t*) __addrof(m, pos));
	lua_pushinteger(L, val);
	return 1;
}

/* write functions */
static int write_u8(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	uint8_t	val = luaL_checkinteger(L, 3);
	check_access(L,	m, pos,	sizeof(val));
	*((volatile uint8_t*) __addrof(m, pos)) = val;
	return 0;
}

static int write_u16(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	off_t pos = luaL_checkinteger(L, 2);
	uint16_t val = luaL_checkinteger(L, 3);
	check_access(L,	m, pos,	sizeof(val));
	*((volatile uint16_t*) __addrof(m, pos)) = val;
	return 0;
}

static int write_u32(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	off_t pos = luaL_checkinteger(L, 2);
	uint32_t val = luaL_checkinteger(L, 3);
	check_access(L,	m, pos,	sizeof(val));
	*((volatile uint32_t*) __addrof(m, pos)) = val;
	return 0;
}

static int write_u64(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	off_t pos = luaL_checkinteger(L, 2);
	uint64_t val = luaL_checkinteger(L, 3);
	check_access(L,	m, pos,	sizeof(val));
	*((volatile uint64_t*) __addrof(m, pos)) = val;
	return 0;
}

static const luaL_Reg mmap_m [] = {
	  { "read_u8", read_u8 },
	  { "read_u16", read_u16 },
	  { "read_u32", read_u32 },
	  { "read_u64", read_u64 },
	  { "write_u8", write_u8 },
	  { "write_u16", write_u16 },
	  { "write_u32", write_u32 },
	  { "write_u64", write_u64 },
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
