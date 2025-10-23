/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * -----------------------------------------------------------------------------
 *
 * Copyright (C) 2010-2025 Giovanni A. Zuliani | Monocasual Laboratories
 *
 * This file is part of Giada - Your Hardcore Loopmachine.
 *
 * Giada - Your Hardcore Loopmachine is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Giada - Your Hardcore Loopmachine is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Giada - Your Hardcore Loopmachine. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------- */

#include "src/core/model/document.h"
#include "src/core/actions/actionFactory.h"
#include "src/core/channels/channelFactory.h"
#include "src/core/conf.h"
#include "src/core/model/shared.h"
#include "src/utils/vector.h"

namespace giada::m::model
{
void Document::load(const Patch& patch, Shared& shared, float sampleRateRatio)
{
	tracks = {};

	for (const Patch::Track& ptrack : patch.tracks)
	{
		Track& track = tracks.add(ptrack.width, ptrack.internal);
		for (const ID channelId : ptrack.channels)
		{
			const Patch::Channel& pchannel = *u::vector::findIfSafe(patch.channels, channelId);
			SceneArray<Sample>    samples;
			std::transform(pchannel.samples.begin(), pchannel.samples.end(), samples.begin(),
			    [&shared](const Patch::Sample& ps)
			{
				return Sample{shared.findWave(ps.waveId), ps.range, ps.shift};
			});
			std::vector<Plugin*> plugins       = shared.findPlugins(pchannel.pluginIds);
			ChannelShared*       channelShared = shared.findChannel(pchannel.id);
			assert(channelShared != nullptr);

			Channel channel = channelFactory::deserializeChannel(pchannel, *channelShared, sampleRateRatio, samples, plugins);
			track.addChannel(std::move(channel));
		}
	}

	actions.set(actionFactory::deserializeActions(patch.actions));

	sequencer.status    = SeqStatus::STOPPED;
	sequencer.bars      = patch.bars;
	sequencer.beats     = patch.beats;
	sequencer.bpm       = patch.bpm;
	sequencer.quantize  = patch.quantize;
	sequencer.metronome = patch.metronome;
}

/* -------------------------------------------------------------------------- */

void Document::load(const Conf& conf)
{
	kernelAudio.api                     = conf.soundSystem;
	kernelAudio.deviceOut.id            = conf.soundDeviceOut;
	kernelAudio.deviceOut.channelsCount = conf.channelsOutCount;
	kernelAudio.deviceOut.channelsStart = conf.channelsOutStart;
	kernelAudio.deviceIn.id             = conf.soundDeviceIn;
	kernelAudio.deviceIn.channelsCount  = conf.channelsInCount;
	kernelAudio.deviceIn.channelsStart  = conf.channelsInStart;
	kernelAudio.samplerate              = conf.samplerate;
	kernelAudio.buffersize              = conf.buffersize;
	kernelAudio.limitOutput             = conf.limitOutput;
	kernelAudio.rsmpQuality             = conf.rsmpQuality;
	kernelAudio.recTriggerLevel         = conf.recTriggerLevel;

	kernelMidi.api         = conf.midiSystem;
	kernelMidi.devicesOut  = conf.midiDevicesOut;
	kernelMidi.devicesIn   = conf.midiDevicesIn;
	kernelMidi.midiMapPath = conf.midiMapPath;
	kernelMidi.sync        = conf.midiSync;

	mixer.inputRecMode   = conf.inputRecMode;
	mixer.recTriggerMode = conf.recTriggerMode;

	midiIn.enabled    = conf.midiInEnabled;
	midiIn.filter     = conf.midiInFilter;
	midiIn.rewind     = conf.midiInRewind;
	midiIn.startStop  = conf.midiInStartStop;
	midiIn.actionRec  = conf.midiInActionRec;
	midiIn.inputRec   = conf.midiInInputRec;
	midiIn.metronome  = conf.midiInMetronome;
	midiIn.volumeIn   = conf.midiInVolumeIn;
	midiIn.volumeOut  = conf.midiInVolumeOut;
	midiIn.beatDouble = conf.midiInBeatDouble;
	midiIn.beatHalf   = conf.midiInBeatHalf;

	behaviors.chansStopOnSeqHalt         = conf.chansStopOnSeqHalt;
	behaviors.treatRecsAsLoops           = conf.treatRecsAsLoops;
	behaviors.inputMonitorDefaultOn      = conf.inputMonitorDefaultOn;
	behaviors.overdubProtectionDefaultOn = conf.overdubProtectionDefaultOn;
}

/* -------------------------------------------------------------------------- */

void Document::store(Patch& patch) const
{
	patch.bars      = sequencer.bars;
	patch.beats     = sequencer.beats;
	patch.bpm       = sequencer.bpm;
	patch.quantize  = sequencer.quantize;
	patch.metronome = sequencer.metronome;
	patch.actions   = actionFactory::serializeActions(actions.getAll());

	for (const Track& track : tracks.getAll())
	{
		patch.tracks.push_back({track.width, track.isInternal(), track.getChannels().getAllIDs()});
		for (const Channel& c : track.getChannels().getAll())
			patch.channels.push_back(channelFactory::serializeChannel(c));
	}
}

/* -------------------------------------------------------------------------- */

void Document::store(Conf& conf) const
{
	conf.soundSystem      = kernelAudio.api;
	conf.soundDeviceOut   = kernelAudio.deviceOut.id;
	conf.channelsOutCount = kernelAudio.deviceOut.channelsCount;
	conf.channelsOutStart = kernelAudio.deviceOut.channelsStart;
	conf.soundDeviceIn    = kernelAudio.deviceIn.id;
	conf.channelsInCount  = kernelAudio.deviceIn.channelsCount;
	conf.channelsInStart  = kernelAudio.deviceIn.channelsStart;
	conf.samplerate       = kernelAudio.samplerate;
	conf.buffersize       = kernelAudio.buffersize;
	conf.limitOutput      = kernelAudio.limitOutput;
	conf.rsmpQuality      = kernelAudio.rsmpQuality;
	conf.recTriggerLevel  = kernelAudio.recTriggerLevel;

	conf.midiSystem     = kernelMidi.api;
	conf.midiDevicesOut = kernelMidi.devicesOut;
	conf.midiDevicesIn  = kernelMidi.devicesIn;
	conf.midiMapPath    = kernelMidi.midiMapPath;
	conf.midiSync       = kernelMidi.sync;

	conf.inputRecMode   = mixer.inputRecMode;
	conf.recTriggerMode = mixer.recTriggerMode;

	conf.midiInEnabled    = midiIn.enabled;
	conf.midiInFilter     = midiIn.filter;
	conf.midiInRewind     = midiIn.rewind;
	conf.midiInStartStop  = midiIn.startStop;
	conf.midiInActionRec  = midiIn.actionRec;
	conf.midiInInputRec   = midiIn.inputRec;
	conf.midiInMetronome  = midiIn.metronome;
	conf.midiInVolumeIn   = midiIn.volumeIn;
	conf.midiInVolumeOut  = midiIn.volumeOut;
	conf.midiInBeatDouble = midiIn.beatDouble;
	conf.midiInBeatHalf   = midiIn.beatHalf;

	conf.chansStopOnSeqHalt         = behaviors.chansStopOnSeqHalt;
	conf.treatRecsAsLoops           = behaviors.treatRecsAsLoops;
	conf.inputMonitorDefaultOn      = behaviors.inputMonitorDefaultOn;
	conf.overdubProtectionDefaultOn = behaviors.overdubProtectionDefaultOn;
}

/* -------------------------------------------------------------------------- */

#if G_DEBUG_MODE
void Document::debug() const
{
	mixer.debug();
	tracks.debug();
	actions.debug();
}
#endif
} // namespace giada::m::model
