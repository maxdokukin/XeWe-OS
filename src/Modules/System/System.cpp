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
//    // info
//    commands_storage.push_back({
//      "info","Chip and build info",
//      string("Sample Use: $")+lower(module_name)+" info",0,
//      [this](string_view){
//        esp_chip_info_t ci; esp_chip_info(&ci);
//        uint32_t flash_sz=0, flash_spd=0; esp_flash_t* chip=NULL;
//        esp_flash_init(chip); esp_flash_get_size(chip,&flash_sz); esp_flash_get_speed(chip,&flash_spd);
//        uint8_t mac[6]; esp_read_mac(mac, ESP_MAC_WIFI_STA);
//        controller.serial_port.printf(
//          "Model %d  Cores %d  Rev %d\nIDF %s\nFlash %u bytes @ %u Hz\nMAC %02X:%02X:%02X:%02X:%02X:%02X\n",
//          ci.model, ci.cores, ci.revision, esp_get_idf_version(),
//          (unsigned)flash_sz, (unsigned)flash_spd,
//          mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
//      }
//    });

//    // uptime
//    commands_storage.push_back({
//      "uptime","Millis since boot",
//      string("Sample Use: $")+lower(module_name)+" uptime",0,
//      [this](string_view){
//        uint64_t us=esp_timer_get_time();
//        uint64_t ms=us/1000;
//        controller.serial_port.printf("uptime_ms %llu\n",(unsigned long long)ms);
//      }
//    });
//
//    // heap
//    commands_storage.push_back({
//      "heap","Heap stats",
//      string("Sample Use: $")+lower(module_name)+" heap",0,
//      [this](string_view){
//        size_t free8=heap_caps_get_free_size(MALLOC_CAP_8BIT);
//        size_t min8 =heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
//        size_t big8 =heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
//        controller.serial_port.printf("8bit free %u min %u largest %u\n",(unsigned)free8,(unsigned)min8,(unsigned)big8);
//    #ifdef MALLOC_CAP_SPIRAM
//        size_t frees=heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
//        size_t mins =heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
//        size_t bigs =heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
//        controller.serial_port.printf("psram free %u min %u largest %u\n",(unsigned)frees,(unsigned)mins,(unsigned)bigs);
//    #endif
//      }
//    });
//
//    // tasks
//    commands_storage.push_back({
//      "tasks","FreeRTOS task list",
//      string("Sample Use: $")+lower(module_name)+" tasks [N]",0,
//      [this](string_view){
//    #if (configUSE_TRACE_FACILITY==1) && (configUSE_STATS_FORMATTING_FUNCTIONS==1)
//        static char buf[2048];
//        vTaskList(buf);
//        controller.serial_port.print(buf);
//    #else
//        controller.serial_port.println("Enable configUSE_TRACE_FACILITY and configUSE_STATS_FORMATTING_FUNCTIONS");
//    #endif
//      }
//    });
//
//    // sleep
//    commands_storage.push_back({
//      "sleep","Deep sleep ms",
//      string("Sample Use: $")+lower(module_name)+" sleep 5000",1,
//      [this](string_view args){
//        uint32_t ms = strtoul(string(args).c_str(), nullptr, 10);
//        esp_sleep_enable_timer_wakeup((uint64_t)ms*1000ULL);
//        controller.serial_port.println("Deep sleeping...");
//        esp_deep_sleep_start();
//      }
//    });
//
//    // reset-reason
//    commands_storage.push_back({
//      "reset-reason","Last reset and wake cause",
//      string("Sample Use: $")+lower(module_name)+" reset-reason",0,
//      [this](string_view){
//        controller.serial_port.printf("reset %d  wake %d\n",
//          (int)esp_reset_reason(), (int)esp_sleep_get_wakeup_cause());
//      }
//    });
//
//    // loglevel
//    commands_storage.push_back({
//      "loglevel","Set ESP log level",
//      string("Sample Use: $")+lower(module_name)+" loglevel * info",2,
//      [this](string_view args){
//        auto s=string(args); auto sp=s.find(' ');
//        if(sp==string::npos){ controller.serial_port.println("need TAG LEVEL"); return; }
//        string tag=s.substr(0,sp), lvl=s.substr(sp+1);
//        esp_log_level_t l=ESP_LOG_INFO;
//        if(lvl=="none") l=ESP_LOG_NONE; else if(lvl=="error") l=ESP_LOG_ERROR;
//        else if(lvl=="warn") l=ESP_LOG_WARN; else if(lvl=="debug") l=ESP_LOG_DEBUG;
//        else if(lvl=="verbose") l=ESP_LOG_VERBOSE;
//        esp_log_level_set(tag=="*" ? "*" : tag.c_str(), l);
//      }
//    });
//
//    // time
//    commands_storage.push_back({
//      "time","Get or set RTC",
//      string("Sample Use: $")+lower(module_name)+" time 2025-10-22 12:34:56",0,
//      [this](string_view args){
//        if(args.empty()){
//          time_t now=time(nullptr); struct tm tm; localtime_r(&now,&tm);
//          char out[32]; strftime(out, sizeof(out), "%F %T", &tm);
//          controller.serial_port.printf("%s\n", out); return;
//        }
//        int Y,M,D,h,m,s; if(sscanf(string(args).c_str(), "%d-%d-%d %d:%d:%d",&Y,&M,&D,&h,&m,&s)==6){
//          struct tm tm={}; tm.tm_year=Y-1900; tm.tm_mon=M-1; tm.tm_mday=D; tm.tm_hour=h; tm.tm_min=m; tm.tm_sec=s;
//          struct timeval tv={.tv_sec=mktime(&tm), .tv_usec=0}; settimeofday(&tv,nullptr);
//        } else controller.serial_port.println("format YYYY-MM-DD HH:MM:SS");
//      }
//    });
//
//    // partitions
//    commands_storage.push_back({
//      "partitions","Partition table summary",
//      string("Sample Use: $")+lower(module_name)+" partitions",0,
//      [this](string_view){
//        const esp_partition_t* boot=esp_ota_get_boot_partition();
//        const esp_partition_t* run =esp_ota_get_running_partition();
//        controller.serial_port.printf("boot %s  run %s\n", boot?boot->label:"?", run?run->label:"?");
//        esp_partition_iterator_t it=esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, nullptr);
//        while(it){ auto p=esp_partition_get(it);
//          controller.serial_port.printf("APP %-8s addr 0x%06x size %u\n", p->label, p->address, p->size);
//          it=esp_partition_next(it);
//        }
//        esp_partition_iterator_release(it);
//      }
//    });
//
//    // ota
//    commands_storage.push_back({
//      "ota","OTA status/control",
//      string("Sample Use: $")+lower(module_name)+" ota status|mark-valid|rollback",1,
//      [this](string_view args){
//        string a(args);
//        const esp_partition_t* run=esp_ota_get_running_partition();
//        if(a=="status"){
//          esp_ota_img_states_t st; esp_ota_get_state_partition(run,&st);
//          controller.serial_port.printf("running %s  state %d\n", run?run->label:"?", (int)st);
//        } else if(a=="mark-valid"){
//          esp_ota_mark_app_valid_cancel_rollback(); controller.serial_port.println("marked valid");
//        } else if(a=="rollback"){
//          esp_ota_mark_app_invalid_rollback_and_reboot();
//        } else controller.serial_port.println("status|mark-valid|rollback");
//      }
//    });

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