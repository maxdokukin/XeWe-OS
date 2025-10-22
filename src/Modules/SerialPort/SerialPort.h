/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/
#pragma once

#include "../../Module/Module.h"
#include <string>
#include <string_view>
#include <optional>
#include <functional>
#include <cstdint>

struct SerialPortConfig : public ModuleConfig {
    unsigned long baud_rate = 9600;
};

class SerialPort : public Module {
public:
    explicit SerialPort(ModuleController& controller);

    // Module lifecycle
    void begin_routines_required(const ModuleConfig& cfg) override;
    void loop() override;
    void reset(const bool verbose=false, const bool do_restart=true) override;

    void print(string_view message = {},
               char edge_character = '|',
               char text_align     = 'l',     // 'l', 'r', 'c'
               uint16_t message_width = 0,    // 0 => no target width (no wrapping/padding field)
               uint16_t margin_l      = 0,
               uint16_t margin_r      = 0,
               string_view end        = xewe::str::kCRLF);

    void printf(char edge_character,
                char text_align,
                uint16_t message_width,
                uint16_t margin_l,
                uint16_t margin_r,
                string_view end,
                const char* fmt, ...);

    void print_separator(uint16_t total_width = 50, char fill = '-', char edge = '+');
    void print_spacer(uint16_t total_width = 50, char edge = '|');
    void print_header(string_view message,
                      uint16_t total_width = 50,
                      char edge = '|',
                      char sep_edge = '+',
                      char sep_fill = '-');

    string get_string(string_view prompt = {},
                      string default_value = {},
                      size_t min_length = 0,
                      size_t max_length = 0,          // 0 => default to input buffer max
                      uint16_t retry_count = 1,
                      uint32_t timeout_ms = 10000,
                      optional<reference_wrapper<bool>> success_sink = nullopt);

    int get_int(string_view prompt = {},
                int default_value = 0,
                int min_value = numeric_limits<int>::min(),
                int max_value = numeric_limits<int>::max(),
                uint16_t retry_count = 1,
                uint32_t timeout_ms = 10000,
                optional<reference_wrapper<bool>> success_sink = nullopt);

    uint8_t  get_uint8 (string_view prompt = {}, uint8_t  default_value = 0,
                        uint8_t  min_value = numeric_limits<uint8_t>::min(),
                        uint8_t  max_value = numeric_limits<uint8_t>::max(),
                        uint16_t retry_count = 1, uint32_t timeout_ms = 10000,
                        optional<reference_wrapper<bool>> success_sink = nullopt);

    uint16_t get_uint16(string_view prompt = {}, uint16_t default_value = 0,
                        uint16_t min_value = numeric_limits<uint16_t>::min(),
                        uint16_t max_value = numeric_limits<uint16_t>::max(),
                        uint16_t retry_count = 1, uint32_t timeout_ms = 10000,
                        optional<reference_wrapper<bool>> success_sink = nullopt);

    uint32_t get_uint32(string_view prompt = {}, uint32_t default_value = 0,
                        uint32_t min_value = numeric_limits<uint32_t>::min(),
                        uint32_t max_value = numeric_limits<uint32_t>::max(),
                        uint16_t retry_count = 1, uint32_t timeout_ms = 10000,
                        optional<reference_wrapper<bool>> success_sink = nullopt);

    bool get_yn(string_view prompt = {},
                bool default_value = false,
                uint16_t retry_count = 1,
                uint32_t timeout_ms = 10000,
                optional<reference_wrapper<bool>> success_sink = nullopt);

    bool   has_line() const;
    string read_line();

private:
    // Input buffering state
    size_t input_buffer_pos         = 0;
    size_t line_length              = 0;
    bool   line_ready               = false;
    static constexpr size_t INPUT_BUFFER_SIZE = 256;
    char   input_buffer[INPUT_BUFFER_SIZE];

    void flush_input();
    void print_raw(string_view message);
    void println_raw(string_view message);
    void printf_raw(const char* fmt, ...);

    bool read_line_with_timeout(string& out, uint32_t timeout_ms);
    void write_line_crlf(string_view s); // write s + CRLF

    template <typename T>
    T get_integral(string_view prompt, T default_value, T min_value, T max_value,
                   uint16_t retry_count, uint32_t timeout_ms,
                   optional<reference_wrapper<bool>> success_sink);
};
