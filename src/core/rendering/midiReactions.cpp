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

#include "src/core/rendering/midiReactions.h"
#include "src/core/actions/actionRecorder.h"
#include "src/core/kernelMidi.h"
#include <cassert>

namespace giada::m::rendering
{
void playMidiChannel(WeakAtomic<ChannelStatus>& a_playStatus)
{
	ChannelStatus playStatus = a_playStatus.load();

	switch (playStatus)
	{
	case ChannelStatus::PLAY:
		playStatus = ChannelStatus::ENDING;
		break;

	case ChannelStatus::ENDING:
		playStatus = ChannelStatus::PLAY;
		break;

	case ChannelStatus::WAIT:
		playStatus = ChannelStatus::OFF;
		break;

	case ChannelStatus::OFF:
		playStatus = ChannelStatus::WAIT;
		break;

	default:
		break;
	}

	a_playStatus.store(playStatus);
}

/* -------------------------------------------------------------------------- */

void stopMidiChannel(WeakAtomic<ChannelStatus>& a_playStatus)
{
	a_playStatus.store(ChannelStatus::OFF);
}

/* -------------------------------------------------------------------------- */

void rewindMidiChannel(WeakAtomic<ChannelStatus>& a_playStatus)
{
	ChannelStatus playStatus = a_playStatus.load();

	if (playStatus == ChannelStatus::ENDING)
		playStatus = ChannelStatus::OFF;
	else if (playStatus == ChannelStatus::WAIT)
		playStatus = ChannelStatus::PLAY;

	a_playStatus.store(playStatus);
}

/* -------------------------------------------------------------------------- */

void recordMidiAction(ID channelId, std::size_t scene, const MidiEvent& event, Frame currentFrameQuantized, ActionRecorder& actionRecorder)
{
	MidiEvent flat(event);
	flat.setChannel(0);
	actionRecorder.liveRec(channelId, scene, flat, currentFrameQuantized);
}
} // namespace giada::m::rendering
