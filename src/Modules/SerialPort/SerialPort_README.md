# SerialPort Module

A compact module for line‑oriented serial I/O. It provides:
- **Raw output** helpers.
- **Framed/boxed output** with edges, margins, alignment, and **word‑wrap by default**.
- **Separators, spacers, and headers** for quick UI framing.
- **Typed input getters** with validation, retries, and timeouts.

The loop echoes input bytes and assembles complete lines. Getters sit on top of that loop and enforce constraints.

---

## Quick overview

### Key prints
```cpp
// Word‑wrap by default ('w'). Use 'c' for fixed character wrap.
void print(string_view message            = {},
           string_view edge_character     = {},
           const char  text_align         = 'l',   // 'l' | 'c' | 'r'
           const char  wrap_mode          = 'w',   // 'w' word | 'c' char
           const uint16_t message_width   = 0,     // 0 = no wrap, no align/pad
           const uint16_t margin_l        = 0,
           const uint16_t margin_r        = 0,
           string_view   end              = kCRLF);

void printf(string_view  edge_character,
            const char   text_align,
            const char   wrap_mode,              // 'w' or 'c'
            const uint16_t message_width,
            const uint16_t margin_l,
            const uint16_t margin_r,
            string_view   end,
            const char*   fmt, ...);
```

- `wrap_mode='w'` keeps whole words on a line when possible. Words longer than `message_width` are hard‑split.
- `wrap_mode='c'` uses strict fixed‑width chunks.
- `message_width==0` disables wrapping and padding. Content is placed as‑is between edges and margins.
- `text_align` applies only when `message_width>0`.

### Other framing
```cpp
void print_separator(uint16_t total_width=50,
                     string_view fill="-",
                     string_view edge_character="+");

void print_spacer(uint16_t total_width=50,
                  string_view edge_character="|");

void print_header(string_view message,
                  uint16_t total_width=50,
                  string_view edge_character="|",
                  string_view cross_edge_character="+",
                  string_view sep_fill="-");
```

### Typed input getters
```cpp
string  get_string(...);
int     get_int(...);
uint8_t get_uint8(...);
uint16_t get_uint16(...);
uint32_t get_uint32(...);
float   get_float(...);
bool    get_yn(...);
```
All getters: prompt → read line (with optional timeout) → parse/validate → retry up to `retry_count` (`0` = infinite) → default on failure.

---

## Quick start

```cpp
SerialPort sp(controller);
SerialPortConfig cfg;
cfg.baud_rate = 115200;
sp.begin_routines_required(cfg);

// In your main loop:
sp.loop();
```

---

## Examples

### Basic framed prints
```cpp
sp.print_header("Serial Port\\sepReady", 40);
sp.print_separator(40, "=", "+");

sp.print("left",   "|", 'l', 'w', 12, 1, 1);
sp.print("center", "|", 'c', 'w', 12, 0, 0);
sp.print("right",  "|", 'r', 'w', 12, 2, 0);

sp.print_spacer(40, "|");
sp.print_separator(40, "-", "+");
```

### Word‑wrap vs char‑wrap
```cpp
// Word‑wrap keeps words together where possible.
sp.print("this is a pretty long centered text. i am curious if wrapping is working well",
         "|", 'c', 'w', 12, 0, 0);

// Char‑wrap splits at exact width.
sp.print("this is a pretty long centered text. i am curious if wrapping is working well",
         "|", 'c', 'c', 12, 0, 0);
```

### printf with framing
```cpp
sp.printf("|", 'l', 'w', 10, 0, 0, kCRLF, "fmt %d %s", 7, "seven");
```

### Integer input with bounds
```cpp
bool ok = false;
int n = sp.get_int("Pick a number [0..10]",
                   0, 10,
                   /*retry_count*/ 3,
                   /*timeout_ms*/ 5000,
                   /*default*/ 5,
                   std::ref(ok));
if (!ok) sp.println_raw("Fell back to default = 5.");
```

### Yes/No
```cpp
bool ok = false;
bool proceed = sp.get_yn("Continue?", 1, 3000, false, std::ref(ok));
if (!ok || !proceed) sp.println_raw("Stopped.");
```

### Read a raw line with a deadline
```cpp
std::string line;
if (sp.read_line_with_timeout(line, 2000)) {
    sp.printf_raw("you said: %s\r\n", line.c_str());
} else {
    sp.println_raw("no line within 2s");
}
```

---

## API reference

### Lifecycle
- **`SerialPort(ModuleController& controller)`**  
  Registers the module and CLI command. No I/O yet.
- **`void begin_routines_required(const ModuleConfig& cfg)`**  
  Sets TX/RX buffer sizes, starts `Serial` at `baud_rate`, then short delay.
- **`void loop()`**  
  Echoes bytes. Ignores `'\r'`. On `'\n'` or buffer end, terminates and marks a line ready.
- **`void reset(bool verbose=false, bool do_restart=true)`**  
  Clears input state and calls base `Module::reset`.

### Output — raw
- **`void print_raw(string_view message)`**  
  Writes bytes as‑is.
- **`void println_raw(string_view message)`**  
  Writes bytes then `CRLF`.
- **`void printf_raw(const char* fmt, ...)`**  
  Uses `vsnprintf`. If no format specs are present, writes the string verbatim.

### Output — boxed
- **`void print(string_view message = {}, string_view edge_character = {}, char align='l', char wrap='w', uint16_t width=0, uint16_t ml=0, uint16_t mr=0, string_view end = kCRLF)`**  
  Splits on `'\n'`. If `width>0` then wrap by word (`wrap='w'`) or by character (`wrap='c'`). Alignment applies only when `width>0`. Writes `end` after the last fragment and `CRLF` between fragments.
- **`void printf(string_view edge, char align, char wrap, uint16_t width, uint16_t ml, uint16_t mr, string_view end, const char* fmt, ...)`**  
  Formats then delegates to `print`.
- **`void print_separator(uint16_t total_width=50, string_view fill="-", string_view edge="+")`**  
  Prints `edge + fill*(total_width-2*edge.size) + edge` when space allows.
- **`void print_spacer(uint16_t total_width=50, string_view edge="|")`**  
  Prints an empty framed line of `total_width` with `edge` characters.
- **`void print_header(string_view message, uint16_t total_width=50, string_view edge="|", string_view cross_edge="+", string_view sep_fill="-")`**  
  Prints a separator, then each `\\sep`‑separated part centered within the inner width, each followed by the same separator.

### Input — lines
- **`bool has_line() const`**  
  True if a full line is ready.
- **`string read_line()`**  
  Returns the line and clears readiness; empty string if none.
- **`void flush_input()`**  
  Drains device and clears state.
- **`bool read_line_with_timeout(string& out, uint32_t timeout_ms)`**  
  Calls `loop()` until a line arrives or the timeout elapses (`0` = no timeout).
- **`void write_line_crlf(string_view s)`**  
  Writes `s` and `CRLF`.

### Input — typed getters
Shared behavior: prompt → iterate with optional timeout → validate → default on failure → optional success flag.

- **Core**
  - `template <typename Ret, typename CheckFn> Ret get_core(...)` — common loop.
  - `template <typename T> T get_integral(...)` — integer parsing and range enforcement.

- **Concrete**
  - `string  get_string(prompt, min_length, max_length, retry_count, timeout_ms, default_value, success_sink)`  
    Accepts length in `[min_length..max_length]`. If `max_length==0`, uses `INPUT_BUFFER_SIZE-1`.
  - `int get_int(...)`, `uint8_t get_uint8(...)`, `uint16_t get_uint16(...)`, `uint32_t get_uint32(...)`  
    Base‑10 parsing. Enforce `[min..max]`.
  - `float get_float(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink)`  
    Parses with `strtod`. Rejects NaN and trailing junk. Enforces range.
  - `bool get_yn(prompt, retry_count, timeout_ms, default_value, success_sink)`  
    Accepts `y/yes/1/true` or `n/no/0/false` (case‑insensitive).

---

## Notes and limits
- Input buffer: **255 bytes**. On overflow the line is force‑ended.
- `'\r'` ignored. `'\n'` commits the line.
- Wrapping counts **bytes**, not glyphs. Non‑ASCII or multi‑byte UTF‑8 may not align visually.
- Word‑wrap uses ASCII `isspace` semantics.
- `message_width==0` disables both wrapping and padding; alignment is bypassed.

---

## Changelog
- **Added**: `wrap_mode` to `print` and `printf`. `'w'` = word‑wrap (default). `'c'` = character‑wrap.
- **Default behavior change**: prints now wrap on **words** when `message_width>0`.

---

## License
PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0. See `LICENSE` and `LICENSE-NO-AI.md` for terms.
