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


SerialPort::SerialPort(ModuleController& controller)
      : Module(controller,
               /* module_name         */ "Serial_Port",
               /* module_description  */ "Allows to send and receive text messages over the USB wire",
               /* nvs_key             */ "ser",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ false)
{}

void SerialPort::begin_routines_required (const ModuleConfig& cfg) {
    const auto& config = static_cast<const SerialPortConfig&>(cfg);
    Serial.setTxBufferSize(2048);
    Serial.setRxBufferSize(1024);
    Serial.begin(config.baud_rate);
    delay(1000);
}

void SerialPort::loop () {
    while (Serial.available()) {
        char c = Serial.read();
        yield();
        Serial.write(c);

        if (c == '\r') {
            continue;
        }
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

void SerialPort::print(string_view msg) {
    Serial.write(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
}

void SerialPort::println(string_view msg) {
    Serial.write(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
    Serial.write(reinterpret_cast<const uint8_t*>("\r\n"), 2); // CRLF is safest on serial terms
}

void SerialPort::printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // First, compute required length
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);
    if (needed <= 0) { va_end(args); return; }

    // Allocate exact size (+1 for terminator used by vsnprintf)
    string buf(static_cast<size_t>(needed), '\0');
    vsnprintf(buf.data(), buf.size() + 1, fmt, args);
    va_end(args);

    Serial.write(reinterpret_cast<const uint8_t*>(buf.data()), buf.size());
}

bool SerialPort::has_line() const {
    return line_ready;
}

string SerialPort::read_line() {
    if (!line_ready) {
        return string();
    }
    string sv(input_buffer, line_length);
    line_ready       = false;
    line_length      = 0;
    input_buffer_pos = 0;
    return sv;
}

string SerialPort::get_string(string_view prompt) {
    if (!prompt.empty()) print(prompt);
    while (!has_line()) loop();
    return read_line();
}

int SerialPort::get_int(string_view prompt) {
    string sv = get_string(prompt);
    while (sv.empty()) sv = get_string();
    char buf[32];
    size_t len = sv.copy(buf, sizeof(buf) - 1);
    buf[len] = '\0';
    return static_cast<int>(strtol(buf, nullptr, 10));
}

bool SerialPort::get_confirmation(string_view prompt) {
    println(prompt);
    print("(y/n): ");
    string sv = get_string();
    transform(sv.begin(), sv.end(), sv.begin(), ::tolower);
    return (sv == "y" || sv == "yes" || sv == "1" || sv == "true");
}

bool SerialPort::prompt_user_yn(string_view prompt, uint16_t timeout) {
    println(prompt);
    uint32_t start = millis();
    print("(y/n)?: ");
    while (timeout == 0 || (millis() - start < timeout)) {
        loop();
        if (has_line()) {
            string sv = read_line();
            if (!sv.empty()) {
                char c = static_cast<char>(tolower(sv[0]));
                if (c == 'y') return true;
                if (c == 'n') return false;
                print("(y/n)?: ");
            }
        }
    }
    println("Timeout!");
    return false;
}


void SerialPort::flush_input() {
    while (Serial.available()) {
        Serial.read();
        loop();
    }
}

void SerialPort::print_spacer (uint16_t total_width,
                                char major_character,
                                const string& edge_characters) {

//    println(xewe::str::generate_split_line(total_width, major_character, edge_characters));
}

void SerialPort::print_centered (string message,
                            uint16_t total_width,
                            const string& edge_characters) {
//    println(xewe::str::center_text(message, total_width, edge_characters));
}