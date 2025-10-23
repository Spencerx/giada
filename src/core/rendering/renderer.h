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

#ifndef G_RENDERER_H
#define G_RENDERER_H

#include "src/core/sequencer.h"
#include <vector>

namespace mcl
{
class AudioBuffer;
}

namespace giada::m
{
class Engine;
class Sequencer;
class Mixer;
class Channel;
class PluginHost;
class KernelMidi;
#ifdef WITH_AUDIO_JACK
class JackSynchronizer;
class JackTransport;
#endif
} // namespace giada::m

namespace giada::m::model
{
class Model;
class Channels;
class Tracks;
} // namespace giada::m::model

namespace giada::m::rendering
{
class Renderer
{
public:
#ifdef WITH_AUDIO_JACK
	Renderer(Sequencer&, Mixer&, PluginHost&, JackSynchronizer&, JackTransport&, KernelMidi&);
#else
	Renderer(Sequencer&, Mixer&, PluginHost&, KernelMidi&);
#endif

	void render(mcl::AudioBuffer& out, const mcl::AudioBuffer& in, const model::Model&) const;

private:
	/* advanceTracks
	Processes Channels' static events (e.g. pre-recorded actions or sequencer
	events) in the current audio block. Called when the sequencer is running. */

	void advanceTracks(const Sequencer::EventBuffer&, const model::Tracks&,
	    SampleRange, int quantizerStep) const;

	void advanceChannel(const Channel&, const Sequencer::EventBuffer&, SampleRange, Frame quantizerStep) const;

	void renderTracks(const model::Tracks&, mcl::AudioBuffer& masterOut,
	    mcl::AudioBuffer& hardwareOut, const mcl::AudioBuffer& in, std::size_t scene,
	    bool hasSolos, bool seqIsRunning) const;
	void renderNormalChannel(const Channel& ch, const mcl::AudioBuffer& in, std::size_t scene, bool seqIsRunning) const;
	void renderMasterIn(const Channel&, mcl::AudioBuffer& in) const;
	void renderMasterOut(const Channel&, mcl::AudioBuffer& out, int channelOffset) const;
	void renderPreview(const Channel&, mcl::AudioBuffer& out) const;
	void renderSampleChannel(const Channel&, const mcl::AudioBuffer& in, std::size_t scene, bool seqIsRunning) const;
	void renderMidiChannel(const Channel&) const;

	/* mergeChannel
	Merges the Channel's audio buffer with 'out'. */

	void mergeChannel(const Channel&, mcl::AudioBuffer& out) const;

	/* mergeChannel (2)
	Same as above, with a channel offset for the destination buffer 'out'. */

	void mergeChannel(const Channel&, mcl::AudioBuffer& out, int destChannelOffset) const;

	Sequencer&  m_sequencer;
	Mixer&      m_mixer;
	PluginHost& m_pluginHost;
	KernelMidi& m_kernelMidi;
#ifdef WITH_AUDIO_JACK
	JackSynchronizer& m_jackSynchronizer;
	JackTransport&    m_jackTransport;
#endif
};
} // namespace giada::m::rendering

#endif
