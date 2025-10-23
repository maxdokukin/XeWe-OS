# SerialPort Module

A small module that reads and writes lines over a serial link. It offers framed output, raw output, and typed input with validation. Nothing more. Nothing less.

## Summary

- Lifecycle hooks integrate with the project’s `Module`/`ModuleController` system.
- Output is either raw bytes or “boxed” lines with edges, widths, and alignment.
- Input is line‑buffered. The loop echoes bytes, collects a line, then marks it ready.
- Getters ask the user for typed values, check bounds, and fall back to defaults.
- `retry_count == 0` means infinite retries. `timeout_ms == 0` means no timeout. Both zero means the module will wait as long as the user can stand.

---

## Lifecycle

### `SerialPort(ModuleController& controller)`
Registers the module, CLI command, and description. No I/O yet.

### `void begin_routines_required(const ModuleConfig& cfg)`
Starts the serial port. Sets TX/RX buffer sizes, then calls `Serial.begin(baud_rate)` from `SerialPortConfig`. A short delay follows. After this point the line loop can work.

### `void loop()`
Runs the echo-and-accumulate logic. Each incoming byte is echoed. `'\r'` is ignored. On `'\n'` or buffer end, the internal buffer is nul‑terminated and `line_ready` becomes true.

### `void reset(bool verbose=false, bool do_restart=true)`
Purges input state and calls the base `Module::reset`. Useful when the world needs to be clean again.

---

## Output: raw

These operate directly on the underlying `Serial` without formatting comfort.

- `void print_raw(string_view message)`  
  Writes bytes as‑is.

- `void println_raw(string_view message)`  
  Writes bytes, then `CRLF`.

- `void printf_raw(const char* fmt, ...)`  
  `vsnprintf` into a temporary buffer, then write the bytes. If there are no format specs, it writes the format string verbatim.

Use these when you do not want alignment, margins, or boxes. Truth, unadorned.

---

## Output: boxed

These print framed lines. The box is not real. Only characters and discipline.

- `void print(string_view message, char edge='|', char align='l', uint16_t width=0, uint16_t ml=0, uint16_t mr=0, string_view end=kCRLF)`  
  Splits `message` by `'\n'`. Optionally wraps to `width`. Pads according to `align` (`l`, `c`, `r`). Writes a single or multi‑line frame with margins `ml` and `mr`. Ends with `end` on the last fragment; `CRLF` between fragments.

- `void printf(char edge, char align, uint16_t width, uint16_t ml, uint16_t mr, string_view end, const char* fmt, ...)`  
  Formats a message first, then calls `print` with the same framing parameters.

- `void print_separator(uint16_t total_width=50, char fill='-', char edge='+')`  
  Prints a rule line: `edge + fill*(total_width-2) + edge`.

- `void print_spacer(uint16_t total_width=50, char edge='|')`  
  Prints an empty framed line of `total_width` with `edge` characters.

- `void print_header(string_view message, uint16_t total_width=50, char edge='|', char sep_edge='+', char sep_fill='-')`  
  Prints a separator, then each part of `message` split by the token `\\sep`, each centered, each separated by the same rule.

Internals depend on helpers such as `wrap_fixed`, `compose_box_line`, `make_rule_line`, and `make_spacer_line`. They are expected to exist in the project’s utility layer.

---

## Input: lines

- `bool has_line() const`  
  True if a full line is ready.

- `string read_line()`  
  Returns the accumulated line. Resets internal state. If no line is ready, returns an empty string.

- `void flush_input()`  
  Drains the serial device. Resets the buffer and the ready flag.

- `bool read_line_with_timeout(string& out, uint32_t timeout_ms)`  
  Polls via `loop()` until a line arrives or time runs out. If `timeout_ms == 0`, time does not run out.

- `void write_line_crlf(string_view s)`  
  Writes `s` followed by `CRLF`.

Buffer details: `INPUT_BUFFER_SIZE == 255`. `'\r'` is discarded. `'\n'` ends the line. If the buffer fills, the line is force‑ended and marked ready. This is a hard world. Choose your words carefully.

---

## Input: getters (typed)

All getters share the same pattern: prompt, parse, validate, repeat or default, and report success if requested.

### Core

- `template <typename Ret, typename CheckFn> Ret get_core(...)`  
  The loop. Prints the initial prompt. Shows the iterative prompt for each attempt. Reads a line with timeout. Calls the checker. On failure, prints an error (if any) and tries again. On exit, writes `success_sink` if supplied.

- `template <typename T> T get_integral(...)`  
  Specialization wrapper for integers. Normalizes `min_value` and `max_value` if swapped. Parses base‑10. On range error, prints the accepted range and tries again.

### Concrete getters

- `string get_string(prompt, min_length, max_length, retry_count, timeout_ms, default_value, success_sink)`  
  Accepts a string within `[min_length..max_length]`. If `max_length == 0`, it uses `INPUT_BUFFER_SIZE - 1`.

- `int get_int(...)`  
- `uint8_t get_uint8(...)`  
- `uint16_t get_uint16(...)`  
- `uint32_t get_uint32(...)`  
  Parse base‑10 integers. Enforce `[min_value..max_value]`.

- `float get_float(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink)`  
  Parses with `strtod`. Rejects NaN and trailing junk. Enforces `[min_value..max_value]`.

- `bool get_yn(prompt, retry_count, timeout_ms, default_value, success_sink)`  
  Accepts `y/yes/1/true` or `n/no/0/false` (case‑insensitive).

### Time and hope

- `retry_count == 0` ⇒ infinite attempts.  
- `timeout_ms == 0` ⇒ no timeout while waiting for a line.  
- Together they wait forever. The module can outlast the user.

On timeout or after the final failed attempt, the function returns `default_value` and marks `success_sink=false` if provided.

---

## Examples

### Initialization

```cpp
SerialPort sp(controller);
SerialPortConfig cfg;
cfg.baud_rate = 115200;
sp.begin_routines_required(cfg);
// In your global loop, ensure sp.loop() is called so input accumulates.
```

### Framed output

```cpp
sp.print_header("Serial Port\\sepReady", 40);
sp.print("left",  '|', 'l', 12, 1, 1);
sp.print("center",'|', 'c', 12, 0, 0);
sp.print("right", '|', 'r', 12, 2, 0);
sp.print_separator(40, '=');
```

### Integer input with bounds and finite patience

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

### Unsigned input, infinite wait

```cpp
bool ok = false;
uint16_t port = sp.get_uint16("Port [1024..65535]",
                              1024, 65535,
                              /*retry_count*/ 0,     // infinite
                              /*timeout_ms*/ 0,      // no timeout
                              /*default*/ 8080,
                              std::ref(ok));
```

### Floating‑point input

```cpp
bool ok = false;
float gain = sp.get_float("Gain [-10.5..10.5]",
                          -10.5f, 10.5f,
                          /*retry_count*/ 2,
                          /*timeout_ms*/ 4000,
                          /*default*/ 1.0f,
                          std::ref(ok));
```

### Yes/No gate

```cpp
bool ok = false;
bool proceed = sp.get_yn("Continue?",
                         /*retry_count*/ 1,
                         /*timeout_ms*/ 3000,
                         /*default*/ false,
                         std::ref(ok));
if (!ok || !proceed) {
    sp.println_raw("Stopped by user or silence.");
}
```

### Reading a raw line with a deadline

```cpp
std::string line;
if (sp.read_line_with_timeout(line, 2000)) {
    sp.printf_raw("you said: %s\r\n", line.c_str());
} else {
    sp.println_raw("no line within 2s");
}
```

---

## Notes

- Input buffer is 255 bytes. Longer lines are cut at the edge and terminated.
- `'\r'` is ignored. `'\n'` commits the line.
- The module echoes input bytes. Silence is not its way.
- Helper utilities for wrapping, composing lines, and trimming must exist in the project.
- This module is not thread‑safe. It is simple, consistent, and blunt.

## License

PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0. See the repository’s `LICENSE` and `LICENSE-NO-AI.md` for terms.
