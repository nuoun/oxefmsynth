/*
Oxe FM Synth: a software synthesizer
Copyright (C) 2004-2015  Daniel Moura <oxe@oxesoft.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "public.sdk/source/common/pluginview.h"
#include "editor.h"
#include "vsthostinterface3.h"

namespace Steinberg {
namespace Vst {

class OxeVst3;

//------------------------------------------------------------------------
class OxeVst3Editor : public CPluginView
{
public:
    OxeVst3Editor(OxeVst3* effect, CSynthesizer* synth);
    ~OxeVst3Editor() SMTG_OVERRIDE;

    //--- from IPlugView ---
    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) SMTG_OVERRIDE;
    tresult PLUGIN_API attached(void* parent, FIDString type) SMTG_OVERRIDE;
    tresult PLUGIN_API removed() SMTG_OVERRIDE;
    tresult PLUGIN_API getSize(ViewRect* size) SMTG_OVERRIDE;

    //---Interface------
    OBJ_METHODS(OxeVst3Editor, CPluginView)
    DEFINE_INTERFACES
        DEF_INTERFACE(IPlugView)
    END_DEFINE_INTERFACES(CPluginView)
    REFCOUNT_METHODS(CPluginView)

private:
    OxeVst3*            effectx;
    CEditor*            oxeeditor;
    CToolkit*           toolkit;
    CVstHostInterface3* hostinterface;
};

} // namespace Vst
} // namespace Steinberg
