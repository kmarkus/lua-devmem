<div align="center">

# lua-devmem

**[Install](#installing) · [Quickstart](#quickstart) · [API](#api) · [Limitations](#limitations) · [Testing](#testing) · [License](#license)**

</div>

lua-devmem is a small Lua module to read and write *device memory*, such as
memory mapped peripheral registers, via `mmap(2)` of `/dev/mem` or any other
mappable file. It is intended for *register-level debugging* and *simple
drivers* on embedded Linux.

- **bounds checked**: every access is checked against the mapped region and
  raises a Lua error instead of crashing
- **aligned, fixed width accesses**: 8, 16, 32 and 64 bit and `float`, done as
  a single `volatile` access of that width
- **any offset**: regions do not need to be page aligned
- **portable**: Lua 5.1 to 5.4 and LuaJIT, 32 and 64 bit
- **minimal**: one C file, no dependencies besides Lua

<details>
<summary>Table of contents</summary>

- [Installing](#installing)
    - [Dependencies](#dependencies)
    - [Building](#building)
- [Quickstart](#quickstart)
- [API](#api)
- [Limitations](#limitations)
- [Testing](#testing)
- [Contributing](#contributing)
- [License](#license)

</details>

Installing
----------

### Dependencies

Debian/Ubuntu, for Lua 5.3 or 5.4:

```sh
apt install cmake pkg-config liblua5.4-dev
```

For Lua 5.1, 5.2 or LuaJIT, additionally
[lua-compat-5.3](https://github.com/lunarmodules/lua-compat-5.3):

```sh
apt install libluajit-5.1-dev lua-compat53-dev
```

### Building

```sh
git clone https://github.com/kmarkus/lua-devmem.git
cd lua-devmem && mkdir build && cd build
cmake ..
make
sudo make install
```

CMake uses the first Lua found via pkg-config, in the order `lua5.4`,
`lua5.3`, `luajit`, `lua5.2`, `lua5.1`, `lua`. The module is installed to
`<prefix>/lib/lua/<version>/devmem.so`, by default
`/usr/local/lib/lua/5.4/`.

Quickstart
----------

Map 0x20 bytes at offset 0x1010 of a file, write and read back a register:

```lua
local devmem = require("devmem")

local m = devmem.new("regs.bin", 0x1010, 0x20)
print(m)
m:write_u32(0x4, 0xdeadbeef)
print(string.format("0x%x", m:read_u32(0x4)))
print(pcall(m.read_u32, m, 0x20))
print(pcall(m.read_u32, m, 0x2))
```

```
mmap regs.bin at 0x7f3c1a2b6000, off: 0x1010 (0x1000), len: 0x20 (0x1000)
0xdeadbeef
false	access out of range
false	unaligned access
```

For peripheral registers, map `/dev/mem` at the physical base address from the
SoC reference manual (needs root or `CAP_SYS_RAWIO`):

```lua
local regs = devmem.new("/dev/mem", BASE, SIZE)
print(string.format("0x%08x", regs:read_u32(0x0)))
```

Other mappable files work as well, e.g. a PCI BAR via
`/sys/bus/pci/devices/<dev>/resource0` or a UIO device `/dev/uioN`.

API
---

| function                         | description                                         |
|----------------------------------|-----------------------------------------------------|
| `devmem.new(file, off [, len])`  | map `len` bytes (default: page size) at `off`       |
| `m:read_u8/u16/u32/u64(pos)`     | read an unsigned integer at `pos`                   |
| `m:read_float(pos)`              | read a 32 bit `float` at `pos`                      |
| `m:write_u8/u16/u32/u64(pos, v)` | write `v` at `pos`                                  |
| `m:write_float(pos, v)`          | write `v` as 32 bit `float` at `pos`                |
| `m:addrof(pos)`                  | virtual address of `pos` in this process            |
| `tostring(m)`                    | file, address, offset and length of the mapping     |

- `pos` is relative to `off`, in bytes.
- `new` opens `file` with `O_RDWR | O_SYNC` and maps it `MAP_SHARED`. It raises
  an error if `off` is negative, `len` is not positive, or `open` or `mmap`
  fail.
- Accesses raise `access out of range` if `[pos, pos + width)` is not within
  `[0, len)`, and `unaligned access` if the address is not a multiple of the
  access width.
- The mapping is removed when `m` is garbage collected. There is no explicit
  close.

Limitations
-----------

- **64 bit values on Lua 5.1, LuaJIT**: Lua numbers are doubles there, so
  `read_u64` and `write_u64` lose precision above 2^53.
- **Values are truncated** to the access width: `write_u8(0, 0x1234)` writes
  `0x34`.
- **`/dev/mem` restrictions**: with `CONFIG_STRICT_DEVMEM` (default on most
  distributions), the kernel allows mapping only regions not claimed by a
  driver. `CONFIG_IO_STRICT_DEVMEM` restricts this further.

Testing
-------

The tests map a temporary regular file, so they need neither root nor
hardware:

```sh
cd build && ctest
```

To run them with another Lua version, point `LUA_CPATH` to a module built for
it:

```sh
LUA_CPATH="build/?.so" luajit test/test.lua
```

The tests cover the API and the argument and bounds checks. Device specific
behavior, such as the actual bus access width, can only be checked on the
target.

Contributing
------------

Contributions are welcome:

- follow the Linux kernel
  [coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html).
- run the tests before submitting.
- submit patches as a GitHub pull request.
- add `Signed-off-by: Random J Developer <random@developer.example.org>`
  to certify the
  [Developer's Certificate of Origin 1.1](https://developercertificate.org/).

License
-------

LGPL-2.0-or-later, see `COPYING`.
