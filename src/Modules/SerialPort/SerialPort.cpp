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

using xewe::str::kCRLF;

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
             /* has_cli_cmds        */ false) {}

void SerialPort::begin_routines_required (const ModuleConfig& cfg) {
    const auto& config = static_cast<const SerialPortConfig&>(cfg);
    Serial.setTxBufferSize(2048);
    Serial.setRxBufferSize(1024);
    Serial.begin(config.baud_rate);
    delay(1000);
}

void SerialPort::loop () {
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
void SerialPort::print_raw(string_view msg) {
    Serial.write(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
}
void SerialPort::println_raw(string_view msg) {
    Serial.write(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
    Serial.write(reinterpret_cast<const uint8_t*>(kCRLF), 2);
}
void SerialPort::printf_raw(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string out = xewe::str::vformat(fmt, ap);
    va_end(ap);
    if (!out.empty()) {
        Serial.write(reinterpret_cast<const uint8_t*>(out.data()), out.size());
    }
}

void SerialPort::print(string_view message,
                       char edge_character,
                       char text_align,
                       uint16_t message_width,
                       uint16_t margin_l,
                       uint16_t margin_r,
                       string_view end) {
    // Split incoming by '\n', then optionally wrap each "line" to message_width.
    auto lines_sv = xewe::str::split_lines_sv(message, '\n');
    const bool use_wrap = (message_width > 0);

    // We'll print CRLF between lines; on the very last emission, use 'end'.
    for (size_t i = 0; i < lines_sv.size(); ++i) {
        std::string base_line(lines_sv[i]);
        xewe::str::rtrim_cr(base_line);

        std::vector<std::string> chunks = use_wrap
                                          ? xewe::str::wrap_fixed(base_line, message_width)
                                          : std::vector<std::string>{base_line};

        for (size_t j = 0; j < chunks.size(); ++j) {
            const bool is_last = (i == lines_sv.size() - 1) && (j == chunks.size() - 1);
            std::string out = xewe::str::compose_box_line(
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

//void SerialPort::printf(const char* fmt, ...) {
//    // Raw, unboxed printf (kept for convenience)
//    va_list ap;
//    va_start(ap, fmt);
//    std::string out = xewe::str::vformat(fmt, ap);
//    va_end(ap);
//    if (!out.empty()) {
//        Serial.write(reinterpret_cast<const uint8_t*>(out.data()), out.size());
//    }
//}

void SerialPort::printf(char edge_character,
                        char text_align,
                        uint16_t message_width,
                        uint16_t margin_l,
                        uint16_t margin_r,
                        string_view end,
                        const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string msg = xewe::str::vformat(fmt, ap);
    va_end(ap);
    print(msg, edge_character, text_align, message_width, margin_l, margin_r, end);
}

void SerialPort::print_separator(uint16_t total_width, char fill, char edge) {
    std::string line = xewe::str::make_rule_line(total_width, fill, edge);
    write_line_crlf(line);
}

void SerialPort::print_spacer(uint16_t total_width, char edge) {
    std::string line = xewe::str::make_spacer_line(total_width, edge);
    write_line_crlf(line);
}

void SerialPort::print_header(string_view message,
                              uint16_t total_width,
                              char edge,
                              char sep_edge,
                              char sep_fill) {
    // Top rule
    print_separator(total_width, sep_fill, sep_edge);

    // Center each part split by literal token "\sep"
    auto parts = xewe::str::split_by_token(message, "\\sep");
    const uint16_t content_width = (total_width >= 2) ? (total_width - 2) : total_width;
    for (auto& p : parts) {
        // Center inside message field (content_width), no extra margins.
        print(p, edge, 'c', content_width, 0, 0, kCRLF);
    }

    // Bottom rule
    print_separator(total_width, sep_fill, sep_edge);
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

bool SerialPort::read_line_with_timeout(string& out, uint32_t timeout_ms) {
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
T SerialPort::get_integral(string_view prompt, T default_value, T min_value, T max_value,
                           uint16_t retry_count, uint32_t timeout_ms,
                           std::optional<std::reference_wrapper<bool>> success_sink) {
    if (!prompt.empty()) println_raw(prompt);

    auto set_success = [&](bool ok){
        if (success_sink.has_value()) success_sink->get() = ok;
    };

    // Normalize constraints
    if (min_value > max_value) std::swap(min_value, max_value);

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
        if (!xewe::str::parse_int<T>(line, value)) {
            println_raw("! Invalid number. Please enter a base-10 integer.");
            if (retry_count != 0 && (attempt + 1) >= attempts) {
                set_success(false);
                return default_value;
            }
            continue;
        }
        if (value < min_value || value > max_value) {
            printf_raw("! Out of range [%lld..%lld].\r\n",
                       static_cast<long long>(min_value),
                       static_cast<long long>(max_value));
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
                        int default_value,
                        int min_value,
                        int max_value,
                        uint16_t retry_count,
                        uint32_t timeout_ms,
                        std::optional<std::reference_wrapper<bool>> success_sink) {
    return get_integral<int>(prompt, default_value, min_value, max_value,
                             retry_count, timeout_ms, success_sink);
}

uint8_t SerialPort::get_uint8(string_view prompt, uint8_t default_value,
                              uint8_t min_value, uint8_t max_value,
                              uint16_t retry_count, uint32_t timeout_ms,
                              std::optional<std::reference_wrapper<bool>> success_sink) {
    return get_integral<uint8_t>(prompt, default_value, min_value, max_value,
                                 retry_count, timeout_ms, success_sink);
}

uint16_t SerialPort::get_uint16(string_view prompt, uint16_t default_value,
                                uint16_t min_value, uint16_t max_value,
                                uint16_t retry_count, uint32_t timeout_ms,
                                std::optional<std::reference_wrapper<bool>> success_sink) {
    return get_integral<uint16_t>(prompt, default_value, min_value, max_value,
                                  retry_count, timeout_ms, success_sink);
}

uint32_t SerialPort::get_uint32(string_view prompt, uint32_t default_value,
                                uint32_t min_value, uint32_t max_value,
                                uint16_t retry_count, uint32_t timeout_ms,
                                std::optional<std::reference_wrapper<bool>> success_sink) {
    return get_integral<uint32_t>(prompt, default_value, min_value, max_value,
                                  retry_count, timeout_ms, success_sink);
}

string SerialPort::get_string(string_view prompt,
                              string default_value,
                              size_t min_length,
                              size_t max_length,
                              uint16_t retry_count,
                              uint32_t timeout_ms,
                              std::optional<std::reference_wrapper<bool>> success_sink) {
    if (!prompt.empty()) println_raw(prompt);

    auto set_success = [&](bool ok){
        if (success_sink.has_value()) success_sink->get() = ok;
    };

    if (max_length == 0) max_length = INPUT_BUFFER_SIZE - 1; // "physical" default

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
                return default_value;
            }
            continue;
        }

        if (line.size() < min_length || line.size() > max_length) {
            printf_raw("! Length must be in [%u..%u] chars.\r\n",
                       static_cast<unsigned>(min_length),
                       static_cast<unsigned>(max_length));
            if (retry_count != 0 && (attempt + 1) >= attempts) {
                set_success(false);
                return default_value;
            }
            continue;
        }

        set_success(true);
        return line;
    }

    set_success(false);
    return default_value;
}

bool SerialPort::get_yn(string_view prompt,
                        bool default_value,
                        uint16_t retry_count,
                        uint32_t timeout_ms,
                        std::optional<std::reference_wrapper<bool>> success_sink) {
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

        std::string low = xewe::str::to_lower(line);
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