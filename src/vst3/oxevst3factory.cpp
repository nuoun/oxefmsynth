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

#include "public.sdk/source/main/pluginfactory.h"
#include "oxevst3.h"

// Required by the SDK when not using the bundled vstinitiids.cpp compilation unit.
// Including it here ensures the interface IDs are defined exactly once per binary.
#include "public.sdk/source/vst/vstinitiids.cpp"

#define OXEFM_VENDOR       "Oxe Music Software"
#define OXEFM_URL          "http://www.oxesoft.com"
#define OXEFM_EMAIL        "oxe@oxesoft.com"
#define OXEFM_VERSION      VERSION_STR

BEGIN_FACTORY(OXEFM_VENDOR, OXEFM_URL, OXEFM_EMAIL,
              PFactoryInfo::kNoFlags)

    DEF_CLASS2(
        INLINE_UID_FROM_FUID(OxeFMSynthUID),
        PClassInfo::kManyInstances,
        kVstAudioEffectClass,
        "Oxe FM Synth",
        Vst::kDistributable,
        Vst::PlugType::kInstrumentSynth,
        OXEFM_VERSION,
        kVstVersionString,
        Steinberg::Vst::OxeVst3::createInstance
    )

END_FACTORY
