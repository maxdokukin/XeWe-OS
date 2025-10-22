/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/
// <filepath from project root>
#pragma once

#include "../../Module/Module.h"


struct ModuleNameConfig : public ModuleConfig {};


class ModuleName : public Module {
public:
    explicit                    ModuleName                  (ModuleController& controller);

    // optional implementation
    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_init         (const ModuleConfig& cfg)       override;
    void                        begin_routines_regular      (const ModuleConfig& cfg)       override;
    void                        begin_routines_common       (const ModuleConfig& cfg)       override;

    void                        loop                        ()                              override;

    void                        enable                      (const bool verbose=false,
                                                             const bool do_restart=true)    override;
    void                        disable                     (const bool verbose=false,
                                                             const bool do_restart=true)    override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true)    override;

    string                      status                      (const bool verbose=false)      const override;

private:

};
