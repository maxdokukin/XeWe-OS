/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/
#include "SerialPort.h"
#include "../../ModuleController/ModuleController.h"

#include <Arduino.h>
#include <cstdarg>
#include <cstdio>
#include <vector>
#include <limits>
#include <type_traits>

// -------------------------------------------------------------------------------------------------
// Module lifecycle
// -------------------------------------------------------------------------------------------------
SerialPort::SerialPort(ModuleController& controller)
    : Module(controller,
             /* module_name         */ "Serial_Port",
             /* module_description  */ "Allows to send and receive text messages over the USB wire",
             /* nvs_key             */ "ser",
             /* requires_init_setup */ false,
             /* can_be_disabled     */ false,
             /* has_cli_cmds        */ true) {
    commands_storage.push_back({
        "test",
        "test available functions",
        string("Sample Use: $") + lower(module_name) + " test",
        0,
        [this](string_view args) { test(); }
    });
}

void SerialPort::begin_routines_required(const ModuleConfig& cfg) {
    const auto& config = static_cast<const SerialPortConfig&>(cfg);
    Serial.setTxBufferSize(2048);
    Serial.setRxBufferSize(1024);
    Serial.begin(config.baud_rate);
    delay(1000);
}

void SerialPort::test() {
    auto banner = [&](string_view fn) {
        printf_raw("[TEST] ------------------------------------------------\r\n");
        printf_raw("[TEST] %.*s BEGIN\r\n", int(fn.size()), fn.data());
    };
    auto done = [&](string_view fn) {
        printf_raw("[TEST] %.*s END\r\n", int(fn.size()), fn.data());
        printf_raw("[TEST] ------------------------------------------------\r\n");
    };
    auto feed_line = [&](const char* s){
        size_t n = strlen(s);
        if (n >= INPUT_BUFFER_SIZE) n = INPUT_BUFFER_SIZE - 1;
        memcpy(input_buffer, s, n);
        input_buffer[n] = '\0';
        input_buffer_pos = 0;
        line_length = n;
        line_ready  = true;
    };

    // RAW OUTPUT
    banner("print_raw");
    printf_raw("[TEST] in : \"raw\"\r\n");
    print_raw("raw");
    printf_raw("[TEST] out: printed\r\n");
    done("print_raw");

    banner("println_raw");
    printf_raw("[TEST] in : \"line\"\r\n");
    println_raw("line");
    printf_raw("[TEST] out: printed with CRLF\r\n");
    done("println_raw");

    banner("printf_raw");
    printf_raw("[TEST] in : fmt=\"num=%%d str=%%s\", 42, \"ok\"\r\n");
    printf_raw("num=%d str=%s\r\n", 42, "ok");
    printf_raw("[TEST] out: printed\r\n");
    done("printf_raw");

    // BOXED PRINT API
    banner("print_separator");
    printf_raw("[TEST] in : total_width=20, fill='-', edge='+'\r\n");
    print_separator(20, '-', '+');
    printf_raw("[TEST] out: printed\r\n");
    done("print_separator");

    banner("print_spacer");
    printf_raw("[TEST] in : total_width=20, edge='|'\r\n");
    print_spacer(20, '|');
    printf_raw("[TEST] out: printed\r\n");
    done("print_spacer");

    banner("print_header");
    printf_raw("[TEST] in : message=\"Header\\sepSub\", total_width=20, edge='|', sep_edge='+', sep_fill='-'\r\n");
    print_header("Header\\sepSub", 20, '|', '+', '-');
    printf_raw("[TEST] out: printed\r\n");
    done("print_header");

    banner("print");
    printf_raw("[TEST] in : message=\"left\", edge='|', align='l', width=10, ml=1, mr=1, end=CRLF\r\n");
    print("left",  '|', 'l', 10, 1, 1, kCRLF);
    printf_raw("[TEST] out: printed\r\n");
    printf_raw("[TEST] in : message=\"center\", edge='|', align='c', width=12, ml=0, mr=0, end=CRLF\r\n");
    print("center",'|', 'c', 12, 0, 0, kCRLF);
    printf_raw("[TEST] out: printed\r\n");
    printf_raw("[TEST] in : message=\"right\", edge='|', align='r', width=12, ml=2, mr=0, end=CRLF\r\n");
    print("right", '|', 'r', 12, 2, 0, kCRLF);
    printf_raw("[TEST] out: printed\r\n");
    done("print");

    banner("printf (boxed)");
    printf_raw("[TEST] in : edge='|', align='l', width=10, ml=0, mr=0, end=CRLF, fmt=\"fmt %%d %%s\", 7, \"seven\"\r\n");
    printf('|','l', 10, 0, 0, kCRLF, "fmt %d %s", 7, "seven");
    printf_raw("[TEST] out: printed\r\n");
    done("printf (boxed)");

    // INPUT AND LINE UTILITIES
    banner("has_line/read_line");
    printf_raw("[TEST] in : feed_line(\"hello\")\r\n");
    feed_line("hello");
    bool hl = has_line();
    printf_raw("[TEST] out: has_line=%s\r\n", hl ? "true" : "false");
    string got = read_line();
    printf_raw("[TEST] out: read_line=\"%s\"\r\n", got.c_str());
    printf_raw("[TEST] out: post: has_line=%s\r\n", has_line() ? "true" : "false");
    done("has_line/read_line");

    banner("flush_input");
    printf_raw("[TEST] in : preload buffer with \"xxx\"\r\n");
    feed_line("xxx");
    flush_input();
    printf_raw("[TEST] out: input_buffer_pos=%u line_length=%u line_ready=%s\r\n",
               static_cast<unsigned>(input_buffer_pos),
               static_cast<unsigned>(line_length),
               line_ready ? "true" : "false");
    done("flush_input");

    banner("read_line_with_timeout");
    printf_raw("[TEST] in : feed_line(\"123\"), timeout_ms=1000\r\n");
    feed_line("123");
    string outLine;
    bool ok = read_line_with_timeout(outLine, 1000);
    printf_raw("[TEST] out: ok=%s, line=\"%s\"\r\n", ok ? "true" : "false", outLine.c_str());
    done("read_line_with_timeout");

    banner("write_line_crlf");
    printf_raw("[TEST] in : \"EOL test\"\r\n");
    write_line_crlf("EOL test");
    printf_raw("[TEST] out: printed\r\n");
    done("write_line_crlf");

    // GETTERS: INTEGRAL
    banner("get_int");
    printf_raw("[TEST] in : prompt=\"int?\", default=5, range=[0..100], retries=1, timeout=1000, next_line=\"17\"\r\n");
    feed_line("17");
    bool succ = false;
    int v = get_int("int?", 5, 0, 100, 1, 1000, std::ref(succ));
    printf_raw("[TEST] out: value=%d, success=%s\r\n", v, succ ? "true" : "false");
    done("get_int");

    banner("get_uint8");
    printf_raw("[TEST] in : prompt=\"u8?\", default=9, range=[0..255], retries=1, timeout=1000, next_line=\"200\"\r\n");
    feed_line("200");
    succ = false;
    uint8_t v8 = get_uint8("u8?", 9, 0, 255, 1, 1000, std::ref(succ));
    printf_raw("[TEST] out: value=%u, success=%s\r\n", static_cast<unsigned>(v8), succ ? "true" : "false");
    done("get_uint8");

    banner("get_uint16");
    printf_raw("[TEST] in : prompt=\"u16?\", default=1, range=[0..10000], retries=1, timeout=1000, next_line=\"6553\"\r\n");
    feed_line("6553");
    succ = false;
    uint16_t v16 = get_uint16("u16?", 1, 0, 10000, 1, 1000, std::ref(succ));
    printf_raw("[TEST] out: value=%u, success=%s\r\n", static_cast<unsigned>(v16), succ ? "true" : "false");
    done("get_uint16");

    banner("get_uint32");
    printf_raw("[TEST] in : prompt=\"u32?\", default=2, range=[0..1000000], retries=1, timeout=1000, next_line=\"429496\"\r\n");
    feed_line("429496");
    succ = false;
    uint32_t v32 = get_uint32("u32?", 2, 0u, 1000000u, 1, 1000, std::ref(succ));
    printf_raw("[TEST] out: value=%lu, success=%s\r\n", static_cast<unsigned long>(v32), succ ? "true" : "false");
    done("get_uint32");

    // GETTER: STRING
    banner("get_string");
    printf_raw("[TEST] in : prompt=\"str?\", default=\"xx\", len=[3..10], retries=1, timeout=1000, next_line=\"abcde\"\r\n");
    feed_line("abcde");
    succ = false;
    string s = get_string("str?", "xx", 3, 10, 1, 1000, std::ref(succ));
    printf_raw("[TEST] out: value=\"%s\", success=%s\r\n", s.c_str(), succ ? "true" : "false");
    done("get_string");

    // GETTER: YES/NO
    banner("get_yn true");
    printf_raw("[TEST] in : prompt=\"yn?\", default=false, retries=1, timeout=1000, next_line=\"y\"\r\n");
    feed_line("y");
    succ = false;
    bool b = get_yn("yn?", false, 1, 1000, std::ref(succ));
    printf_raw("[TEST] out: value=%s, success=%s\r\n", b ? "true" : "false", succ ? "true" : "false");
    done("get_yn true");

    banner("get_yn false");
    printf_raw("[TEST] in : prompt=\"yn?\", default=true, retries=1, timeout=1000, next_line=\"no\"\r\n");
    feed_line("no");
    succ = false;
    b = get_yn("yn?", true, 1, 1000, std::ref(succ));
    printf_raw("[TEST] out: value=%s, success=%s\r\n", b ? "true" : "false", succ ? "true" : "false");
    done("get_yn false");

    // BOUNDARIES
    banner("get_int out-of-range -> default");
    printf_raw("[TEST] in : prompt=\"int-range?\", default=77, range=[0..10], retries=1, timeout=100, next_line=\"9999\"\r\n");
    feed_line("9999");
    succ = false;
    v = get_int("int-range?", 77, 0, 10, 1, 100, std::ref(succ));
    printf_raw("[TEST] out: value=%d, success=%s\r\n", v, succ ? "true" : "false");
    done("get_int out-of-range -> default");

    banner("get_string too short -> default");
    printf_raw("[TEST] in : prompt=\"str-len?\", default=\"DEF\", len=[3..5], retries=1, timeout=100, next_line=\"a\"\r\n");
    feed_line("a");
    succ = false;
    s = get_string("str-len?", "DEF", 3, 5, 1, 100, std::ref(succ));
    printf_raw("[TEST] out: value=\"%s\", success=%s\r\n", s.c_str(), succ ? "true" : "false");
    done("get_string too short -> default");

    banner("get_yn invalid -> default");
    printf_raw("[TEST] in : prompt=\"yn-bad?\", default=true, retries=1, timeout=100, next_line=\"maybe\"\r\n");
    feed_line("maybe");
    succ = false;
    b = get_yn("yn-bad?", true, 1, 100, std::ref(succ));
    printf_raw("[TEST] out: value=%s, success=%s\r\n", b ? "true" : "false", succ ? "true" : "false");
    done("get_yn invalid -> default");

    // SUMMARY
    banner("summary");
    printf_raw("[TEST] in : none\r\n");
    print_separator(16, '=', '+');
    print("done", '|', 'c', 10, 0, 0, kCRLF);
    print_separator(16, '=', '+');
    printf_raw("[TEST] out: printed\r\n");
    done("summary");
}

void SerialPort::loop() {
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());
        yield();  // allow RTOS/watchdog

        // Echo what user types (optional)
        Serial.write(static_cast<uint8_t>(c));

        if (c == '\r') continue; // normalize CRLF -> LF
        if (c == '\n' || input_buffer_pos >= INPUT_BUFFER_SIZE - 1) {
            input_buffer[input_buffer_pos] = '\0';
            line_length = input_buffer_pos;
            input_buffer_pos = 0;
            line_ready = true;
        } else {
            input_buffer[input_buffer_pos++] = c;
        }
    }
}

void SerialPort::reset (const bool verbose, const bool do_restart) {
    flush_input();
    input_buffer_pos = 0;
    line_length      = 0;
    line_ready       = false;
    Module::reset(verbose, do_restart);
}

// -------------------------------------------------------------------------------------------------
// RAW OUTPUT
// -------------------------------------------------------------------------------------------------
void SerialPort::print_raw(string_view message) {
    Serial.write(reinterpret_cast<const uint8_t*>(message.data()), message.size());
}

void SerialPort::println_raw(string_view message) {
    Serial.write(reinterpret_cast<const uint8_t*>(message.data()), message.size());
    Serial.write(reinterpret_cast<const uint8_t*>(kCRLF), 2);
}

void SerialPort::printf_raw(const char* fmt, ...) {
    if (!fmt) return;
    va_list ap;
    va_start(ap, fmt);

    va_list ap2;
    va_copy(ap2, ap);
    int needed = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    if (needed <= 0) { va_end(ap2); return; }

    std::vector<char> buf(static_cast<size_t>(needed) + 1u);
    vsnprintf(buf.data(), buf.size(), fmt, ap2);
    va_end(ap2);

    Serial.write(reinterpret_cast<const uint8_t*>(buf.data()), needed);
}

void SerialPort::print(string_view  message,
                       const char   edge_character,
                       const char   text_align,
                       const uint16_t message_width,
                       const uint16_t margin_l,
                       const uint16_t margin_r,
                       string_view  end) {
    // Split incoming by '\n', then optionally wrap each "line" to message_width.
    auto lines_sv = split_lines_sv(message, '\n');
    const bool use_wrap = (message_width > 0);

    // We'll print CRLF between lines; on the very last emission, use 'end'.
    for (size_t i = 0; i < lines_sv.size(); ++i) {
        std::string base_line(lines_sv[i]);
        rtrim_cr(base_line);

        std::vector<std::string> chunks = use_wrap
                                          ? wrap_fixed(base_line, message_width)
                                          : std::vector<std::string>{base_line};

        for (size_t j = 0; j < chunks.size(); ++j) {
            const bool is_last = (i == lines_sv.size() - 1) && (j == chunks.size() - 1);
            std::string out = compose_box_line(
                chunks[j], edge_character, message_width, margin_l, margin_r, text_align);

            Serial.write(reinterpret_cast<const uint8_t*>(out.data()), out.size());
            if (is_last) {
                if (!end.empty()) {
                    Serial.write(reinterpret_cast<const uint8_t*>(end.data()), end.size());
                }
            } else {
                Serial.write(reinterpret_cast<const uint8_t*>(kCRLF), 2);
            }
        }
    }
}

void SerialPort::printf(const char        edge_character,
                        const char        text_align,
                        const uint16_t    message_width,
                        const uint16_t    margin_l,
                        const uint16_t    margin_r,
                        const string_view end,
                        const char*       fmt,
                                           ...) {
    if (!fmt) {
        print("", edge_character, text_align, message_width, margin_l, margin_r, end);
        return;
    }

    va_list ap;
    va_start(ap, fmt);

    va_list ap2;
    va_copy(ap2, ap);
    int needed = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    std::string msg;
    if (needed > 0) {
        msg.resize(static_cast<size_t>(needed));
        vsnprintf(msg.data(), msg.size() + 1, fmt, ap2);
    }
    va_end(ap2);

    print(msg, edge_character, text_align, message_width, margin_l, margin_r, end);
}

void SerialPort::print_separator(const uint16_t total_width,
                                 const char     fill,
                                 const char     edge) {
    std::string line = make_rule_line(total_width, fill, edge);
    write_line_crlf(line);
}

void SerialPort::print_spacer(const uint16_t total_width,
                              const char     edge) {
    std::string line = make_spacer_line(total_width, edge);
    write_line_crlf(line);
}

void SerialPort::print_header(string_view  message,
                              const uint16_t total_width,
                              const char   edge,
                              const char   sep_edge,
                              const char   sep_fill) {
    // Top rule
    print_separator(total_width, sep_fill, sep_edge);

    // Center each part split by literal token "\sep"
    auto parts = split_by_token(message, "\\sep");
    const uint16_t content_width = (total_width >= 2) ? (total_width - 2) : total_width;
    for (auto& p : parts) {
        // Center inside message field (content_width), no extra margins.
        print(p, edge, 'c', content_width, 0, 0, kCRLF);
        print_separator(total_width, sep_fill, sep_edge);
    }
}

// -------------------------------------------------------------------------------------------------
// INPUT (line-based with retries/timeout/constraints)
// -------------------------------------------------------------------------------------------------
bool SerialPort::has_line() const { return line_ready; }

string SerialPort::read_line() {
    if (!line_ready) return {};
    string out(input_buffer, line_length);
    line_ready       = false;
    line_length      = 0;
    input_buffer_pos = 0;
    return out;
}

void SerialPort::flush_input() {
    while (Serial.available()) {
        (void)Serial.read();
        yield();
    }
    input_buffer_pos = 0;
    line_length      = 0;
    line_ready       = false;
}

bool SerialPort::read_line_with_timeout(string& out,
                                        const uint32_t timeout_ms) {
    uint32_t start = millis();
    for (;;) {
        loop();
        if (has_line()) {
            out = read_line();
            return true;
        }
        if (timeout_ms != 0 && (millis() - start >= timeout_ms)) {
            return false;
        }
        yield();
    }
}

// Generic integral reader
template <typename T>
T SerialPort::get_integral(string_view prompt,
                           const T default_value,
                           const T min_value,
                           const T max_value,
                           const uint16_t retry_count,
                           const uint32_t timeout_ms,
                           optional<reference_wrapper<bool>> success_sink) {
    if (!prompt.empty()) println_raw(prompt);

    auto set_success = [&](bool ok){
        if (success_sink.has_value()) success_sink->get() = ok;
    };

    // Normalize constraints
    T minv = min_value;
    T maxv = max_value;
    if (minv > maxv) std::swap(minv, maxv);

    // Determine attempt policy
    uint32_t attempts = (retry_count == 0) ? std::numeric_limits<uint32_t>::max()
                                           : static_cast<uint32_t>(retry_count);

    for (uint32_t attempt = 0; attempt < attempts; ++attempt) {
        println_raw("> ");  // simple input marker

        string line;
        bool got = read_line_with_timeout(line, timeout_ms);
        if (!got) {
            println_raw("! Timeout.");
            if (retry_count != 0 && (attempt + 1) >= attempts) {
                set_success(false);
                return default_value;
            }
            continue;
        }

        // Parse and enforce range
        T value{};
        if (!parse_int<T>(line, value)) {
            println_raw("! Invalid number. Please enter a base-10 integer.");
            if (retry_count != 0 && (attempt + 1) >= attempts) {
                set_success(false);
                return default_value;
            }
            continue;
        }
        if (value < minv || value > maxv) {
            printf_raw("! Out of range [%lld..%lld].\r\n",
                       static_cast<long long>(minv),
                       static_cast<long long>(maxv));
            if (retry_count != 0 && (attempt + 1) >= attempts) {
                set_success(false);
                return default_value;
            }
            continue;
        }

        set_success(true);
        return value;
    }

    // Only reached with retry_count == 0 and timeout_ms == 0 (infinite attempts).
    set_success(false);
    return default_value;
}

// Signed int (platform width)
int SerialPort::get_int(string_view prompt,
                        const int default_value,
                        const int min_value,
                        const int max_value,
                        const uint16_t retry_count,
                        const uint32_t timeout_ms,
                        optional<reference_wrapper<bool>> success_sink) {
    return get_integral<int>(prompt, default_value, min_value, max_value,
                             retry_count, timeout_ms, success_sink);
}

uint8_t SerialPort::get_uint8(string_view prompt,
                              const uint8_t default_value,
                              const uint8_t min_value,
                              const uint8_t max_value,
                              const uint16_t retry_count,
                              const uint32_t timeout_ms,
                              optional<reference_wrapper<bool>> success_sink) {
    return get_integral<uint8_t>(prompt, default_value, min_value, max_value,
                                 retry_count, timeout_ms, success_sink);
}

uint16_t SerialPort::get_uint16(string_view prompt,
                                const uint16_t default_value,
                                const uint16_t min_value,
                                const uint16_t max_value,
                                const uint16_t retry_count,
                                const uint32_t timeout_ms,
                                optional<reference_wrapper<bool>> success_sink) {
    return get_integral<uint16_t>(prompt, default_value, min_value, max_value,
                                  retry_count, timeout_ms, success_sink);
}

uint32_t SerialPort::get_uint32(string_view prompt,
                                const uint32_t default_value,
                                const uint32_t min_value,
                                const uint32_t max_value,
                                const uint16_t retry_count,
                                const uint32_t timeout_ms,
                                optional<reference_wrapper<bool>> success_sink) {
    return get_integral<uint32_t>(prompt, default_value, min_value, max_value,
                                  retry_count, timeout_ms, success_sink);
}

string SerialPort::get_string(string_view prompt,
                              string_view default_value,
                              const uint16_t min_length,
                              const uint16_t max_length,
                              const uint16_t retry_count,
                              const uint32_t timeout_ms,
                              optional<reference_wrapper<bool>> success_sink) {
    if (!prompt.empty()) println_raw(prompt);

    auto set_success = [&](bool ok){
        if (success_sink.has_value()) success_sink->get() = ok;
    };

    const size_t min_len = static_cast<size_t>(min_length);
    size_t max_len = (max_length == 0) ? (INPUT_BUFFER_SIZE - 1)
                                       : static_cast<size_t>(max_length);

    uint32_t attempts = (retry_count == 0) ? std::numeric_limits<uint32_t>::max()
                                           : static_cast<uint32_t>(retry_count);

    for (uint32_t attempt = 0; attempt < attempts; ++attempt) {
        println_raw("> ");

        string line;
        bool got = read_line_with_timeout(line, timeout_ms);
        if (!got) {
            println_raw("! Timeout.");
            if (retry_count != 0 && (attempt + 1) >= attempts) {
                set_success(false);
                return string(default_value);
            }
            continue;
        }

        if (line.size() < min_len || line.size() > max_len) {
            printf_raw("! Length must be in [%u..%u] chars.\r\n",
                       static_cast<unsigned>(min_len),
                       static_cast<unsigned>(max_len));
            if (retry_count != 0 && (attempt + 1) >= attempts) {
                set_success(false);
                return string(default_value);
            }
            continue;
        }

        set_success(true);
        return line;
    }

    set_success(false);
    return string(default_value);
}

bool SerialPort::get_yn(string_view prompt,
                        const bool default_value,
                        const uint16_t retry_count,
                        const uint32_t timeout_ms,
                        optional<reference_wrapper<bool>> success_sink) {
    if (!prompt.empty()) println_raw(prompt);

    auto set_success = [&](bool ok){
        if (success_sink.has_value()) success_sink->get() = ok;
    };

    uint32_t attempts = (retry_count == 0) ? std::numeric_limits<uint32_t>::max()
                                           : static_cast<uint32_t>(retry_count);

    for (uint32_t attempt = 0; attempt < attempts; ++attempt) {
        println_raw("(y/n) > ");

        string line;
        bool got = read_line_with_timeout(line, timeout_ms);
        if (!got) {
            println_raw("! Timeout.");
            if (retry_count != 0 && (attempt + 1) >= attempts) {
                set_success(false);
                return default_value;
            }
            continue;
        }

        std::string low = to_lower(line);
        if (low == "y" || low == "yes" || low == "1" || low == "true") {
            set_success(true);
            return true;
        }
        if (low == "n" || low == "no" || low == "0" || low == "false") {
            set_success(true);
            return false;
        }

        println_raw("! Please answer 'y' or 'n'.");
        if (retry_count != 0 && (attempt + 1) >= attempts) {
            set_success(false);
            return default_value;
        }
    }

    set_success(false);
    return default_value;
}

void SerialPort::write_line_crlf(string_view s) {
    Serial.write(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    Serial.write(reinterpret_cast<const uint8_t*>(kCRLF), 2);
}
