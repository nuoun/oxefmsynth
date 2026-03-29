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

#include "oxevst3.h"
#include "oxevst3editor.h"
#include "vsthostinterface3.h"

#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/base/ibstream.h"
#include "public.sdk/source/vst/vstparameters.h"

#include <string.h>
#include <stdio.h>

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
OxeVst3::OxeVst3()
    : posExt(0)
    , posInt(0)
    , bufferPos(0)
{
    memset(&midiEvents, 0, sizeof(midiEvents));
}

//------------------------------------------------------------------------
OxeVst3::~OxeVst3()
{
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3::initialize(FUnknown* context)
{
    tresult result = SingleComponentEffect::initialize(context);
    if (result != kResultTrue)
        return result;

    // No audio inputs (instrument), one stereo output
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);

    // One MIDI event input bus (16 channels)
    addEventInput(STR16("MIDI In"), 16);

    // Register all synth parameters as normalised continuous values [0..1].
    // The synth uses [0..MAXPARVALUE] internally; conversion happens at the
    // setParamNormalized / getParamStringByValue boundary.
    for (int32 i = 0; i < PARAMETERS_COUNT; i++)
    {
        char name[64] = {};
        snprintf(name, sizeof(name), "Param%d", i);

        // Convert narrow char name to UTF-16 for the VST3 API
        TChar wname[64] = {};
        for (int k = 0; k < 63 && name[k]; ++k)
            wname[k] = static_cast<TChar>(name[k]);

        RangeParameter* param = new RangeParameter(
            wname, i, nullptr,
            0.0, 1.0, 0.0,
            0,
            ParameterInfo::kCanAutomate);
        parameters.addParameter(param);
    }

    return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3::terminate()
{
    return SingleComponentEffect::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3::setupProcessing(ProcessSetup& setup)
{
    tresult result = SingleComponentEffect::setupProcessing(setup);
    synthesizer.SetSampleRate(static_cast<float>(setup.sampleRate));
    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3::setActive(TBool state)
{
    if (!state)
        synthesizer.KillNotes();
    return kResultOk;
}

//------------------------------------------------------------------------
void OxeVst3::processEvents(IEventList* inputEvents)
{
    if (!inputEvents)
        return;

    int32 count = inputEvents->getEventCount();
    for (int32 i = 0; i < count; i++)
    {
        Event e;
        if (inputEvents->getEvent(i, e) != kResultOk)
            continue;

        unsigned char bstat = 0, bdad1 = 0, bdad2 = 0;

        switch (e.type)
        {
            case Event::kNoteOnEvent:
                bstat = static_cast<unsigned char>(0x90 | (e.noteOn.channel & 0x0F));
                bdad1 = static_cast<unsigned char>(e.noteOn.pitch & 0x7F);
                bdad2 = static_cast<unsigned char>(
                    e.noteOn.velocity >= 1.f ? 127
                    : static_cast<int>(e.noteOn.velocity * 127.f) & 0x7F);
                break;
            case Event::kNoteOffEvent:
                bstat = static_cast<unsigned char>(0x80 | (e.noteOff.channel & 0x0F));
                bdad1 = static_cast<unsigned char>(e.noteOff.pitch & 0x7F);
                bdad2 = 0;
                break;
            default:
                continue;
        }

        int32 n = (midiEvents.nextEvent + midiEvents.eventsCount) & EVENTS_MASK;
        midiEvents.event[n].bstat = bstat;
        midiEvents.event[n].bdad1 = bdad1;
        midiEvents.event[n].bdad2 = bdad2;
        midiEvents.event[n].pos   = e.sampleOffset + bufferPos;
        midiEvents.eventsCount++;
    }
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3::process(ProcessData& data)
{
    // Handle parameter changes from the host (automation)
    if (data.inputParameterChanges)
    {
        int32 numChanges = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numChanges; i++)
        {
            IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);
            if (!queue)
                continue;
            int32 sampleOffset;
            ParamValue value;
            int32 numPoints = queue->getPointCount();
            if (numPoints > 0 &&
                queue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
            {
                ParamID tag = queue->getParameterId();
                if (tag < static_cast<ParamID>(PARAMETERS_COUNT))
                    synthesizer.SetPar(0, static_cast<int>(tag),
                                       static_cast<float>(value) * MAXPARVALUE);
            }
        }
    }

    if (!data.outputs || data.numOutputs == 0 || data.numSamples == 0)
        return kResultOk;

    float* out1 = data.outputs[0].channelBuffers32[0];
    float* out2 = data.outputs[0].channelBuffers32[1];

    processEvents(data.inputEvents);

    const int32 tambufferInt = SAMPLES_PER_PROCESS << 1;
    const int32 tambufferExt = data.numSamples;

    while (true)
    {
        if (!posInt)
        {
            // Dispatch MIDI events that fall within this processing slice
            while (midiEvents.eventsCount)
            {
                if (midiEvents.event[midiEvents.nextEvent].pos >
                    bufferPos + SAMPLES_PER_PROCESS)
                    break;
                if (midiEvents.event[midiEvents.nextEvent].pos < bufferPos)
                    midiEvents.event[midiEvents.nextEvent].pos = bufferPos;
                synthesizer.SendEvent(
                    midiEvents.event[midiEvents.nextEvent].bstat,
                    midiEvents.event[midiEvents.nextEvent].bdad1,
                    midiEvents.event[midiEvents.nextEvent].bdad2,
                    midiEvents.event[midiEvents.nextEvent].pos);
                midiEvents.eventsCount--;
                midiEvents.nextEvent++;
                midiEvents.nextEvent &= EVENTS_MASK;
            }
            synthesizer.Process(synthesizer.buffers.bSynthOut,
                                 SAMPLES_PER_PROCESS, bufferPos);
            bufferPos += SAMPLES_PER_PROCESS;
        }

        int32 iaux = (tambufferInt - posInt < tambufferExt - posExt)
                         ? tambufferInt - posInt
                         : tambufferExt - posExt;
        while (iaux > 0)
        {
            out1[posExt] = static_cast<float>(synthesizer.buffers.bSynthOut[posInt++]) / 32767.f;
            out2[posExt] = static_cast<float>(synthesizer.buffers.bSynthOut[posInt++]) / 32767.f;
            posExt++;
            iaux -= 2;
        }

        if (posInt >= tambufferInt)
            posInt = 0;

        if (posExt >= tambufferExt)
        {
            posExt = 0;
            break;
        }
    }

    return kResultOk;
}

//------------------------------------------------------------------------
// State serialisation: save/load the full bank (identical to VST2 getChunk/setChunk)
//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3::getState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    SBank* bank = synthesizer.GetBank();
    int32  size = static_cast<int32>(sizeof(SBank));

    int32 written = 0;
    state->write(bank, size, &written);
    return (written == size) ? kResultTrue : kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3::setState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    SBank bank;
    int32 read = 0;
    state->read(&bank, static_cast<int32>(sizeof(SBank)), &read);
    if (read != static_cast<int32>(sizeof(SBank)))
        return kResultFalse;

    synthesizer.SetBank(&bank);
    return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API OxeVst3::createView(FIDString name)
{
    if (strcmp(name, ViewType::kEditor) == 0)
        return new OxeVst3Editor(this, &synthesizer);
    return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3::setParamNormalized(ParamID tag, ParamValue value)
{
    tresult result = SingleComponentEffect::setParamNormalized(tag, value);
    if (result == kResultTrue && tag < static_cast<ParamID>(PARAMETERS_COUNT))
        synthesizer.SetPar(0, static_cast<int>(tag),
                           static_cast<float>(value) * MAXPARVALUE);
    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API OxeVst3::getParamStringByValue(ParamID tag,
                                                    ParamValue valueNormalized,
                                                    String128 string)
{
    if (tag >= static_cast<ParamID>(PARAMETERS_COUNT))
        return kResultFalse;

    char text[32] = {};
    snprintf(text, sizeof(text), "%.2f",
             static_cast<float>(valueNormalized) * MAXPARVALUE);

    // Copy narrow string into the UTF-16 String128 output buffer
    for (int i = 0; i < 127 && text[i]; ++i)
        string[i] = static_cast<TChar>(text[i]);
    string[127] = 0;
    return kResultTrue;
}

//------------------------------------------------------------------------
void OxeVst3::setProgramOnly(int32 program)
{
    synthesizer.SendEvent(0xC0, static_cast<unsigned char>(program), 0, 0);
}

//------------------------------------------------------------------------
void OxeVst3::syncAllParams()
{
    for (int32 i = 0; i < PARAMETERS_COUNT; i++)
    {
        ParamValue norm = static_cast<ParamValue>(synthesizer.GetPar(0, i)) / MAXPARVALUE;
        // Call the parent directly to avoid re-invoking SetPar via our override.
        SingleComponentEffect::setParamNormalized(static_cast<ParamID>(i), norm);
    }
}

} // namespace Vst
} // namespace Steinberg
