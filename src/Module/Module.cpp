/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/
// src/Modules/Module/Module.cpp
#include "Module.h"
#include "../ModuleController/ModuleController.h"


void Module::begin (const ModuleConfig& cfg) {
    DBG_PRINTF(Module, "'%s'->begin(): Called.\n", module_name.c_str());
    if (requires_init_setup) {
        // controller.serial_port.print_spacer();
        // controller.serial_port.print_centered(capitalize(module_name) + " Setup");
        // controller.serial_port.print_spacer();
    }

    bool first_boot = !controller.nvs.read_bool(nvs_key, "not_first_boot");
    enabled = first_boot || controller.nvs.read_bool(nvs_key, "is_enabled");

    if (is_disabled(true)) return;

    if (!requirements_enabled(true)) {
        Serial.printf("%s requirements not enabled; skipping\n", module_name.c_str());
        enabled = false;
        controller.nvs.write_bool(nvs_key, "is_enabled", false);
        controller.nvs.write_bool(nvs_key, "not_first_boot", true);
        return;
    }

    begin_routines_required(cfg);

    if (first_boot) {
        if (can_be_disabled) {
            enabled = true;// controller.serial_port.prompt_user_yn(string("Would you like to enable ") + capitalize(module_name) + " module?\n" + module_description, 0);
            if (!enabled) {
                controller.nvs.write_bool(nvs_key, "is_enabled", false);
                controller.nvs.write_bool(nvs_key, "not_first_boot", true);
                return;
            }
        }
        controller.nvs.write_bool(nvs_key, "is_enabled", true);
        controller.nvs.write_bool(nvs_key, "not_first_boot", true);
    }

    if (!init_setup_complete()) {
        begin_routines_init(cfg);
        controller.nvs.write_bool(nvs_key, "init_complete", true);
    } else {
        begin_routines_regular(cfg);
    }

    begin_routines_common(cfg);
}

void Module::begin_routines_required(const ModuleConfig&) {}
void Module::begin_routines_init(const ModuleConfig&) {}
void Module::begin_routines_regular(const ModuleConfig&) { /* controller.serial_port.println(module_name + " setup complete");*/ }
void Module::begin_routines_common(const ModuleConfig&) {}

void Module::loop() {}

void Module::reset(const bool verbose, const bool do_restart) {
    controller.nvs.write_bool(nvs_key, "init_complete", false);
    controller.nvs.write_bool(nvs_key, "is_enabled", false);

    if (verbose) Serial.printf("%s module reset\n", module_name.c_str());
    if (do_restart) {
        ESP.restart();
        if (verbose) Serial.printf("Restarting...\n\n\n");
    }
}

// returns success of the operation
void Module::enable(const bool verbose, const bool do_restart) {
    DBG_PRINTF(Module, "'%s'->enable(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (is_enabled()){
        DBG_PRINTLN(Module, "enable(): Module is already enabled.");
        Serial.printf("%s module already enabled\n", module_name.c_str());
        return;
    }
    if (!requirements_enabled(true)) {
        Serial.printf("%s: requirements not enabled; enable them first\n", module_name.c_str());
        return;
    }
    enabled = true;
    DBG_PRINTLN(Module, "enable(): Writing 'is_enabled'=true to NVS.");
    controller.nvs.write_bool(nvs_key, "is_enabled", true);
    if (verbose) Serial.printf("%s module enabled. Restarting...\n\n\n", module_name.c_str());
    ESP.restart();
    return;
}

// returns success of the operation
void Module::disable(const bool verbose, const bool do_restart) {
    DBG_PRINTF(Module, "'%s'->disable(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (is_disabled()){
        if (verbose) Serial.printf("%s module already disabled\n", module_name.c_str());
        return;
    }
    if (!can_be_disabled) {
        if (verbose) Serial.printf("%s module can't be disabled\n", module_name.c_str());
        return;
    }
    if (!dependent_modules.empty()) {
        for (auto* m : dependent_modules) {
            if (verbose) Serial.printf("disabled %s module\n", m->module_name.c_str());
            m->reset(true, false); // reset with no verbose, and dont reboot
        }
    }
    if (verbose) {
        Serial.printf("%s module disabled.\n", module_name.c_str());
        reset(true, true);
    }
    return;
}

string Module::status(bool verbose) const {
    DBG_PRINTF(Module, "'%s'->status(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    string status_str = (module_name + " module " + (controller.nvs.read_bool(nvs_key, "is_enabled") ? "enabled" : "disabled"));
    DBG_PRINTF(Module, "status(): Generated status string: '%s'.\n", status_str.c_str());
    if (verbose) Serial.printf("%s\n", status_str.c_str());
    return status_str;
}

// only print the debug msg if true
bool Module::is_enabled(bool verbose) const {
    DBG_PRINTF(Module, "'%s'->is_enabled(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (can_be_disabled) {
        DBG_PRINTF(Module, "is_enabled(): Module can be disabled, read NVS 'is_enabled' flag as %s.\n", enabled ? "true" : "false");
        if (verbose && enabled) Serial.printf("%s module enabled\n", module_name.c_str());
        return enabled;
    }
    DBG_PRINTLN(Module, "is_enabled(): Module cannot be disabled, returning true by default.");
    return true;
}

// only print the debug msg if true
bool Module::is_disabled(bool verbose) const {
    DBG_PRINTF(Module, "'%s'->is_disabled(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (can_be_disabled) {
        if (verbose && !enabled) Serial.printf("%s module disabled; use $%s enable\n", module_name.c_str(), lower(module_name).c_str());
        return !enabled;
    }
    DBG_PRINTLN(Module, "is_disabled(): Module cannot be disabled, returning false by default.");
    return false;
}

bool Module::init_setup_complete (bool verbose) const {
    DBG_PRINTF(Module, "'%s'->init_setup_complete(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    bool stp_cmp_flag = controller.nvs.read_bool(nvs_key, "init_complete");
    bool result = !requires_init_setup || stp_cmp_flag;
    DBG_PRINTF(Module, "init_setup_complete(): requires_init_setup=%s, nvs 'stp_cmp' flag=%s. Final result=%s\n",
        requires_init_setup ? "true" : "false",
        stp_cmp_flag ? "true" : "false",
        result ? "true" : "false"
    );
    return result;
}

CommandsGroup Module::get_commands_group() {
    DBG_PRINTF(Module, "'%s'->get_commands_group(): Called.\n", module_name.c_str());
    commands_group.name     = module_name;
    commands_group.group     = lower(module_name);
    commands_group.commands = span<const Command>(
        commands_storage.data(),
        commands_storage.size()
    );
    DBG_PRINTF(Module, "get_commands_group(): Returning command group '%s' with %zu commands.\n", commands_group.name.c_str(), commands_storage.size());
    return commands_group;
}

void Module::register_generic_commands() {
    DBG_PRINTF(Module, "'%s'->register_generic_commands(): Called.\n", module_name.c_str());
    // “status” command
    DBG_PRINTLN(Module, "register_generic_commands(): Registering 'status' command.");
    commands_storage.push_back(Command{
        "status",
        "Get module status",
        string("Sample Use: $") + lower(module_name) + " status",
        0,
        [this](string) {
            status(true);
        }
    });

    // “reset” command
    DBG_PRINTLN(Module, "register_generic_commands(): Registering 'reset' command.");
    commands_storage.push_back(Command{
        "reset",
        "Reset the module",
        string("Sample Use: $") + lower(module_name) + " reset",
        0,
        [this](string) {
            reset(true);
        }
    });

    // “enable” / “disable” commands (if supported)
    if (can_be_disabled) {
        DBG_PRINTLN(Module, "register_generic_commands(): Module can be disabled, registering 'enable'/'disable' commands.");
        commands_storage.push_back(Command{
            "enable",
            "Enable this module",
            string("Sample Use: $") + lower(module_name) + " enable",
            0,
            [this](string) {
                enable(true);
            }
        });
        commands_storage.push_back(Command{
            "disable",
            "Disable this module",
            string("Sample Use: $") + lower(module_name) + " disable",
            0,
            [this](string) {
                disable(true);
            }
        });
    }
}

void Module::run_with_dots(const function<void()>& work, uint32_t duration_ms, uint32_t dot_interval_ms) {
  if (dot_interval_ms == 0) dot_interval_ms = 1;

  const uint32_t start = millis();
  uint32_t next = start;  // first dot at t=0

  while ((uint32_t)(millis() - start) < duration_ms) {
    work();  // run the target function

    const uint32_t now = millis();
    if ((int32_t)(now - next) >= 0) {
      // controller.serial_port.print(string_view{"."});

      // If we're late by multiple intervals, skip ahead (prevents dot bursts)
      const uint32_t late = now - next;
      const uint32_t intervals = 1u + (late / dot_interval_ms);
      next += intervals * dot_interval_ms;
    }
  }
  // controller.serial_port.print(string_view{"\n"});
}

void Module::add_requirement(Module& other) {
    required_modules.push_back(&other);
    other.dependent_modules.push_back(this);
}

bool Module::requirements_enabled(bool verbose) const {
    bool all_enabled = true;
    for (auto* r : required_modules) {
        bool req_enabled = r->is_enabled();
        all_enabled = all_enabled && req_enabled;
        if (!req_enabled && verbose)
            Serial.printf("%s requires %s module; use $%s enable\n", module_name.c_str(), r->module_name.c_str(), lower(r->module_name).c_str());
    }
    return all_enabled;
}
