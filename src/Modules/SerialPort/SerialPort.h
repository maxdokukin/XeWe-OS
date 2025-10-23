/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/
// src/Modules/Serialport/Serialport.h
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
    explicit                    SerialPort                  (ModuleController& controller);

    void                        begin_routines_required     (const ModuleConfig&    cfg)           override;
    void                        begin_routines_init         (const ModuleConfig&    cfg)           override;
    void                        loop                        ()                                     override;
    void                        print                       (string_view            message         = {},
                                                             const char             edge_character  = '|',
                                                             const char             text_align      = 'l',
                                                             const uint16_t         message_width   = 0,
                                                             const uint16_t         margin_l        = 0,
                                                             const uint16_t         margin_r        = 0,
                                                             string_view            end             = kCRLF
                                                            );
    void                        printf                      (const char             edge_character,
                                                             const char             text_align,
                                                             const uint16_t         message_width,
                                                             const uint16_t         margin_l,
                                                             const uint16_t         margin_r,
                                                             const string_view      end,
                                                             const char*            fmt,
                                                                                    ...
                                                            );
    void                        print_separator             (const uint16_t         total_width     = 50,
                                                             const char             fill            = '-',
                                                             const char             edge            = '+'
                                                            );
    void                        print_spacer                (const uint16_t         total_width     = 50,
                                                             const char             edge            = '|'
                                                            );
    void                        print_header                (string_view            message,
                                                             const uint16_t         total_width     = 50,
                                                             const char             edge            = '|',
                                                             const char             sep_edge        = '+',
                                                             const char             sep_fill        = '-'
                                                            );
    string                      get_string                  (string_view            prompt          = {},
                                                             string_view            default_value   = {},
                                                             const uint16_t         min_length      = 0,
                                                             const uint16_t         max_length      = 0,
                                                             const uint16_t         retry_count     = 1,
                                                             const uint32_t         timeout_ms      = 10000,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    int                         get_int                     (string_view            prompt          = {},
                                                             const int              default_value   = 0,
                                                             const int              min_value       = numeric_limits<int>::min(),
                                                             const int              max_value       = numeric_limits<int>::max(),
                                                             const uint16_t         retry_count     = 1,
                                                             const uint32_t         timeout_ms      = 10000,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    uint8_t                     get_uint8                   (string_view            prompt          = {},
                                                             const uint8_t          default_value   = 0,
                                                             const uint8_t          min_value       = numeric_limits<uint8_t>::min(),
                                                             const uint8_t          max_value       = numeric_limits<uint8_t>::max(),
                                                             const uint16_t         retry_count     = 1,
                                                             const uint32_t         timeout_ms      = 10000,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    uint16_t                    get_uint16                  (string_view            prompt          = {},
                                                             const uint16_t         default_value   = 0,
                                                             const uint16_t         min_value       = numeric_limits<uint16_t>::min(),
                                                             const uint16_t         max_value       = numeric_limits<uint16_t>::max(),
                                                             const uint16_t         retry_count     = 1,
                                                             const uint32_t         timeout_ms      = 10000,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    uint32_t                    get_uint32                  (string_view            prompt          = {},
                                                             const uint32_t         default_value   = 0,
                                                             const uint32_t         min_value       = numeric_limits<uint32_t>::min(),
                                                             const uint32_t         max_value       = numeric_limits<uint32_t>::max(),
                                                             const uint16_t         retry_count     = 1,
                                                             const uint32_t         timeout_ms      = 10000,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );
    bool                        get_yn                      (string_view            prompt          = {},
                                                             const bool             default_value   = false,
                                                             const uint16_t         retry_count     = 1,
                                                             const uint32_t         timeout_ms      = 10000,
                                                             optional<reference_wrapper<bool>> success_sink = nullopt
                                                            );

    bool                        has_line                    ()                                      const;
    string                      read_line                   ();

private:
    void                        flush_input                 ();
    void                        print_raw                   (string_view message);
    void                        println_raw                 (string_view message);
    void                        printf_raw                  (const char* fmt, ...);

    bool                        read_line_with_timeout      (string& out,
                                                             const uint32_t timeout_ms
                                                             );
    void                        write_line_crlf             (string_view s);

    template <typename T>
    T                           get_integral                (string_view prompt,
                                                             const T default_value,
                                                             const T min_value,
                                                             const T max_value,
                                                             const uint16_t retry_count,
                                                             const uint32_t timeout_ms,
                                                             optional<reference_wrapper<bool>> success_sink);

    size_t                      input_buffer_pos            = 0;
    size_t                      line_length                 = 0;
    bool                        line_ready                  = false;
    static constexpr size_t     INPUT_BUFFER_SIZE           = 255;
    char                        input_buffer                [INPUT_BUFFER_SIZE];
};
