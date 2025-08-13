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

#include "src/core/channels/channel.h"
#include "src/gui/const.h"
#include <cassert>
#if G_DEBUG_MODE
#include "src/utils/string.h"
#include <fmt/core.h>
#endif

namespace giada::m
{
Channel::Channel(ChannelType type, ID id, ChannelShared& s)
: shared(&s)
, id(id)
, type(type)
, volume(G_DEFAULT_VOL)
, pan(G_DEFAULT_PAN)
, armed(false)
, key(0)
, hasActions(false)
, height(G_GUI_UNIT)
, sendToMaster(true)
, m_mute(false)
, m_solo(false)
{
	switch (type)
	{
	case ChannelType::SAMPLE:
	case ChannelType::PREVIEW:
		sampleChannel.emplace();
		break;

	case ChannelType::MIDI:
		midiChannel.emplace();
		break;

	default:
		break;
	}
}

/* -------------------------------------------------------------------------- */

Channel::Channel(const Patch::Channel& p, ChannelShared& s, float samplerateRatio, Wave* wave, std::vector<Plugin*> plugins)
: shared(&s)
, id(p.id)
, type(p.type)
, volume(p.volume)
, pan(p.pan)
, armed(p.armed)
, key(p.key)
, hasActions(p.hasActions)
, name(p.name)
, height(p.height)
, plugins(plugins)
, sendToMaster(p.sendToMaster)
, extraOutputs(p.extraOutputs)
, midiInput(p)
, midiLightning(p)
, m_mute(p.mute)
, m_solo(p.solo)
{
	shared->readActions.store(p.readActions);
	shared->recStatus.store(p.readActions ? ChannelStatus::PLAY : ChannelStatus::OFF);

	switch (type)
	{
	case ChannelType::SAMPLE:
	case ChannelType::PREVIEW:
		sampleChannel.emplace(p, wave, samplerateRatio);
		break;

	case ChannelType::MIDI:
		midiChannel.emplace(p);
		break;

	default:
		break;
	}
}

/* -------------------------------------------------------------------------- */

bool Channel::operator==(const Channel& other) const
{
	return id == other.id;
}

/* -------------------------------------------------------------------------- */

bool Channel::isInternal() const
{
	return type == ChannelType::MASTER || type == ChannelType::PREVIEW;
}

bool Channel::isMuted() const
{
	/* Internals can't be muted. */
	return !isInternal() && m_mute;
}

bool Channel::isSoloed() const
{
	return m_solo;
}

bool Channel::isAudible(bool mixerHasSolos) const
{
	if (isInternal())
		return true;
	if (isMuted())
		return false;
	return !mixerHasSolos || (mixerHasSolos && isSoloed());
}

bool Channel::canInputRec() const
{
	if (type != ChannelType::SAMPLE)
		return false;

	bool hasWave     = sampleChannel->hasWave();
	bool isProtected = sampleChannel->overdubProtection;
	bool canOverdub  = !hasWave || (hasWave && !isProtected);

	return armed && canOverdub;
}

bool Channel::canActionRec() const
{
	return hasWave() && !sampleChannel->isAnyLoopMode();
}

bool Channel::hasWave() const
{
	return sampleChannel && sampleChannel->hasWave();
}

bool Channel::isPlaying() const
{
	ChannelStatus s = shared->playStatus.load();
	return s == ChannelStatus::PLAY || s == ChannelStatus::ENDING;
}

/* -------------------------------------------------------------------------- */

#if G_DEBUG_MODE
std::string Channel::debug() const
{
	std::string out = fmt::format("ID={} name='{}' type={} channelShared={}",
	    id, name, u::string::toString(type), (void*)&shared);

	if (type == ChannelType::SAMPLE || type == ChannelType::PREVIEW)
		out += fmt::format(" wave={} mode={} begin={} end={}",
		    (void*)sampleChannel->getWave(),
		    u::string::toString(sampleChannel->mode),
		    sampleChannel->range.a,
		    sampleChannel->range.b);

	return out;
}
#endif

/* -------------------------------------------------------------------------- */

bool Channel::canReceiveAudio() const
{
	return type == ChannelType::SAMPLE && armed && sampleChannel->inputMonitor;
}

/* -------------------------------------------------------------------------- */

bool Channel::canSendMidi() const
{
	return type == ChannelType::MIDI && !isMuted() && midiChannel->outputEnabled;
}

/* -------------------------------------------------------------------------- */

void Channel::setMute(bool v)
{
	m_mute = v;
}

void Channel::setSolo(bool v)
{
	m_solo = v;
}

/* -------------------------------------------------------------------------- */

void Channel::loadWave(Wave* w, SampleRange newRange, Frame newShift)
{
	assert(sampleChannel);

	shared->tracker.store(0);
	shared->playStatus.store(w != nullptr ? ChannelStatus::OFF : ChannelStatus::EMPTY);

	sampleChannel->loadWave(w, newRange, newShift);
}

/* -------------------------------------------------------------------------- */

void Channel::setWave(Wave* w, float samplerateRatio)
{
	assert(sampleChannel);

	sampleChannel->setWave(w, samplerateRatio);
}

/* -------------------------------------------------------------------------- */

void Channel::kickIn(Frame f)
{
	shared->tracker.store(f);
	shared->playStatus.store(ChannelStatus::PLAY);
}
} // namespace giada::m