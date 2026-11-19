-- devmem tests against a regular file instead of /dev/mem
-- usage: LUA_CPATH=<builddir>/?.so lua test/test.lua

local devmem = require("devmem")

local pass, fail = 0, 0
local function check(name, cond, msg)
   if cond then
      pass = pass + 1
   else
      fail = fail + 1
      print("FAIL " .. name .. (msg and (": " .. tostring(msg)) or ""))
   end
end

local function expect_err(name, pat, f, ...)
   local ok, err = pcall(f, ...)
   check(name, not ok and tostring(err):find(pat), err)
end

local file = os.tmpname()
local fh = assert(io.open(file, "wb"))
fh:write(string.rep("\0", 8192))
fh:close()

-- unaligned offset: region starts within a page
local m = devmem.new(file, 16, 16)

m:write_u8(0, 0xab)
check("rw u8", m:read_u8(0) == 0xab)
m:write_u16(2, 0xbeef)
check("rw u16", m:read_u16(2) == 0xbeef)
m:write_u32(4, 0xdeadbeef)
check("rw u32", m:read_u32(4) == 0xdeadbeef)
m:write_u64(8, 0x0102030405060000)
check("rw u64", m:read_u64(8) == 0x0102030405060000)
m:write_float(12, 1.5)
check("rw float", m:read_float(12) == 1.5)
check("tostring", tostring(m):find("^mmap "))
check("addrof", m:addrof(4) - m:addrof(0) == 4)

-- region boundaries
check("last byte ok", pcall(m.read_u8, m, 15))
check("last u32 ok", pcall(m.read_u32, m, 12))
expect_err("u8 past end", "out of range", m.read_u8, m, 16)
expect_err("u32 crossing end", "out of range", m.read_u32, m, 13)
expect_err("neg pos read", "out of range", m.read_u32, m, -2)
expect_err("neg pos write", "out of range", m.write_u32, m, -2, 1)
expect_err("huge pos", "out of range", m.read_u8, m, math.maxinteger or 2^52)

-- writes stayed inside the region
local m0 = devmem.new(file, 0, 16)
check("no write before region", m0:read_u64(0) == 0 and m0:read_u64(8) == 0)

-- argument validation
expect_err("len 0", "invalid length", devmem.new, file, 0, 0)
expect_err("len -1", "invalid length", devmem.new, file, 16, -1)
expect_err("neg offset", "offset", devmem.new, file, -4096, 16)
expect_err("open fail", "failed to open", devmem.new, "/nonexistent/devmem", 0)
check("default len", pcall(devmem.new, file, 0))

-- gc of failed and good mappings must not crash
m, m0 = nil, nil
collectgarbage(); collectgarbage()

os.remove(file)
print(string.format("%d passed, %d failed", pass, fail))
os.exit(fail == 0 and 0 or 1)
