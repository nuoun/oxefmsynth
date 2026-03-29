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

#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "synthesizer.h"

//
// 16-byte plugin FUID – generated once and fixed for all time.
// Hosts use this to identify the plugin across sessions.
// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
//
static const Steinberg::FUID OxeFMSynthUID(
    0xA1B2C3D4, 0xE5F67890, 0xABCDEF12, 0x34567890);

#define MAX_EVENTS_AT_ONCE_POWEROFTWO 8
#define EVENTS_MASK                   ((1 << MAX_EVENTS_AT_ONCE_POWEROFTWO) - 1)

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
class OxeVst3 : public SingleComponentEffect
{
public:
    OxeVst3();
    ~OxeVst3() SMTG_OVERRIDE;

    static FUnknown* createInstance(void* /*context*/)
    {
        return static_cast<IAudioProcessor*>(new OxeVst3);
    }

    //--- from IPluginBase -----------------------------------------------
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    //--- from IAudioProcessor ------------------------------------------
    tresult PLUGIN_API setupProcessing(ProcessSetup& setup) SMTG_OVERRIDE;
    tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;
    tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE;
    tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
    tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;

    //--- from IEditController ------------------------------------------
    IPlugView* PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;
    tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value) SMTG_OVERRIDE;
    tresult PLUGIN_API getParamStringByValue(ParamID tag, ParamValue valueNormalized,
                                             String128 string) SMTG_OVERRIDE;

    //--- internal helpers -----------------------------------------------
    // Called by the host-interface bridge when the GUI changes a program slot.
    void setProgramOnly(int32 program);

    // Sync the VST3 parameters table from the synthesizer's current state.
    // Calls the parent setParamNormalized directly so SetPar is not re-invoked.
    void syncAllParams();

    CSynthesizer synthesizer;

private:
    struct MIDIEvent
    {
        unsigned char bstat;
        unsigned char bdad1;
        unsigned char bdad2;
        int32         pos;
    };
    struct MIDIEvents
    {
        int32     eventsCount;
        int32     nextEvent;
        MIDIEvent event[1 << MAX_EVENTS_AT_ONCE_POWEROFTWO];
    };

    MIDIEvents midiEvents;
    int32      posExt;
    int32      posInt;
    int32      bufferPos;

    void processEvents(IEventList* inputEvents);
};

} // namespace Vst
} // namespace Steinberg
