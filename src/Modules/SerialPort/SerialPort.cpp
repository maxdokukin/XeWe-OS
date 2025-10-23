/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/
// src/Modules/SerialPort/SerialPort.cpp
#include "SerialPort.h"
#include "../../ModuleController/ModuleController.h"

#include <Arduino.h>
#include <cstdarg>
#include <cstdio>
#include <vector>
#include <limits>
#include <type_traits>
#include <cstdlib>   // strtod

using std::string;
using std::string_view;
using std::optional;
using std::reference_wrapper;

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
             /* has_cli_cmds        */ false) {
    commands_storage.push_back({
        "test",
        "test available functions",
        string("Sample Use: $") + lower(module_name) + " test",
        0,
        [this](string_view) { test(); }
    });
}

void SerialPort::begin_routines_required(const ModuleConfig& cfg) {
    const auto& config = static_cast<const SerialPortConfig&>(cfg);
    Serial.setTxBufferSize(2048);
    Serial.setRxBufferSize(1024);
    Serial.begin(config.baud_rate);
    delay(1000);
}

void SerialPort::loop() {
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());
        yield();

        Serial.write(static_cast<uint8_t>(c));

        if (c == '\r') continue;
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

    bool has_spec = false;
    for (const char* p = fmt; *p; ++p) {
        if (*p == '%') {
            if (*(p + 1) == '%') { ++p; continue; }
            has_spec = true;
            break;
        }
    }
    if (!has_spec) {
        size_t n = strlen(fmt);
        if (n) Serial.write(reinterpret_cast<const uint8_t*>(fmt), n);
        return;
    }

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

    Serial.write(reinterpret_cast<const uint8_t*>(buf.data()), static_cast<size_t>(needed));
}

void SerialPort::print(string_view  message,
                       const char   edge_character,
                       const char   text_align,
                       const uint16_t message_width,
                       const uint16_t margin_l,
                       const uint16_t margin_r,
                       string_view  end) {
    auto lines_sv = split_lines_sv(message, '\n');
    const bool use_wrap = (message_width > 0);

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
    print_separator(total_width, sep_fill, sep_edge);

    auto parts = split_by_token(message, "\\sep");
    const uint16_t content_width = (total_width >= 2) ? (total_width - 2) : total_width;
    for (auto& p : parts) {
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

// -------------------------------------------------------------------------------------------------
// Generalized input core usage + getters
// -------------------------------------------------------------------------------------------------

// Integral
template <typename T>
T SerialPort::get_integral(string_view prompt,
                           const T min_value,
                           const T max_value,
                           const uint16_t retry_count,
                           const uint32_t timeout_ms,
                           const T default_value,
                           optional<reference_wrapper<bool>> success_sink) {
    T minv = min_value, maxv = max_value;
    if (minv > maxv) std::swap(minv, maxv);

    auto checker = [&](const string& line, T& out, const char*& err)->bool {
        T v{};
        if (!parse_int<T>(line, v)) {
            err = "! Invalid number. Please enter a base-10 integer.";
            return false;
        }
        if (v < minv || v > maxv) {
            printf_raw("! Out of range [%lld..%lld].\r\n",
                       static_cast<long long>(minv),
                       static_cast<long long>(maxv));
            err = nullptr;
            return false;
        }
        out = v;
        return true;
    };

    return get_core<T>(prompt, retry_count, timeout_ms, default_value,
                          success_sink, "> ", /*crlf*/false, checker);
}

// String
std::string SerialPort::get_string(string_view prompt,
                                   const uint16_t min_length,
                                   const uint16_t max_length,
                                   const uint16_t retry_count,
                                   const uint32_t timeout_ms,
                                   string_view default_value,
                                   optional<reference_wrapper<bool>> success_sink) {
    const size_t min_len = static_cast<size_t>(min_length);
    const size_t max_len = (max_length == 0) ? (INPUT_BUFFER_SIZE - 1)
                                             : static_cast<size_t>(max_length);

    auto checker = [&](const string& line, string& out, const char*& err)->bool {
        if (line.size() < min_len || line.size() > max_len) {
            printf_raw("! Length must be in [%u..%u] chars.\r\n",
                       static_cast<unsigned>(min_len),
                       static_cast<unsigned>(max_len));
            err = nullptr;
            return false;
        }
        out = line;
        return true;
    };

    return get_core<string>(prompt, retry_count, timeout_ms, string(default_value),
                               success_sink, "> ", /*crlf*/false, checker);
}

// Yes/No
bool SerialPort::get_yn(string_view prompt,
                        const uint16_t retry_count,
                        const uint32_t timeout_ms,
                        const bool default_value,
                        optional<reference_wrapper<bool>> success_sink) {
    auto checker = [&](const string& line, bool& out, const char*& err)->bool {
        std::string low = to_lower(line);
        if (low == "y" || low == "yes" || low == "1" || low == "true")  { out = true;  return true; }
        if (low == "n" || low == "no"  || low == "0" || low == "false") { out = false; return true; }
        err = "! Please answer 'y' or 'n'.";
        return false;
    };

    return get_core<bool>(prompt, retry_count, timeout_ms, default_value,
                             success_sink, "(y/n) > ", /*crlf*/false, checker);
}

// Float
float SerialPort::get_float(string_view prompt,
                            const float min_value,
                            const float max_value,
                            const uint16_t retry_count,
                            const uint32_t timeout_ms,
                            const float default_value,
                            optional<reference_wrapper<bool>> success_sink) {
    float minv = min_value, maxv = max_value;
    if (minv > maxv) std::swap(minv, maxv);

    auto checker = [&](const string& line, float& out, const char*& err)->bool {
        const char* s = line.c_str();
        char* end = nullptr;
        double dv = strtod(s, &end);
        while (end && *end == ' ') ++end;
        if (s == end || (end && *end != '\0')) {
            err = "! Invalid number. Please enter a decimal value.";
            return false;
        }
        if (dv != dv) { err = "! Invalid number."; return false; } // NaN
        float v = static_cast<float>(dv);
        if (v < minv || v > maxv) {
            printf_raw("! Out of range [%g..%g].\r\n",
                       static_cast<double>(minv),
                       static_cast<double>(maxv));
            err = nullptr;
            return false;
        }
        out = v;
        return true;
    };

    return get_core<float>(prompt, retry_count, timeout_ms, default_value,
                              success_sink, "> ", /*crlf*/false, checker);
}

void SerialPort::write_line_crlf(string_view s) {
    Serial.write(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    Serial.write(reinterpret_cast<const uint8_t*>(kCRLF), 2);
}

int SerialPort::get_int(std::string_view prompt,
                        const int min_value,
                        const int max_value,
                        const uint16_t retry_count,
                        const uint32_t timeout_ms,
                        const int default_value,
                        std::optional<std::reference_wrapper<bool>> success_sink) {
    return get_integral<int>(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink);
}

uint8_t SerialPort::get_uint8(std::string_view prompt,
                              const uint8_t min_value,
                              const uint8_t max_value,
                              const uint16_t retry_count,
                              const uint32_t timeout_ms,
                              const uint8_t default_value,
                              std::optional<std::reference_wrapper<bool>> success_sink) {
    return get_integral<uint8_t>(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink);
}

uint16_t SerialPort::get_uint16(std::string_view prompt,
                                const uint16_t min_value,
                                const uint16_t max_value,
                                const uint16_t retry_count,
                                const uint32_t timeout_ms,
                                const uint16_t default_value,
                                std::optional<std::reference_wrapper<bool>> success_sink) {
    return get_integral<uint16_t>(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink);
}

uint32_t SerialPort::get_uint32(std::string_view prompt,
                                const uint32_t min_value,
                                const uint32_t max_value,
                                const uint16_t retry_count,
                                const uint32_t timeout_ms,
                                const uint32_t default_value,
                                std::optional<std::reference_wrapper<bool>> success_sink) {
    return get_integral<uint32_t>(prompt, min_value, max_value, retry_count, timeout_ms, default_value, success_sink);
}
