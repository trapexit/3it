# BitStream

A portable, header-only big-endian bit stream library with C89 and C++ APIs.

Reads and writes arbitrary-width bit fields packed MSB-first into byte arrays.
The bit cursor and buffer size are always 64-bit.  The `32` / `64` suffix on
each header controls the maximum field width and the integer type returned by
reads: `32` headers read/write up to 32 bits and return `unsigned int`; `64`
headers read/write up to 64 bits and return `unsigned long long`.

All headers are self-contained and require no build system -- drop the file(s)
you need into your project and include them.

---

## Contents

- [Design goals](#design-goals)
- [Picking the right header](#picking-the-right-header)
- [C API](#c-api)
  - [Fixed-buffer combined (BitStreamT)](#fixed-buffer-combined-bitstreamt)
  - [Dynamic combined (BitStreamDyn)](#dynamic-combined-bitstreamdyn)
  - [Read-only (BitStreamReader)](#read-only-bitstreamreader)
  - [Write-only (BitStreamWriter)](#write-only-bitstreamwriter)
  - [Alias headers](#c-alias-headers)
  - [Function reference](#function-reference)
  - [Fixed-width functions](#fixed-width-functions)
- [C++ API](#c-api-1)
  - [BitStreamT template](#bitstreamt-template)
  - [Type aliases](#type-aliases)
- [Implementation notes](#implementation-notes)
- [File index](#file-index)

---

## Design goals

- **Portable** -- C89-compatible C headers; C++11 and C++17 variants for the template API.
- **Header-only** -- all functions are `static`; include the header and go.
- **Zero overhead** -- thin wrappers around a pointer, size, and cursor.  The
  compiler inlines everything at `-O1` and above.
- **Big-endian bit order** -- bit 0 of the stream is the MSB of byte 0
  (standard for compressed formats, codecs, network protocols).
- **Fixed and dynamic storage** -- the same API covers stack/static buffers,
  non-owning views, read-only views, and heap-backed growable streams.
- **Fixed-width fast paths** -- for compile-time-known field widths the
  X-macro-generated `read_fixed_N` / `write_fixed_N` functions let the
  compiler eliminate all branches and unroll all loops.

---

## Picking the right header

```
Do you need C or C++?
|
+-- C++  --------------------------------------------------------------------------+
|                                                                                  |
|   bitstream_t.hpp        C++17 template, all storage variants                    |
|   bitstream_t_cpp11.hpp  Same, C++11-compatible                                  |
|                                                                                  |
|   Six ready-made aliases (see "Type aliases"):                                   |
|     BitStream / BitStreamView / BitStreamReader         (64-bit cursor, 64-bit fields) |
|     BitStream32 / BitStreamView32 / BitStreamReader32   (64-bit cursor, 32-bit fields) |
|                                                                                  |
+----------------------------------------------------------------------------------+
|
+-- C
   |
   +-- Do you need the buffer to grow automatically?
   |  |
    |  +-- YES  --  bitstream_dyn32.h  /  bitstream_dyn64.h
    |              (or canonical aliases: bitstream32.h / bitstream64.h)
   |
   +-- NO (fixed buffer or view)
      |
      +-- Need both read AND write on the same buffer?
      |  |
      |  +-- YES  --  bitstream_t32.h  /  bitstream_t64.h
      |              (or aliases: bitstream_view32/64.h, bitstream_const32/64.h)
      |
      +-- Need only ONE direction?
         |
         +-- Read only  --  bitstream_reader32.h  /  bitstream_reader64.h
         +-- Write only --  bitstream_writer32.h  /  bitstream_writer64.h
```

The `32` / `64` suffix refers to the **maximum field width** -- the integer
type used for bit counts and read/write values.  The bit cursor and buffer
size are always 64-bit in all variants, so streams may be arbitrarily large
regardless of suffix.  Choose `32` when your format fields are 32 bits wide
or narrower; choose `64` when you need to read or write fields up to 64 bits.

---

## C API

### Fixed-buffer combined (BitStreamT)

**Headers:** `bitstream_t32.h`, `bitstream_t64.h`
**Structs:** `BitStreamT32`, `BitStreamT64`
**Prefixes:** `bst32_`, `bst64_`

A single struct that can both read and write.  The buffer is caller-managed;
no allocation takes place.  Overflow triggers `assert`.

Implementation note: `bitstream_t32.h` / `bitstream_t64.h` now keep their
original 3-field public structs, but forward the read/write/fixed-width core
through `bitstream_dyn32.h` / `bitstream_dyn64.h` in fixed mode
(`realloc_fn == NULL`).

```c
#include "bitstream_t32.h"

/* --- mutable buffer (read + write) --- */
BitStreamT32 s;
bst32_init(&s, buffer, sizeof(buffer), 0);

bst32_write(&s, 5, 0x1A);        /* write 5 bits, value 0x1A */
bst32_write_fixed_11(&s, 0x7FF); /* write 11 bits (fixed-width, branch-free) */

bst32_rewind(&s);

unsigned long a = bst32_read(&s, 5);          /* runtime width */
unsigned long b = bst32_read_fixed_11(&s);    /* compile-time width */

/* --- read-only view (casts away const internally; do not write) --- */
BitStreamT32 r;
bst32_init_ro(&r, data, byte_count, 0);
unsigned long v = bst32_read(&r, 6);
```

### Dynamic combined (BitStreamDyn)

**Headers:** `bitstream_dyn32.h`, `bitstream_dyn64.h`
**Structs:** `BitStreamDyn32`, `BitStreamDyn64`
**Prefixes:** `bsd32_`, `bsd64_`

Extends the combined struct with an optional resize callback.  When
`realloc_fn` is `NULL` the behaviour is identical to `BitStreamT` (fixed,
assert on overflow).  When non-NULL the buffer doubles on demand.

This is also the shared implementation underneath the fixed and one-direction
C headers.

```c
#include "bitstream_dyn32.h"

/* dynamic: starts empty, grows with malloc */
BitStreamDyn32 s;
bsd32_init_dyn(&s, bsd32_stdlib_realloc, NULL);

for(int i = 0; i < 1000; i++)
    bsd32_write(&s, 13, values[i]);   /* buffer grows transparently */

bsd32_shrink_to_size(&s);            /* trim allocation to written bytes */
bsd32_rewind(&s);

for(int i = 0; i < 1000; i++)
    decoded[i] = bsd32_read(&s, 13);

bsd32_free(&s);

/* custom allocator: arena example */
bsd32_init_dyn(&s, my_arena_realloc, &arena);
```

The `realloc_fn` signature is:

```c
void* realloc_fn(void *ptr, size_t new_bytes, void *ctx);
```

`new_bytes == 0` signals a free (the default `bsd32_stdlib_realloc` calls
`free(ptr)` in this case).

The struct separates `capacity` (allocated bits) from `size` (high-water mark
of valid bits written), matching the `vector::capacity` / `vector::size`
distinction in the C++ template.

#### Fixed and read-only modes

`BitStreamDyn` also exposes `bsd32_init_fixed` and `bsd32_init_ro` for use as
a drop-in replacement for `BitStreamT` when you want a single struct type
throughout a codebase:

```c
/* fixed mutable view */
bsd32_init_fixed(&s, buffer, sizeof(buffer), 0);

/* read-only view */
bsd32_init_ro(&s, const_data, byte_count, 0);
```

For repo-local validation of the header-only C surface, run:

```sh
make check-c-bitstream-headers
```

### Read-only (BitStreamReader)

**Headers:** `bitstream_reader32.h`, `bitstream_reader64.h`
**Structs:** `BitStreamReader32`, `BitStreamReader64`
**Prefixes:** `bsr32_`, `bsr64_`

Specialised for decoding.  Holds a `const` data pointer.  Slightly simpler
struct (no write machinery) and a useful choice when you want the type system
to prevent accidental writes at the call site.

Implementation note: `bitstream_reader32.h` / `bitstream_reader64.h` keep
their compact 3-field public structs, but forward the read/fixed-width core
through `bitstream_dyn32.h` / `bitstream_dyn64.h` in fixed read-only mode
(`realloc_fn == NULL`).

```c
#include "bitstream_reader32.h"

BitStreamReader32 r;
bsr32_init(&r, data, byte_count, 0);

unsigned long x = bsr32_read(&r, 7);
unsigned long y = bsr32_read_fixed_4(&r);   /* compile-time 4 bits */
bsr32_skip_to_8bit_boundary(&r);
```

### Write-only (BitStreamWriter)

**Headers:** `bitstream_writer32.h`, `bitstream_writer64.h`
**Structs:** `BitStreamWriter32`, `BitStreamWriter64`
**Prefixes:** `bsw32_`, `bsw64_`

Specialised for encoding.  Holds a writable pointer and a `capacity` field.
Asserts if capacity is exceeded.

Implementation note: `bitstream_writer32.h` / `bitstream_writer64.h` keep
their compact 3-field public structs, but forward the write/fixed-width core
through `bitstream_dyn32.h` / `bitstream_dyn64.h` in fixed mode
(`realloc_fn == NULL`).

```c
#include "bitstream_writer32.h"

BitStreamWriter32 w;
bsw32_init(&w, buffer, sizeof(buffer), 0);

bsw32_write(&w, 3, 0x5);
bsw32_write_fixed_16(&w, header_word);
bsw32_skip_to_32bit_boundary(&w);
```

### C alias headers

These are thin `#include` + `typedef` wrappers that provide descriptive type
names matching the C++ aliases, with zero additional code.

| Header                   | Typedef          | Analog                                    |
|--------------------------|------------------|-------------------------------------------|
| `bitstream_view32.h`     | `BitStreamView32`  | `BitStreamT<BitStreamSpan<u8>, u32>`    |
| `bitstream_view64.h`     | `BitStreamView`, `BitStreamView64`  | `BitStreamT<BitStreamSpan<u8>>`         |
| `bitstream_const32.h`    | `BitStreamConst32` | `BitStreamT<BitStreamConstSpan<u8>, u32>` |
| `bitstream_const64.h`    | `BitStreamConst64` | `BitStreamT<BitStreamConstSpan<u8>>`    |
| `bitstream32.h`          | `BitStream32`      | owning dynamic C89 type |
| `bitstream64.h`          | `BitStream`, `BitStream64`      | owning dynamic C89 type |

`View`   = mutable fixed buffer (init with `bst*_init`).
`Const`  = read-only fixed buffer (init with `bst*_init_ro`).
`Stream` = dynamic growable buffer (init with `bsd*_init_dyn`).

For naming parity with the C++ headers, the 64-bit aliases now also export the
unsuffixed names `BitStream`, `BitStreamView`, and `BitStreamReader`.

On the C89 side there is no separate `BitStreamRealloc*` family. The owning
dynamic type is simply `BitStream32` / `BitStream` / `BitStream64`.

### Function reference

All fixed-buffer families (`bst32_`, `bsr32_`, `bsw32_`, and their `64`
counterparts) share the same navigation API.  The dynamic family (`bsd32_`,
`bsd64_`) adds allocation and size-management functions.

#### Navigation (all families)

| Function                       | Description                                          |
|--------------------------------|------------------------------------------------------|
| `*_seek(s, idx)`               | Move cursor to bit `idx`                             |
| `*_rewind(s)`                  | Move cursor to bit 0                                 |
| `*_rewind_bits(s, n)`          | Move cursor back `n` bits                            |
| `*_skip(s, n)`                 | Advance cursor by `n` bits                           |
| `*_skip_to_8bit_boundary(s)`   | Advance to next byte boundary                        |
| `*_skip_to_16bit_boundary(s)`  | Advance to next 16-bit boundary                      |
| `*_skip_to_32bit_boundary(s)`  | Advance to next 32-bit boundary                      |
| `*_skip_to_64bit_boundary(s)`  | Advance to next 64-bit boundary (64-bit only)        |
| `*_on_8bit_boundary(s)`        | Returns non-zero if cursor is byte-aligned           |
| `*_bits_to_8bit_boundary(s)`   | Bits remaining until next byte boundary              |
| `*_tell(s)`                    | Current cursor position in bits                      |
| `*_tell_bits(s)`               | Alias for `*_tell`                                   |
| `*_tell_bytes(s)`              | Cursor in bytes (rounded up)                         |
| `*_tell_u32(s)`                | Cursor in 32-bit words (rounded up)                  |

#### Size / capacity (combined and dynamic families)

| Function                  | Description                                             |
|---------------------------|---------------------------------------------------------|
| `*_size(s)`               | Total bits in buffer (reader/writer)                    |
| `bsd*_size_bits(s)`       | High-water mark in bits                                 |
| `bsd*_size_8bits(s)`      | High-water mark in bytes (rounded up)                   |
| `bsd*_size_32bits(s)`     | High-water mark in 32-bit words (rounded up)            |
| `bsd*_capacity_bits(s)`   | Allocated capacity in bits                              |
| `bsd*_capacity_bytes(s)`  | Allocated capacity in bytes                             |
| `bsd*_shrink_to_idx(s)`   | Reallocate down to cursor position                      |
| `bsd*_shrink_to_size(s)`  | Reallocate down to high-water mark                      |
| `bsd*_set_size_bits(s,n)` | Set high-water mark and shrink                          |
| `bsd*_set_size_8bits(s,n)`| Set high-water mark in bytes                            |
| `bsd*_set_size_32bits(s,n)`| Set high-water mark in 32-bit words                   |

#### Read (combined, read-only, and dynamic families)

| Function                     | Description                                          |
|------------------------------|------------------------------------------------------|
| `*_read_at(s, idx, bits)`    | Read `bits` bits at absolute position `idx`          |
| `*_read(s, bits)`            | Read `bits` bits at cursor and advance               |
| `*_read_fixed_N_at(s, idx)`  | Read exactly `N` bits at absolute position           |
| `*_read_fixed_N(s)`          | Read exactly `N` bits at cursor and advance          |

#### Write (combined, write-only, and dynamic families)

| Function                          | Description                                       |
|-----------------------------------|---------------------------------------------------|
| `*_write_at(s, idx, bits, val)`   | Write `val` into `bits` bits at position `idx`   |
| `*_write(s, bits, val)`           | Write `val` into `bits` bits at cursor and advance|
| `*_write_bytes(s, src, count)`    | Write `count` raw bytes; memcpy when byte-aligned |
| `*_write_fixed_N_at(s, idx, val)` | Write `N` bits at absolute position               |
| `*_write_fixed_N(s, val)`         | Write `N` bits at cursor and advance              |
| `*_zero_till_8bit_boundary(s)`    | Write zero bits up to next byte boundary (combined/dynamic only) |
| `*_zero_till_16bit_boundary(s)`   | Write zero bits up to next 16-bit boundary (combined/dynamic only) |
| `*_zero_till_32bit_boundary(s)`   | Write zero bits up to next 32-bit boundary (combined/dynamic only) |
| `*_zero_till_64bit_boundary(s)`   | Write zero bits up to next 64-bit boundary (64-bit combined/dynamic only) |

### Fixed-width functions

Every header pre-generates a `read_fixed_N` and (where applicable)
`write_fixed_N` pair for every valid `N` using an X-macro:

- `bitstream_reader32.h` / `bitstream_writer32.h` / `bitstream_t32.h` / `bitstream_dyn32.h` -- N = 1-32
- `bitstream_reader64.h` / `bitstream_writer64.h` / `bitstream_t64.h` / `bitstream_dyn64.h` -- N = 1-64

Because `N` is a compile-time constant baked into the function body, the
compiler can:
- Compute `BYTES = (7 + N + 7) / 8` at compile time
- Unroll the byte-accumulation loop
- Eliminate the 5-byte / 9-byte overflow branch for N <= 25 (32-bit) or N <= 57 (64-bit)

Use these whenever the field width is known at compile time.

---

## C++ API

### BitStreamT template

**Headers:** `bitstream_t.hpp` (C++17), `bitstream_t_cpp11.hpp` (C++11)

```cpp
template<typename Storage, typename Word = u64>
class BitStreamT;
```

`Storage` is any type providing `.data()` and `.size()`.  When `Storage` also
provides `.resize(n)`, write operations and `seek` will grow the buffer
automatically -- detected via `is_resizable<Storage>` and dispatched with
`if constexpr` (C++17) or SFINAE (C++11).

`Word` must be `u32` or `u64` and selects the integer width used for runtime
field widths and read/write values.  Cursor positions and sizes are always
`u64`, so `BitStreamT<Storage, u32>` still supports arbitrarily large streams
while limiting each read/write operation to 32 bits.

#### Storage concepts

| Storage type             | Resizable | Writable | Notes                    |
|--------------------------|-----------|----------|--------------------------|
| `std::vector<u8>`        | yes       | yes      | Owning heap buffer       |
| `BitStreamReallocStorage<u8>` | yes  | yes      | Owning realloc-backed buffer |
| `BitStreamSpan<u8>`      | no        | yes      | Non-owning mutable view  |
| `BitStreamConstSpan<u8>` | no        | no       | Non-owning read-only view|

`BitStreamSpan<T>` and `BitStreamConstSpan<T>` are minimal span types
defined in the header; they do not depend on C++20 `std::span`.

For first-class raw-pointer ownership, `bitstream_t.hpp` also provides:

```cpp
using bitstream_realloc_fn = void* (*)(void *ptr, size_t new_bytes, void *ctx);
void* bitstream_stdlib_realloc(void *ptr, size_t new_bytes, void *ctx);

template<typename T = u8>
class BitStreamReallocStorage;
```

`BitStreamReallocStorage<T>` owns a contiguous buffer through a realloc-style
callback.  It can start empty, adopt an existing malloc'ed pointer, or use a
custom allocator/context pair.  Newly grown bytes are zero-filled so masked
partial-byte writes behave like `std::vector::resize`.

#### Interface summary

```cpp
// Construction
BitStreamT()                        // empty
explicit BitStreamT(Storage s)      // wrap existing storage

// Access
const Storage& storage() const;
Storage&       storage();
const u8*      data() const;

// Navigation
void seek(u64 idx);
void rewind();
void rewind(u64 bits);
void skip(u64 bits);

bool on_8bit_boundary()  const;
bool on_16bit_boundary() const;
bool on_32bit_boundary() const;
bool on_64bit_boundary() const;

u8 bits_to_8bit_boundary()  const;
u8 bits_to_16bit_boundary() const;
u8 bits_to_32bit_boundary() const;
u8 bits_to_64bit_boundary() const;

void skip_to_8bit_boundary();
void skip_to_16bit_boundary();
void skip_to_32bit_boundary();
void skip_to_64bit_boundary();

void zero_till_8bit_boundary();
void zero_till_16bit_boundary();
void zero_till_32bit_boundary();
void zero_till_64bit_boundary();

u64 tell()       const;   // cursor in bits
u64 tell_bits()  const;
u64 tell_bytes() const;   // rounded up

u64 size_bits()   const;  // high-water mark in bits
u64 size_8bits()  const;
u64 size_32bits() const;

// Size management (resizable storage only)
void shrink_to_idx();
void shrink_to_size();
void set_size_bits(u64 n);
void set_size_8bits(u64 n);
void set_size_32bits(u64 n);

// Read
Word read_bit(u64 idx) const;
Word read(u64 idx, Word bits) const;  // random access
Word read(Word bits);                 // streaming
template<u64 BITS> Word read(u64 idx) const;  // fixed-width random access
template<u64 BITS> Word read();               // fixed-width streaming

// Write (requires writable storage)
void write(u64 idx, Word bits, Word val);  // random access
void write(Word bits, Word val);           // streaming
template<u64 BITS> void write(u64 idx, Word val);  // fixed-width random access
template<u64 BITS> void write(Word val);           // fixed-width streaming
void write_bytes(const u8 *src, u64 count);

// Compare
template<typename OtherStorage, typename OtherWord>
bool cmp(u64 idx, const BitStreamT<OtherStorage,OtherWord>& other,
         u64 other_idx, u64 length) const;
```

### Type aliases

```cpp
// 64-bit cursor, 64-bit field width (default)
using BitStream       = BitStreamT<std::vector<u8>>;           // owning, resizable
using BitStreamRealloc = BitStreamT<BitStreamReallocStorage<u8>>; // owning, realloc-backed
using BitStreamView   = BitStreamT<BitStreamSpan<u8>>;         // mutable view
using BitStreamReader = BitStreamT<BitStreamConstSpan<u8>>;    // read-only view

// 64-bit cursor, 32-bit field width
using BitStream32       = BitStreamT<std::vector<u8>, u32>;
using BitStreamRealloc32 = BitStreamT<BitStreamReallocStorage<u8>, u32>;
using BitStreamView32   = BitStreamT<BitStreamSpan<u8>, u32>;
using BitStreamReader32 = BitStreamT<BitStreamConstSpan<u8>, u32>;
```

#### C++ usage examples

```cpp
#include "bitstream_t.hpp"

// Owning stream: encode
BitStream bs;
bs.write(5, 0x1A);
bs.write<11>(0x7FF);                // fixed-width, branch-free
bs.zero_till_8bit_boundary();

// Read back
bs.rewind();
u64 a = bs.read(5);
u64 b = bs.read<11>();

// Read-only view over existing data
BitStreamReader r(BitStreamConstSpan<u8>(data, byte_count));
u64 x = r.read<6>();
r.skip_to_32bit_boundary();
u64 y = r.read(17);

// 32-bit field width for smaller per-read values, still with a 64-bit cursor
BitStream32 small;
small.write(3, 0x5);
u32 v = small.read<3>();

// Owning realloc-backed storage (default stdlib realloc/free)
BitStreamRealloc dyn;
dyn.seek(4);
dyn.write(4, 0xA);                  // newly grown high bits stay zero

u8 *owned = dyn.storage().release();
std::free(owned);

// Adopt an existing malloc'ed pointer
u8 *raw = static_cast<u8*>(std::malloc(byte_count));
std::memset(raw, 0, byte_count);

BitStreamRealloc adopted(BitStreamReallocStorage<u8>(raw, byte_count));
adopted.seek(12);
adopted.write(4, 0xB);

// Custom realloc callback
BitStreamRealloc custom(BitStreamReallocStorage<u8>(nullptr, 0, my_realloc, ctx));
```

---

## Implementation notes

### Bit ordering

Bits are numbered from the MSB of byte 0.  Bit 0 is the most significant bit
of `data[0]`; bit 7 is the least significant.  This is the standard ordering
for MPEG, JPEG, Deflate, and most network protocol fields.

### bswap fast path

On GCC, Clang, and MSVC the read path loads a full 32-bit or 64-bit word with
`memcpy` (avoiding alignment faults) then byte-swaps it with the hardware
instruction (`__builtin_bswap32`, `_byteswap_ulong`, etc.).  The value is then
shifted and masked to extract the requested bits.  This typically compiles to
2-3 instructions for reads that fit within one word.

On compilers without bswap intrinsics the fallback loops over individual bytes;
the compiler still unrolls these for fixed-width calls.

### 5-byte / 9-byte span

A read of `N` bits starting at a bit offset `off` within a byte may require
reading bytes at positions `[byte, byte+4]` for 32-bit (when `off + N > 32`)
or `[byte, byte+8]` for 64-bit (when `off + N > 64`).  For fixed-width reads
with N <= 25 (32-bit) or N <= 57 (64-bit) the maximum bit offset of 7 means
`off + N <= 32` or `<= 64` always holds, so the extended-span branch is
statically dead and elided.

### Growth strategy (dynamic variants)

`BitStreamDyn` starts with NULL / 0 capacity and doubles on each overflow,
with a minimum initial allocation of 8 bytes.  The doubling ensures amortised
O(1) appends.  Call `bsd*_shrink_to_size()` after encoding is complete to
reclaim unused capacity before passing the buffer to a caller.

### X-macro expansion

The fixed-width function families are generated with:

```c
#define X(n) BST32_DEFINE_FIXED(n)
BST32_X_ALL   /* expands X(1) X(2) ... X(32) */
#undef X
```

This keeps the header free of 32 (or 64) repetitive hand-written functions
while still giving the compiler full visibility into each constant `N`.

---

## File index

### C headers

All variants use a 64-bit cursor and size.  "Value type" is the integer type
returned by reads and accepted by writes.

| File                    | Struct              | Prefix   | Value type | Read | Write | Dynamic |
|-------------------------|---------------------|----------|------------|------|-------|---------|
| `bitstream_reader32.h`  | `BitStreamReader32` | `bsr32_` | u32        | yes  | no    | no      |
| `bitstream_reader64.h`  | `BitStreamReader64` | `bsr64_` | u64        | yes  | no    | no      |
| `bitstream_writer32.h`  | `BitStreamWriter32` | `bsw32_` | u32        | no   | yes   | no      |
| `bitstream_writer64.h`  | `BitStreamWriter64` | `bsw64_` | u64        | no   | yes   | no      |
| `bitstream_t32.h`       | `BitStreamT32`      | `bst32_` | u32        | yes  | yes   | no      |
| `bitstream_t64.h`       | `BitStreamT64`      | `bst64_` | u64        | yes  | yes   | no      |
| `bitstream_dyn32.h`     | `BitStreamDyn32`    | `bsd32_` | u32        | yes  | yes   | yes     |
| `bitstream_dyn64.h`     | `BitStreamDyn64`    | `bsd64_` | u64        | yes  | yes   | yes     |

### C alias headers

| File                    | Typedef            | Underlying header    |
|-------------------------|--------------------|----------------------|
| `bitstream_view32.h`    | `BitStreamView32`  | `bitstream_t32.h`    |
| `bitstream_view64.h`    | `BitStreamView`, `BitStreamView64`  | `bitstream_t64.h`    |
| `bitstream_const32.h`   | `BitStreamConst32` | `bitstream_t32.h`    |
| `bitstream_const64.h`   | `BitStreamConst64` | `bitstream_t64.h`    |
| `bitstream32.h`         | `BitStream32`      | `bitstream_dyn32.h`  |
| `bitstream64.h`         | `BitStream`, `BitStream64`      | `bitstream_dyn64.h`  |

### C++ headers

| File                    | Standard | Notes                                           |
|-------------------------|----------|-------------------------------------------------|
| `bitstream_t.hpp`       | C++17    | Primary template, `if constexpr`, `std::void_t` |
| `bitstream_t_cpp11.hpp` | C++11    | SFINAE dispatch, `void_t` backport, tag dispatch|
