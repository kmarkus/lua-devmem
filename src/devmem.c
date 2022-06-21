/* SPDX-License-Identifier: GPL-2.0 */

#define DEBUG

#include "devmem.h"

static int mmap_gc(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	munmap(m->base, m->len);
	return 0;
}

/**
 * map_new: create a new memory mapping
 * @param off: offset
 */
static int mmap_new(lua_State *L)
{
	int fd;
	off_t off;
	size_t len;
	const char* file;
	struct mmap* m = NULL;

	const size_t pg_size = sysconf(_SC_PAGESIZE);

	file = luaL_checkstring(L, 1);
	off = luaL_checkinteger(L, 2);
	len = luaL_checkinteger(L, 3);

	len = (len==0) ? pg_size : roundup(len, pg_size);

	fd = open(file, O_RDWR | O_SYNC);

	m = (struct mmap*) lua_newuserdata(L, sizeof(struct mmap));

	luaL_getmetatable(L, MMAP_MT);
	lua_setmetatable(L, -2);

	m->base = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off);

	if (m->base == (void *) -1)
		luaL_error(L, "mmap failed failed: %m");

	m->off = off;
	m->len = len;

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
	snprintf(buf, sizeof(buf), "mmap at %p, off: 0x%lx, len: 0x%lx",
		 m->base, m->off, m->len);
	lua_pushstring(L, buf);
	return 1;
}

/* read functions */
static int read_u8(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	check_access(L,	m, pos,	sizeof(uint8_t));
	lua_pushinteger(L, m->base[pos]);
	return 1;
}

static int read_u16(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	check_access(L,	m, pos,	sizeof(uint16_t));
	uint16_t val = *((volatile uint16_t*) (m->base + pos));
	lua_pushinteger(L, val);
	return 1;
}

static int read_u32(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	check_access(L,	m, pos,	sizeof(uint32_t));
	uint32_t val = *((volatile uint32_t*) (m->base + pos));
	lua_pushinteger(L, val);
	return 1;
}

static int read_u64(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	size_t pos = luaL_checkinteger(L, 2);
	check_access(L,	m, pos,	sizeof(uint64_t));
	uint64_t val = *((volatile uint64_t*) (m->base + pos));
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
	*((volatile uint8_t*) (m->base + pos)) = val;
	return 0;
}

static int write_u16(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	off_t pos = luaL_checkinteger(L, 2);
	uint16_t val = luaL_checkinteger(L, 3);
	check_access(L,	m, pos,	sizeof(val));
	*((volatile uint16_t*) (m->base + pos)) = val;
	return 0;
}

static int write_u32(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	off_t pos = luaL_checkinteger(L, 2);
	uint32_t val = luaL_checkinteger(L, 3);
	check_access(L,	m, pos,	sizeof(val));
	*((volatile uint32_t*) (m->base + pos)) = val;
	return 0;
}

static int write_u64(lua_State *L)
{
	struct mmap *m  = (struct mmap*) luaL_checkudata(L, 1, MMAP_MT);
	off_t pos = luaL_checkinteger(L, 2);
	uint64_t val = luaL_checkinteger(L, 3);
	check_access(L,	m, pos,	sizeof(val));
	*((volatile uint64_t*) (m->base + pos)) = val;
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
