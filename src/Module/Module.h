/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/
// src/Modules/Module/Module.h
#pragma once

#include <functional>
#include <span>
#include <utility>
#include <vector>

#include "../Config.h"
#include "../Debug.h"
#include "../XeWeStringUtils.h"

using namespace std;
using namespace xewe::str;

class ModuleController;

class ModuleConfig {
public:
    ModuleConfig                                            ()                              = default;
    virtual ~ModuleConfig                                   () noexcept                     = default;
    ModuleConfig                                            (const ModuleConfig&)           = default;
    ModuleConfig& operator=                                 (const ModuleConfig&)           = default;
    ModuleConfig                                            (ModuleConfig&&) noexcept       = default;
    ModuleConfig& operator=                                 (ModuleConfig&&) noexcept       = default;
};

using command_function_t = function<void(string args)>;

struct Command {
    string                 name;
    string                 description;
    string                 sample_usage;
    size_t                 arg_count;
    command_function_t          function;
};

struct CommandsGroup {
    string                 name;
    string                 group;
    span<const Command>    commands;
};

class Module {
public:
    Module(ModuleController&    controller,
           string               module_name,
           string               module_description,
           string               nvs_key,
           bool                 requires_init_setup,
           bool                 can_be_disabled,
           bool                 has_cli_commands)
      : controller              (controller)
      , module_name             (move(module_name))
      , module_description      (move(module_description))
      , nvs_key                 (move(nvs_key))
      , requires_init_setup     (requires_init_setup)
      , can_be_disabled         (can_be_disabled)
      , has_cli_commands        (has_cli_commands)
      , enabled                 (true)
    {
        if (has_cli_commands)
            register_generic_commands();
    }

    virtual ~Module                                         () noexcept                     = default;

    Module                                                  (const Module&)                 = delete;
    Module& operator=                                       (const Module&)                 = delete;
    Module                                                  (Module&&)                      = delete;
    Module& operator=                                       (Module&&)                      = delete;

    virtual void                begin                       (const ModuleConfig& cfg);
    virtual void                begin_routines_required     (const ModuleConfig& cfg);
    virtual void                begin_routines_init         (const ModuleConfig& cfg);
    virtual void                begin_routines_regular      (const ModuleConfig& cfg);
    virtual void                begin_routines_common       (const ModuleConfig& cfg);

    virtual void                loop                        ();

    virtual void                enable                      (const bool verbose=false,
                                                             const bool do_restart=true);
    virtual void                disable                     (const bool verbose=false,
                                                             const bool do_restart=true);
    virtual void                reset                       (const bool verbose=false,
                                                             const bool do_restart=true);

    virtual string              status                      (const bool verbose=false)      const;
    virtual bool                is_enabled                  (const bool verbose=false)      const;
    virtual bool                is_disabled                 (const bool verbose=false)      const;
    virtual bool                init_setup_complete         (const bool verbose=false)      const;

    virtual void                add_requirement             (Module& other);

    CommandsGroup               get_commands_group          ();
    string_view                 get_module_name             ()                              const { return module_name; };

protected:
    ModuleController&           controller;
    string                      module_name;
    string                      module_description;
    string                      nvs_key;

    bool                        can_be_disabled;
    bool                        requires_init_setup;
    bool                        has_cli_commands;

    bool                        enabled;

    vector<Command>             commands_storage;
    CommandsGroup               commands_group;
    void                        register_generic_commands   ();

    void                        run_with_dots               (const function<void()>& work,
                                                             uint32_t duration_ms=1000,
                                                             uint32_t dot_interval_ms=200);

    bool                        requirements_enabled        (const bool verbose=false)      const;

private:
    vector<Module*>             required_modules;
    vector<Module*>             dependent_modules;
};
