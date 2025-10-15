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

#include "src/core/plugins/pluginManager.h"
#include "src/core/model/model.h"
#include "src/core/patch.h"
#include "src/core/plugins/plugin.h"
#include "src/core/plugins/pluginFactory.h"
#include "src/utils/fs.h"
#include "src/utils/log.h"
#include "src/utils/string.h"
#include <cassert>
#include <cstddef>
#include <memory>

namespace giada::m
{
namespace
{
juce::FileSearchPath toJuceFileSearchPath_(const std::string& dirs)
{
	juce::FileSearchPath searchPath;
	for (const std::string& dir : u::string::split(dirs, ";"))
		searchPath.add(juce::File(dir));
	return searchPath;
}
} // namespace

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void PluginManager::reset()
{
	pluginFactory::reset();

	if (m_formatManager.getNumFormats() == 0) // Must be called only once
		m_formatManager.addDefaultFormats();

	loadList(u::fs::join(u::fs::getConfigDirPath(), "plugins.xml"));
}

/* -------------------------------------------------------------------------- */

int PluginManager::scanDirs(const std::string& dirs, std::function<bool(float)> progressCb)
{
	u::log::print("[pluginManager::scanDir] requested directories: '{}'\n", dirs);
	u::log::print("[pluginManager::scanDir] currently known plug-ins: {}\n", m_knownPluginList.getNumTypes());

	m_knownPluginList.clear(); // clear up previous plugins

	const juce::FileSearchPath searchPath    = toJuceFileSearchPath_(dirs);
	const float                numFormats    = m_formatManager.getNumFormats();
	bool                       shouldRun     = true;
	float                      totalProgress = 1.0f;

	for (juce::AudioPluginFormat* format : m_formatManager.getFormats())
	{
		if (!shouldRun)
			break;

		juce::PluginDirectoryScanner scanner(m_knownPluginList, *format, searchPath, /*recursive=*/true, juce::File());

		juce::String name;
		while (scanner.scanNextFile(false, name) && shouldRun)
		{
			u::log::print("[pluginManager::scanDir]   scanning '{}'\n", name.toRawUTF8());
			shouldRun = progressCb((totalProgress + scanner.getProgress()) / numFormats);
		}

		totalProgress += 1.0f;
	}

	u::log::print("[pluginManager::scanDir] {} plugin(s) found\n", m_knownPluginList.getNumTypes());
	return m_knownPluginList.getNumTypes();
}

/* -------------------------------------------------------------------------- */

bool PluginManager::saveList(const std::string& filepath) const
{
	bool out = m_knownPluginList.createXml()->writeTo(juce::File(filepath));
	if (!out)
		u::log::print("[pluginManager::saveList] unable to save plugin list to {}\n", filepath);
	return out;
}

/* -------------------------------------------------------------------------- */

bool PluginManager::loadList(const std::string& filepath)
{
	std::unique_ptr<juce::XmlElement> elem(juce::XmlDocument::parse(juce::File(filepath)));
	if (elem == nullptr)
		return false;
	m_knownPluginList.recreateFromXml(*elem);
	return true;
}

/* -------------------------------------------------------------------------- */

std::unique_ptr<Plugin> PluginManager::makePlugin(const std::string& juceId,
    int sampleRate, int bufferSize, const model::Sequencer& sequencer, ID id)
{
	std::unique_ptr<juce::AudioPluginInstance> pi = makeJucePlugin(juceId, sampleRate, bufferSize);
	return pluginFactory::create(id, juceId, std::move(pi), sequencer, sampleRate, bufferSize);
}

/* -------------------------------------------------------------------------- */

std::unique_ptr<Plugin> PluginManager::makePlugin(const Plugin& src, int sampleRate,
    int bufferSize, const model::Sequencer& sequencer)
{
	std::unique_ptr<Plugin> p = makePlugin(src.getJuceId(), sampleRate, bufferSize, sequencer);
	if (src.getState().getSize() != 0)
		p->setState(src.getState());
	return p;
}

/* -------------------------------------------------------------------------- */

std::vector<Plugin*> PluginManager::clonePlugins(const std::vector<Plugin*>& source,
    int sampleRate, int bufferSize, model::Model& model)
{
	std::vector<Plugin*> clones;
	for (const Plugin* plugin : source)
		clones.push_back(&model.addPlugin(makePlugin(*plugin, sampleRate, bufferSize, model.get().sequencer)));

	return clones;
}

/* -------------------------------------------------------------------------- */

int PluginManager::countAvailablePlugins() const
{
	return m_knownPluginList.getNumTypes();
}

/* -------------------------------------------------------------------------- */

std::vector<PluginInfo> PluginManager::getPluginsInfo() const
{
	std::vector<PluginInfo> out;

	for (int i = 0; i < m_knownPluginList.getNumTypes(); i++)
	{
		juce::PluginDescription pd = m_knownPluginList.getTypes()[i];
		PluginInfo              pi;

		pi.juceId           = pd.createIdentifierString().toStdString();
		pi.name             = pd.descriptiveName.toStdString();
		pi.category         = pd.category.toStdString();
		pi.manufacturerName = pd.manufacturerName.toStdString();
		pi.format           = pd.pluginFormatName.toStdString();
		pi.isInstrument     = pd.isInstrument;
		pi.exists           = m_formatManager.doesPluginStillExist(*m_knownPluginList.getTypeForIdentifierString(pi.juceId));
		pi.isKnown          = true;
		out.push_back(pi);
	}

	return out;
}

/* -------------------------------------------------------------------------- */

void PluginManager::sortPlugins(PluginSortMode mode)
{
	switch (mode.method)
	{
	case PluginSortMethod::NAME:
		m_knownPluginList.sort(juce::KnownPluginList::SortMethod::sortAlphabetically, mode.dir == PluginSortDir::ASC);
		break;
	case PluginSortMethod::CATEGORY:
		m_knownPluginList.sort(juce::KnownPluginList::SortMethod::sortByCategory, mode.dir == PluginSortDir::ASC);
		break;
	case PluginSortMethod::MANUFACTURER:
		m_knownPluginList.sort(juce::KnownPluginList::SortMethod::sortByManufacturer, mode.dir == PluginSortDir::ASC);
		break;
	case PluginSortMethod::FORMAT:
		m_knownPluginList.sort(juce::KnownPluginList::SortMethod::sortByFormat, mode.dir == PluginSortDir::ASC);
		break;
	}
}

/* -------------------------------------------------------------------------- */

std::unique_ptr<juce::AudioPluginInstance> PluginManager::makeJucePlugin(const std::string& juceId, int sampleRate, int bufferSize)
{
	const std::unique_ptr<juce::PluginDescription> pd = m_knownPluginList.getTypeForIdentifierString(juceId);
	if (pd == nullptr)
	{
		u::log::print("[pluginManager::makeJucePlugin] no plugin found with juceId={}!\n", juceId);
		return nullptr;
	}

	juce::String                               error;
	std::unique_ptr<juce::AudioPluginInstance> pi = m_formatManager.createPluginInstance(*pd, sampleRate, bufferSize, error);
	if (pi == nullptr)
	{
		u::log::print("[pluginManager::makeJucePlugin] unable to create instance with juceId={}! Error: {}\n",
		    juceId, error.toStdString());
		return nullptr;
	}

	u::log::print("[pluginManager::makeJucePlugin] plugin instance with juceId={} created\n", juceId);

	return pi;
}
} // namespace giada::m