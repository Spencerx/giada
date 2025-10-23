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

#ifndef G_CHANNEL_FACTORY_H
#define G_CHANNEL_FACTORY_H

#include "src/core/channels/channel.h"
#include "src/core/idManager.h"
#include "src/core/patch.h"
#include "src/core/types.h"

namespace giada::m
{
class Wave;
class Channel;
} // namespace giada::m

namespace giada::m::channelFactory
{
struct Data
{
	Channel                        channel;
	std::unique_ptr<ChannelShared> shared;
};

/* getNextId
    Returns the next channel ID that will be assigned to a new channel. */

ID getNextId();

/* reset
    Resets internal ID generator. */

void reset();

/* create (1)
    Creates a new channel. If channelId == 0 generates a new ID, reuse the one
    passed in otherwise. */

Data create(ID channelId, ChannelType type, int bufferSize, Resampler::Quality, bool overdubProtection);

/* create (2)
    Creates a new channel given an existing one (i.e. clone). */

Data create(const Channel& ch, int bufferSize, Resampler::Quality);

/* (de)deserializeChannel
Creates a new channel given the patch raw data and vice versa. */

Channel              deserializeChannel(const Patch::Channel& c, ChannelShared&, float samplerateRatio, std::vector<Wave*> waves, std::vector<Plugin*>);
const Patch::Channel serializeChannel(const Channel& c);

/* deserializeShared
Returns a new ChannelShared object to be passed to deserializeChannel() above. */

std::unique_ptr<ChannelShared> deserializeShared(const Patch::Channel&, int bufferSize, Resampler::Quality);

} // namespace giada::m::channelFactory

#endif
