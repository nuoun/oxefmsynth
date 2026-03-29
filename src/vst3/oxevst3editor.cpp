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

#include "oxevst3editor.h"
#include "oxevst3.h"
#include "ostoolkit.h"

#include "pluginterfaces/gui/iplugview.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
OxeVst3Editor::OxeVst3Editor(OxeVst3* effect, CSynthesizer* synth)
    : CPluginView(nullptr)
    , effectx(effect)
    , oxeeditor(nullptr)
    , toolkit(nullptr)
    , hostinterface(nullptr)
{
    oxeeditor = new CEditor(synth);

    // Set the fixed size the synth GUI expects
    rect.left   = 0;
    rect.top    = 0;
    rect.right  = GUI_WIDTH;
    rect.bottom = GUI_HEIGHT;
}

//------------------------------------------------------------------------
OxeVst3Editor::~OxeVst3Editor()
{
    delete oxeeditor;
    oxeeditor = nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3Editor::isPlatformTypeSupported(FIDString type)
{
#ifdef _WIN32
    if (strcmp(type, kPlatformTypeHWND) == 0)
        return kResultTrue;
#elif defined(__APPLE__)
    if (strcmp(type, kPlatformTypeNSView) == 0)
        return kResultTrue;
#elif defined(__linux__)
    if (strcmp(type, kPlatformTypeX11EmbedWindowID) == 0)
        return kResultTrue;
#endif
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3Editor::attached(void* parent, FIDString /*type*/)
{
    systemWindow = parent;

    hostinterface = new CVstHostInterface3(effectx);
    toolkit       = new COSToolkit(parent, oxeeditor);
    oxeeditor->SetToolkit(toolkit);
    oxeeditor->SetHostInterface(hostinterface);
    toolkit->StartWindowProcesses();

    return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3Editor::removed()
{
    oxeeditor->SetToolkit(nullptr);
    oxeeditor->SetHostInterface(nullptr);

    delete toolkit;
    toolkit = nullptr;

    delete hostinterface;
    hostinterface = nullptr;

    systemWindow = nullptr;
    return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3Editor::getSize(ViewRect* size)
{
    if (!size)
        return kResultFalse;
    *size = rect;
    return kResultTrue;
}

} // namespace Vst
} // namespace Steinberg
