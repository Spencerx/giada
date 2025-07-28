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

#include "src/core/api/pluginsApi.h"
#include "src/core/channels/channelManager.h"
#include "src/core/engine.h"
#include "src/core/kernelAudio.h"
#include "src/core/mixer.h"
#include "src/core/plugins/pluginFactory.h"
#include "src/utils/fs.h"

namespace giada::m
{
PluginsApi::PluginsApi(KernelAudio& ka, PluginManager& pm, PluginHost& ph, model::Model& m)
: m_kernelAudio(ka)
, m_pluginManager(pm)
, m_pluginHost(ph)
, m_model(m)
{
}

/* -------------------------------------------------------------------------- */

const Plugin* PluginsApi::get(ID pluginId) const
{
	return m_model.findPlugin(pluginId);
}

/* -------------------------------------------------------------------------- */

std::vector<PluginInfo> PluginsApi::getInfo() const
{
	return m_pluginManager.getPluginsInfo();
}

/* -------------------------------------------------------------------------- */

int PluginsApi::countAvailablePlugins() const
{
	return m_pluginManager.countAvailablePlugins();
}

/* -------------------------------------------------------------------------- */

void PluginsApi::add(const std::string& juceId, ID channelId)
{
	/* Plug-in creation must be done in the main thread, due to JUCE and VST3
	internal workings. */

	const int sampleRate = m_kernelAudio.getSampleRate();
	const int bufferSize = m_kernelAudio.getBufferSize();

	std::unique_ptr<m::Plugin> plugin    = m_pluginManager.makePlugin(juceId, sampleRate, bufferSize, m_model.get().sequencer);
	const Plugin*              pluginPtr = plugin.get();
	if (plugin != nullptr)
		m_pluginHost.addPlugin(std::move(plugin));

	/* TODO - unfortunately JUCE wants mutable plugin objects due to the
	    presence of the non-const processBlock() method. Why not const_casting
	    only in the Plugin class? */
	m_model.get().tracks.getChannel(channelId).plugins.push_back(const_cast<Plugin*>(pluginPtr));
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void PluginsApi::swap(const Plugin& p1, const Plugin& p2, ID channelId)
{
	m_pluginHost.swapPlugin(p1, p2, m_model.get().tracks.getChannel(channelId).plugins);
	m_model.swap(model::SwapType::HARD);
}

/* -------------------------------------------------------------------------- */

void PluginsApi::sort(PluginSortMode mode)
{
	m_pluginManager.sortPlugins(mode);
}

/* -------------------------------------------------------------------------- */

void PluginsApi::free(const Plugin& plugin, ID channelId)
{
	u::vector::remove(m_model.get().tracks.getChannel(channelId).plugins, &plugin);
	m_model.swap(model::SwapType::HARD);
	m_pluginHost.freePlugin(plugin);
}

/* -------------------------------------------------------------------------- */

void PluginsApi::setProgram(ID pluginId, int programIndex)
{
	m_pluginHost.setPluginProgram(pluginId, programIndex);
}

/* -------------------------------------------------------------------------- */

void PluginsApi::toggleBypass(ID pluginId)
{
	m_pluginHost.toggleBypass(pluginId);
}

/* -------------------------------------------------------------------------- */

void PluginsApi::setParameter(ID pluginId, int paramIndex, float value)
{
	m_pluginHost.setPluginParameter(pluginId, paramIndex, value);
}

/* -------------------------------------------------------------------------- */

void PluginsApi::scan(const std::string& dir, const std::function<bool(float)>& progress)
{
	m_pluginManager.scanDirs(dir, progress);
	m_pluginManager.saveList(u::fs::join(u::fs::getConfigDirPath(), "plugins.xml"));
}

/* -------------------------------------------------------------------------- */

void PluginsApi::process(mcl::AudioBuffer& outBuf, const std::vector<Plugin*>& plugins, juce::MidiBuffer* events)
{
	m_pluginHost.processStack(outBuf, plugins, events);
}
} // namespace giada::m
