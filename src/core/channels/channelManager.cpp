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

#include "src/core/channels/channelManager.h"
#include "src/core/channels/channel.h"
#include "src/core/channels/channelFactory.h"
#include "src/core/midiEvent.h"
#include "src/core/mixer.h"
#include "src/core/model/model.h"
#include "src/core/rendering/midiOutput.h"
#include "src/core/rendering/midiReactions.h"
#include "src/core/rendering/sampleReactions.h"
#include "src/core/waveFactory.h"
#include "src/deps/mcl-audio-buffer/src/audioBuffer.hpp"
#include "src/utils/log.h"

namespace giada::m
{
namespace
{
constexpr int Q_ACTION_PLAY   = 0;
constexpr int Q_ACTION_REWIND = 10000; // Avoid clash with Q_ACTION_PLAY + channelId
} // namespace

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

ChannelManager::ChannelManager(model::Model& model, MidiMapper<KernelMidi>& m, ActionRecorder& a, KernelMidi& km)
: m_model(model)
, m_kernelMidi(km)
, m_actionRecorder(a)
, m_midiMapper(m)
{
}

/* -------------------------------------------------------------------------- */

Channel& ChannelManager::getChannel(ID channelId)
{
	return m_model.get().tracks.getChannel(channelId);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::reset(Frame framesInBuffer)
{
	/* Create internal track with internal channels (Master In/Out, Preview). */

	const bool               overdubProtection = false;
	const Resampler::Quality rsmpQuality       = m_model.get().kernelAudio.rsmpQuality;

	channelFactory::Data masterOutData = channelFactory::create(
	    Mixer::MASTER_OUT_CHANNEL_ID, ChannelType::MASTER, framesInBuffer, rsmpQuality, overdubProtection);
	channelFactory::Data masterInData = channelFactory::create(
	    Mixer::MASTER_IN_CHANNEL_ID, ChannelType::MASTER, framesInBuffer, rsmpQuality, overdubProtection);
	channelFactory::Data previewData = channelFactory::create(
	    Mixer::PREVIEW_CHANNEL_ID, ChannelType::PREVIEW, framesInBuffer, rsmpQuality, overdubProtection);

	m_model.get().tracks = {};

	model::Track& track = m_model.get().tracks.add(std::move(masterOutData.channel), 0, /*isInternal=*/true);
	track.addChannel(std::move(masterInData.channel));
	track.addChannel(std::move(previewData.channel));

	m_model.addChannelShared(std::move(masterOutData.shared));
	m_model.addChannelShared(std::move(masterInData.shared));
	m_model.addChannelShared(std::move(previewData.shared));

	/* Create six visible empty tracks. */

	addTrack(framesInBuffer);
	addTrack(framesInBuffer);
	addTrack(framesInBuffer);
	addTrack(framesInBuffer);
	addTrack(framesInBuffer);
	addTrack(framesInBuffer);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setBufferSize(int bufferSize)
{
	for (auto& channelShared : m_model.getAllChannelsShared())
		channelShared->setBufferSize(bufferSize);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::addTrack(Frame bufferSize)
{
	const bool               overdubProtection = false;
	const Resampler::Quality rsmpQuality       = m_model.get().kernelAudio.rsmpQuality;

	channelFactory::Data groupData = channelFactory::create(/*id=*/0, ChannelType::GROUP, bufferSize, rsmpQuality, overdubProtection);

	m_model.addChannelShared(std::move(groupData.shared));
	m_model.get().tracks.add(std::move(groupData.channel), G_DEFAULT_TRACK_WIDTH, /*isInternal=*/false);
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::removeTrack(std::size_t trackIndex)
{
	assert(canRemoveTrack(trackIndex));

	const Channel& ch = m_model.get().tracks.get(trackIndex).getGroupChannel();

	m_model.removeChannelShared(*ch.shared);
	m_model.get().tracks.remove(trackIndex);
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setTrackWidth(std::size_t trackIndex, int width)
{
	m_model.get().tracks.get(trackIndex).width = width;
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

Channel& ChannelManager::addChannel(ChannelType type, std::size_t trackIndex, int bufferSize)
{
	const bool               overdubProtectionDefaultOn = m_model.get().behaviors.overdubProtectionDefaultOn;
	const Resampler::Quality rsmpQuality                = m_model.get().kernelAudio.rsmpQuality;

	channelFactory::Data data = channelFactory::create(/*id=*/0, type, bufferSize, rsmpQuality, overdubProtectionDefaultOn);

	setupChannelCallbacks(data.channel, *data.shared);

	m_model.get().tracks.addChannel(std::move(data.channel), trackIndex);
	m_model.addChannelShared(std::move(data.shared));
	m_model.swap(model::SwapType::HARD);

	triggerOnChannelsAltered();

	return m_model.get().tracks.getLastChannel(trackIndex);
}

/* -------------------------------------------------------------------------- */

int ChannelManager::loadSampleChannel(ID channelId, const std::string& fname, int sampleRate, Resampler::Quality quality)
{
	waveFactory::Result res = waveFactory::createFromFile(fname, /*id=*/0, sampleRate, quality);
	if (res.status != G_RES_OK)
		return res.status;

	loadSampleChannel(channelId, m_model.addWave(std::move(res.wave)));

	return G_RES_OK;
}

/* -------------------------------------------------------------------------- */

void ChannelManager::loadSampleChannel(ID channelId, Wave& wave)
{
	Channel&    channel = m_model.get().tracks.getChannel(channelId);
	Wave&       newWave = wave;
	const Wave* oldWave = channel.sampleChannel->getWave();

	loadSampleChannel(channel, &newWave);
	m_model.swap(model::SwapType::HARD);

	/* Remove the old Wave, if any. It is safe to do it now: the audio thread is
	already processing the new Document. */

	if (oldWave != nullptr)
		m_model.removeWave(*oldWave);

	triggerOnChannelsAltered();
}

/* -------------------------------------------------------------------------- */

void ChannelManager::cloneChannel(ID channelId, int bufferSize, const std::vector<Plugin*>& plugins)
{
	const Channel&           oldChannel     = m_model.get().tracks.getChannel(channelId);
	const std::size_t        trackIndex     = m_model.get().tracks.getByChannel(channelId).getIndex();
	const Resampler::Quality rsmpQuality    = m_model.get().kernelAudio.rsmpQuality;
	channelFactory::Data     newChannelData = channelFactory::create(oldChannel, bufferSize, rsmpQuality);

	setupChannelCallbacks(newChannelData.channel, *newChannelData.shared);

	/* Clone Wave first, if any. */

	if (oldChannel.sampleChannel && oldChannel.sampleChannel->hasWave())
	{
		const Wave& oldWave  = *oldChannel.sampleChannel->getWave();
		const Frame oldShift = oldChannel.sampleChannel->shift;
		const Frame oldBegin = oldChannel.sampleChannel->begin;
		const Frame oldEnd   = oldChannel.sampleChannel->end;
		Wave&       wave     = m_model.addWave(waveFactory::createFromWave(oldWave));

		newChannelData.channel.loadWave(&wave, oldBegin, oldEnd, oldShift);
	}

	newChannelData.channel.plugins = plugins;

	/* Then push the new channel in the channels vector. */

	m_model.get().tracks.get(trackIndex).addChannel(std::move(newChannelData.channel));
	m_model.addChannelShared(std::move(newChannelData.shared));
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::freeSampleChannel(ID channelId)
{
	Channel& ch = m_model.get().tracks.getChannel(channelId);

	assert(ch.sampleChannel);

	const Wave* wave = ch.sampleChannel->getWave();

	loadSampleChannel(ch, nullptr);
	m_model.swap(model::SwapType::HARD);

	if (wave != nullptr)
		m_model.removeWave(*wave);

	triggerOnChannelsAltered();
}

/* -------------------------------------------------------------------------- */

void ChannelManager::freeAllSampleChannels()
{
	m_model.get().tracks.forEachChannel([this](Channel& ch)
	{
		if (ch.sampleChannel)
			loadSampleChannel(ch, nullptr);
		return true;
	});

	m_model.swap(model::SwapType::HARD);
	m_model.clearWaves();

	triggerOnChannelsAltered();
}

/* -------------------------------------------------------------------------- */

void ChannelManager::moveChannel(ID channelId, std::size_t newTrackIndex, std::size_t newPosition)
{
	Channel ch = m_model.get().tracks.getChannel(channelId); // Make copy
	m_model.get().tracks.removeChannel(channelId);
	m_model.get().tracks.addChannel(std::move(ch), newTrackIndex, newPosition);
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::deleteChannel(ID channelId)
{
	const Channel& ch   = m_model.get().tracks.getChannel(channelId);
	const Wave*    wave = ch.sampleChannel ? ch.sampleChannel->getWave() : nullptr;

	m_model.removeChannelShared(*ch.shared);
	m_model.get().tracks.getByChannel(channelId).removeChannel(channelId);
	m_model.swap(model::SwapType::HARD);

	if (wave != nullptr)
		m_model.removeWave(*wave);

	triggerOnChannelsAltered();
}

/* -------------------------------------------------------------------------- */

void ChannelManager::renameChannel(ID channelId, const std::string& name)
{
	m_model.get().tracks.getChannel(channelId).name = name;
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

float ChannelManager::getMasterInVol() const
{
	return m_model.get().tracks.getChannel(Mixer::MASTER_IN_CHANNEL_ID).volume;
}

float ChannelManager::getMasterOutVol() const
{
	return m_model.get().tracks.getChannel(Mixer::MASTER_OUT_CHANNEL_ID).volume;
}

/* -------------------------------------------------------------------------- */

void ChannelManager::finalizeInputRec(const mcl::AudioBuffer& buffer, Frame recordedFrames, Frame currentFrame)
{
	for (Channel* ch : getRecordableChannels())
		recordChannel(*ch, buffer, recordedFrames, currentFrame);
	for (Channel* ch : getOverdubbableChannels())
		overdubChannel(*ch, buffer, currentFrame);

	triggerOnChannelsAltered();
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setInputMonitor(ID channelId, bool value)
{
	m_model.get().tracks.getChannel(channelId).sampleChannel->inputMonitor = value;
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setVolume(ID channelId, float value)
{
	m_model.get().tracks.getChannel(channelId).volume = std::clamp(value, 0.0f, G_MAX_VOLUME);
	m_model.swap(model::SwapType::SOFT);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setPitch(ID channelId, float value)
{
	assert(m_model.get().tracks.getChannel(channelId).sampleChannel);

	const float pitch = std::clamp(value, G_MIN_PITCH, G_MAX_PITCH);

	m_model.get().tracks.getChannel(channelId).sampleChannel->pitch                 = pitch;
	m_model.get().tracks.getChannel(Mixer::PREVIEW_CHANNEL_ID).sampleChannel->pitch = pitch;
	m_model.swap(model::SwapType::SOFT);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setPan(ID channelId, float value)
{
	m_model.get().tracks.getChannel(channelId).pan = std::clamp(value, 0.0f, G_MAX_PAN);
	m_model.swap(model::SwapType::SOFT);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setBeginEnd(ID channelId, Frame b, Frame e)
{
	Channel& c       = m_model.get().tracks.getChannel(channelId);
	Channel& preview = m_model.get().tracks.getChannel(Mixer::PREVIEW_CHANNEL_ID);

	assert(c.sampleChannel);

	b = std::clamp(b, 0, c.sampleChannel->getWaveSize() - 1);
	e = std::clamp(e, 1, c.sampleChannel->getWaveSize());
	if (b >= e)
		b = e - 1;
	else if (e < b)
		e = b + 1;

	if (c.shared->tracker.load() < b)
		c.shared->tracker.store(b);

	c.sampleChannel->begin       = b;
	c.sampleChannel->end         = e;
	preview.sampleChannel->begin = b;
	preview.sampleChannel->end   = e;
	m_model.swap(model::SwapType::HARD);
}

void ChannelManager::resetBeginEnd(ID channelId)
{
	Channel& c = m_model.get().tracks.getChannel(channelId);

	assert(c.sampleChannel);

	c.sampleChannel->begin = 0;
	c.sampleChannel->end   = c.sampleChannel->getWaveSize();
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::toggleArm(ID channelId)
{
	Channel& ch = m_model.get().tracks.getChannel(channelId);
	ch.armed    = !ch.armed;

	m_model.swap(model::SwapType::SOFT);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setOverdubProtection(ID channelId, bool value)
{
	Channel& ch                         = m_model.get().tracks.getChannel(channelId);
	ch.sampleChannel->overdubProtection = value;
	if (value == true && ch.armed)
		ch.armed = false;
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setSamplePlayerMode(ID channelId, SamplePlayerMode mode)
{
	m_model.get().tracks.getChannel(channelId).sampleChannel->mode = mode;
	m_model.swap(model::SwapType::SOFT);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setHeight(ID channelId, int height)
{
	m_model.get().tracks.getChannel(channelId).height = height;
	m_model.swap(model::SwapType::SOFT);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setSendToMaster(ID channelId, bool value)
{
	m_model.get().tracks.getChannel(channelId).sendToMaster = value;
	m_model.swap(model::SwapType::NONE);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::addExtraOutput(ID channelId, int offset)
{
	/* 'offset' can be only multiple of 2, that is stereo pair. */
	assert(offset >= 0 && offset % 2 == 0);

	m_model.get().tracks.getChannel(channelId).extraOutputs.push_back(offset);
	m_model.swap(model::SwapType::HARD);
}

void ChannelManager::removeExtraOutput(ID channelId, std::size_t i)
{
	u::vector::removeAt(m_model.get().tracks.getChannel(channelId).extraOutputs, i);
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::loadWaveInPreviewChannel(ID channelId)
{
	Channel&       previewCh = m_model.get().tracks.getChannel(Mixer::PREVIEW_CHANNEL_ID);
	const Channel& sourceCh  = m_model.get().tracks.getChannel(channelId);

	assert(previewCh.sampleChannel);
	assert(sourceCh.sampleChannel);

	previewCh.loadWave(sourceCh.sampleChannel->getWave());
	previewCh.sampleChannel->mode  = SamplePlayerMode::SINGLE_BASIC_PAUSE;
	previewCh.sampleChannel->begin = sourceCh.sampleChannel->begin;
	previewCh.sampleChannel->end   = sourceCh.sampleChannel->end;
	previewCh.sampleChannel->pitch = sourceCh.sampleChannel->pitch;

	m_model.swap(model::SwapType::SOFT);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::freeWaveInPreviewChannel()
{
	Channel& previewCh = m_model.get().tracks.getChannel(Mixer::PREVIEW_CHANNEL_ID);

	previewCh.loadWave(nullptr);
	m_model.swap(model::SwapType::SOFT);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setPreviewTracker(Frame f)
{
	m_model.get().tracks.getChannel(m::Mixer::PREVIEW_CHANNEL_ID).shared->tracker.store(f);
}

/* -------------------------------------------------------------------------- */

bool ChannelManager::saveSample(ID channelId, const std::string& filePath)
{
	const Channel& ch = m_model.get().tracks.getChannel(channelId);

	assert(ch.sampleChannel);

	Wave* wave = ch.sampleChannel->getWave();

	assert(wave != nullptr);

	if (!waveFactory::save(*wave, filePath))
		return false;

	u::log::print("[saveSample] sample saved to {}\n", filePath);

	/* Reset logical and edited states in Wave. */

	model::SharedLock lock = m_model.lockShared();
	wave->setLogical(false);
	wave->setEdited(false);

	return true;
}

/* -------------------------------------------------------------------------- */

void ChannelManager::consolidateChannels(const std::unordered_set<ID>& ids)
{
	for (ID id : ids)
	{
		Channel& ch = m_model.get().tracks.getChannel(id);
		ch.shared->readActions.store(true);
		ch.shared->recStatus.store(ChannelStatus::PLAY);
		if (ch.type == ChannelType::MIDI)
			ch.shared->playStatus.store(ChannelStatus::PLAY);
	}
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

bool ChannelManager::hasInputRecordableChannels() const
{
	return m_model.get().tracks.anyChannelOf([](const Channel& ch)
	{ return ch.canInputRec(); });
}

bool ChannelManager::hasActions() const
{
	return m_model.get().tracks.anyChannelOf([](const Channel& ch)
	{ return ch.hasActions; });
}

bool ChannelManager::hasAudioData() const
{
	return m_model.get().tracks.anyChannelOf([](const Channel& ch)
	{
		return ch.sampleChannel && ch.sampleChannel->hasWave();
	});
}

bool ChannelManager::hasSolos() const
{
	return m_model.get().tracks.anyChannelOf([](const Channel& ch)
	{
		return !ch.isInternal() && ch.isSoloed();
	});
}

/* -------------------------------------------------------------------------- */

bool ChannelManager::canRemoveTrack(std::size_t trackIndex) const
{
	return m_model.get().tracks.get(trackIndex).getNumChannels() == 1;
}

/* -------------------------------------------------------------------------- */

void ChannelManager::loadSampleChannel(Channel& ch, Wave* w, Frame begin, Frame end, Frame shift) const
{
	ch.loadWave(w, begin, end, shift);
	ch.name = w != nullptr ? w->getBasename(/*ext=*/false) : "";
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setupChannelCallbacks(const Channel& ch, ChannelShared& shared) const
{
	assert(onChannelPlayStatusChanged != nullptr);

	shared.playStatus.onChange = [this, id = ch.id](ChannelStatus status)
	{
		onChannelPlayStatusChanged(id, status);
	};

	if (ch.type == ChannelType ::SAMPLE)
	{
		shared.quantizer->schedule(Q_ACTION_PLAY + ch.id, [&shared](Frame delta)
		{
			rendering::playSampleChannel(shared, delta);
		});
		shared.quantizer->schedule(Q_ACTION_REWIND + ch.id, [&shared](Frame delta)
		{
			const ChannelStatus status = shared.playStatus.load();
			if (status == ChannelStatus::OFF)
				rendering::playSampleChannel(shared, delta);
			else if (status == ChannelStatus::PLAY || status == ChannelStatus::ENDING)
				rendering::rewindSampleChannel(shared, delta);
		});
	}
}

/* -------------------------------------------------------------------------- */

std::vector<Channel*> ChannelManager::getRecordableChannels()
{
	return m_model.get().tracks.getChannelsIf([](const Channel& c)
	{ return c.canInputRec() && !c.hasWave(); });
}

std::vector<Channel*> ChannelManager::getOverdubbableChannels()
{
	return m_model.get().tracks.getChannelsIf([](const Channel& c)
	{ return c.canInputRec() && c.hasWave(); });
}

/* -------------------------------------------------------------------------- */

void ChannelManager::setupChannelPostRecording(Channel& ch, Frame currentFrame)
{
	/* Start sample channels in loop mode right away. */
	if (ch.sampleChannel->isAnyLoopMode())
		ch.kickIn(currentFrame);
	/* Disable 'arm' button if overdub protection is on. */
	if (ch.sampleChannel->overdubProtection == true)
		ch.armed = false;
}

/* -------------------------------------------------------------------------- */

void ChannelManager::recordChannel(Channel& ch, const mcl::AudioBuffer& buffer, Frame recordedFrames, Frame currentFrame)
{
	assert(onChannelRecorded != nullptr);

	std::unique_ptr<Wave> wave = onChannelRecorded(recordedFrames);

	G_DEBUG("Created new Wave, size={}", wave->getBuffer().countFrames());

	/* Copy up to wave.getSize() from the mixer's input buffer into wave's. */

	wave->getBuffer().setAll(buffer, wave->getBuffer().countFrames(), 0, 0);

	/* Update channel with the new Wave. */

	loadSampleChannel(ch, &m_model.addWave(std::move(wave)));
	setupChannelPostRecording(ch, currentFrame);

	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::overdubChannel(Channel& ch, const mcl::AudioBuffer& buffer, Frame currentFrame)
{
	Wave* wave = ch.sampleChannel->getWave();

	/* Need model::DataLock here, as data might be being read by the audio
	thread at the same time. */

	model::SharedLock lock = m_model.lockShared();

	wave->getBuffer().sumAll(buffer);
	wave->setLogical(true);

	setupChannelPostRecording(ch, currentFrame);
}

/* -------------------------------------------------------------------------- */

void ChannelManager::triggerOnChannelsAltered()
{
	assert(onChannelsAltered != nullptr);
	onChannelsAltered();
}
} // namespace giada::m
