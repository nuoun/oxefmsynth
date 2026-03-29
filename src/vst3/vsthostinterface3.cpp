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

#include "vsthostinterface3.h"
#include "oxevst3.h"

#include "pluginterfaces/vst/ivsteditcontroller.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

CVstHostInterface3::CVstHostInterface3(OxeVst3* effect)
    : effectx(effect)
{
}

void CVstHostInterface3::ReceiveMessageFromPlugin(unsigned int messageID,
                                                   unsigned int par1,
                                                   unsigned int par2)
{
    if (!effectx)
        return;

    IComponentHandler* handler = effectx->getComponentHandler();

    switch (messageID)
    {
        case UPDATE_DISPLAY:
        {
            effectx->syncAllParams();
            if (handler)
                handler->restartComponent(kParamValuesChanged);
            break;
        }
        case SET_PROGRAM:
        {
            // The synthesizer already changed the program via SendEvent(0xC0,...).
            // Calling setProgramOnly here would recurse back into SendEvent.
            // Just sync the VST3 parameters table and tell the host to re-read.
            effectx->syncAllParams();
            if (handler)
                handler->restartComponent(kParamValuesChanged);
            break;
        }
        case SET_PARAMETER:
        {
            int32 index       = static_cast<int32>(par1);
            ParamValue value  = static_cast<ParamValue>(par2) / MAXPARVALUE;
            if (handler)
            {
                handler->beginEdit(static_cast<ParamID>(index));
                handler->performEdit(static_cast<ParamID>(index), value);
                handler->endEdit(static_cast<ParamID>(index));
            }
            break;
        }
    }
}
