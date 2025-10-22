/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/
// src/Modules/Software/System/System.cpp

#include "System.h"
#include "../../ModuleController/ModuleController.h"


System::System(ModuleController& controller)
      : Module(controller,
               /* module_name         */ "System",
               /* module_description  */ "Stores integral commands and routines",
               /* nvs_key             */ "sys",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ true) {
    commands_storage.push_back({
        "restart",
        "Restart the ESP",
        string("Sample Use: $") + lower(module_name) + " restart",
        0,
        [this](string_view args) { ESP.restart(); }
    });
    commands_storage.push_back({
        "reboot",
        "Restart the ESP",
        string("Sample Use: $") + lower(module_name) + " reboot",
        0,
        [this](string_view args) { ESP.restart(); }
    });
}

void System::begin_routines_required (const ModuleConfig& cfg) {
    controller.serial_port.print_header(
        string("XeWe OS") + "\\sep" +
        "Lightweight ESP32 OS" + "\\sep" +
        "https://github.com/maxdokukin/XeWe-OS" + "\\sep" +
        "Version " + TO_STRING(BUILD_VERSION) + "\n" +
        "Build Timestamp " + TO_STRING(BUILD_TIMESTAMP)
    );
}