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
               /* requires_init_setup */ true,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ true) {
    commands_storage.push_back({
        "restart",
        "Restart the ESP",
        string("Sample Use: $") + lower(module_name) + " restart",
        0,
        [this](string_view args) {
            DBG_PRINTLN(System, "System: 'restart' command issued. Rebooting now.");
            ESP.restart();
        }
    });
    commands_storage.push_back({
        "reboot",
        "Restart the ESP",
        string("Sample Use: $") + lower(module_name) + " reboot",
        0,
        [this](string_view args) {
            DBG_PRINTLN(System, "System: 'reboot' command issued. Rebooting now.");
            ESP.restart();
        }
    });
}


void System::begin_routines_required (const ModuleConfig& cfg) {
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("XeWe OS");
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered(string("Version ") + TO_STRING(BUILD_VERSION));
    controller.serial_port.print_centered("https://github.com/maxdokukin/XeWe-OS");
    controller.serial_port.print_centered(string("Build Timestamp ") + TO_STRING(BUILD_TIMESTAMP));
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("Lightweight ESP32 OS");
    controller.serial_port.print_spacer();
}

void System::begin_routines_init (const ModuleConfig& cfg) {
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("Initial Set Up Flow");
    controller.serial_port.print_centered("");
    controller.serial_port.print_centered("- Device Name                          ");
    controller.serial_port.print_centered("- LED Strip                            ");
    controller.serial_port.print_centered("- WiFi                                 ");
    controller.serial_port.print_centered("- Web Interface     REQUIRES WiFi      ");
    controller.serial_port.print_centered("- HomeKit           REQUIRES WiFi      ");
    controller.serial_port.print_centered("- Alexa             REQUIRES WiFi & Web");
    controller.serial_port.print_centered("- Buttons                              ");
    controller.serial_port.print_spacer();

    controller.serial_port.print_spacer();
    controller.serial_port.print_centered("Name Your Device");
    controller.serial_port.print_spacer();
    controller.serial_port.println("Set the name your device will proudly hold until");
    controller.serial_port.println("the last electron leaves it");
    controller.serial_port.println("Sample names: \"Desk Lights\" or \"Ceiling Lights\"");

    string device_name;
    bool confirmed = false;
    while (!confirmed) {
        device_name = controller.serial_port.get_string("Enter device name: ");
        confirmed = controller.serial_port.prompt_user_yn("Confirm name: " + device_name);
    }

    controller.nvs.write_str(nvs_key, "dname", device_name);
    controller.serial_port.get_string(
        "\nDevice name setup success!\n"
        "Press enter to continue"
    );

    DBG_PRINTLN(System, "System->init_setup(): Complete.");
}

string System::get_device_name () { return controller.nvs.read_str(nvs_key, "dname"); };
