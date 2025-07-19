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

#include "src/gui/ui.h"
#include "src/gui/const.h"
#include "src/gui/dialogs/warnings.h"
#include "src/gui/elems/mainWindow/keyboard/keyboard.h"
#include "src/gui/elems/mainWindow/keyboard/track.h"
#include "src/gui/elems/mainWindow/mainInput.h"
#include "src/gui/elems/mainWindow/mainOutput.h"
#include "src/gui/elems/mainWindow/mainTimer.h"
#include "src/gui/updater.h"
#include "src/utils/gui.h"
#include "src/utils/log.h"
#include <FL/Fl.H>
#include <FL/Fl_Tooltip.H>
#if G_OS_LINUX || G_OS_FREEBSD
#include <X11/Xlib.h> // For XInitThreads
#endif

namespace giada::v
{
ID Ui::getPluginWindowId(ID pluginId)
{
	return WID_FX + pluginId;
}

/* -------------------------------------------------------------------------- */

Ui::Ui()
: m_updater(*this)
, m_blinker(0)
{
}

/* -------------------------------------------------------------------------- */

bool Ui::shouldBlink() const
{
	return m_blinker > BLINK_RATE / 2;
}

/* -------------------------------------------------------------------------- */

float Ui::getScaling() const
{
	// return 1.0;
	return Fl::screen_scale(mainWindow->screen_num());
}

/* -------------------------------------------------------------------------- */

const char* Ui::getI18Text(const std::string& key) const
{
	return m_langMapper.get(key);
}

/* -------------------------------------------------------------------------- */

const std::vector<std::string>& Ui::getLangMapFilesFound() const
{
	return m_langMapper.getMapFilesFound();
}

/* -------------------------------------------------------------------------- */

void Ui::load(const m::Patch& patch)
{
	reset();

	model.load(patch);

	mainWindow->keyboard->rebuild();
	mainWindow->setTitle(patch.name);
}

/* -------------------------------------------------------------------------- */

void Ui::init(const m::Conf& conf, const std::string& patchName, bool isAudioReady)
{
	model.load(conf);

	/* This is of paramount importance on Linux with VST enabled, otherwise many
	plug-ins go nuts and crash hard. It seems that some plug-ins on our Juce-based
	PluginHost use Xlib concurrently. */

#if G_OS_LINUX || G_OS_FREEBSD
	XInitThreads();
#endif

	m_langMapper.init();
	m_langMapper.read(model.langMap);

	mainWindow = std::make_unique<gdMainWindow>(u::gui::getCenterWinBounds(model.mainWindowBounds), "");
	mainWindow->setTitle(patchName == "" ? G_DEFAULT_PATCH_NAME : patchName);

	if (Fl::screen_scaling_supported() && model.uiScaling != G_DEFAULT_UI_SCALING)
		Fl::screen_scale(mainWindow->screen_num(), model.uiScaling);

	Fl_Tooltip::color(G_COLOR_GREY_1);
	Fl_Tooltip::textcolor(G_COLOR_LIGHT_2);
	Fl_Tooltip::size(G_GUI_FONT_SIZE_BASE);
	Fl_Tooltip::enable(conf.showTooltips);

	dispatcher.init(*mainWindow, model);
	m_updater.start();

	rebuildStaticWidgets();
	startJuceDispatchLoop();

	if (!isAudioReady)
		v::gdAlert(m_langMapper.get(v::LangMap::MESSAGE_INIT_WRONGSYSTEM));
}

/* -------------------------------------------------------------------------- */

void Ui::reset()
{
	model.projectName = "";
	mainWindow->setTitle(G_DEFAULT_PATCH_NAME);
	rebuildStaticWidgets();
	closeAllSubwindows();
	mainWindow->clearKeyboard();
	mainWindow->rebuild();
}

/* -------------------------------------------------------------------------- */

void Ui::run()
{
	Fl::lock(); // Enable multithreading in FLTK
	m_updater.run();
}

/* -------------------------------------------------------------------------- */

void Ui::shutdown(m::Conf& conf)
{
	model.store(conf);

	mainWindow.reset();
	m_updater.stop();

	stopJuceDispatchLoop();

	u::log::print("[ui] All windows closed\n");
}

/* -------------------------------------------------------------------------- */

void Ui::stopUpdater() { m_updater.stop(); }
void Ui::startUpdater() { m_updater.start(); }

/* -------------------------------------------------------------------------- */

bool Ui::pumpEvent(const Updater::Event& e)
{
	return m_updater.pumpEvent(e);
}

/* -------------------------------------------------------------------------- */

void Ui::refresh()
{
	/* Update dynamic elements inside main window: in and out meters, beat meter
	and each channel. */

	mainWindow->refresh();

	/* Compute timer for blinker. */

	m_blinker = (m_blinker + 1) % BLINK_RATE;

	/* Refresh Sample Editor and Action Editor for dynamic playhead. */

	refreshSubWindow(WID_SAMPLE_EDITOR);
	refreshSubWindow(WID_ACTION_EDITOR);
}

/* -------------------------------------------------------------------------- */

void Ui::rebuild()
{
	mainWindow->rebuild();
	rebuildSubWindow(WID_FX_LIST);
	rebuildSubWindow(WID_SAMPLE_EDITOR);
	rebuildSubWindow(WID_ACTION_EDITOR);
	rebuildSubWindow(WID_CHANNEL_ROUTING);
}

/* -------------------------------------------------------------------------- */

void Ui::rebuildSubWindow(ID wid)
{
	v::gdWindow* w = getSubwindow(wid);
	if (w != nullptr) // If its open
		w->rebuild();
}

/* -------------------------------------------------------------------------- */

void Ui::refreshSubWindow(ID wid)
{
	v::gdWindow* w = getSubwindow(wid);
	if (w != nullptr) // If its open
		w->refresh();
}

/* -------------------------------------------------------------------------- */

v::gdWindow* Ui::getSubwindow(ID wid)
{
	return mainWindow->getChild(wid);
}

/* -------------------------------------------------------------------------- */

void Ui::openSubWindow(v::gdWindow* child)
{
	mainWindow->addChild(child);
}

/* -------------------------------------------------------------------------- */

void Ui::closeSubWindow(ID wid)
{
	mainWindow->deleteChild(wid);
}

/* -------------------------------------------------------------------------- */

void Ui::closeAllSubwindows()
{
	mainWindow->deleteChildren();
}

/* -------------------------------------------------------------------------- */

void Ui::setMainWindowTitle(const std::string& title)
{
	mainWindow->setTitle(title);
}

/* -------------------------------------------------------------------------- */

void Ui::startJuceDispatchLoop()
{
	Fl::add_timeout(G_GUI_REFRESH_RATE, juceDispatchLoop);
}

void Ui::stopJuceDispatchLoop()
{
	Fl::remove_timeout(juceDispatchLoop);
}

/* -------------------------------------------------------------------------- */

void Ui::rebuildStaticWidgets()
{
	mainWindow->mainInput->rebuild();
	mainWindow->mainOutput->rebuild();
	mainWindow->mainTimer->rebuild();
}

/* -------------------------------------------------------------------------- */

void Ui::juceDispatchLoop(void*)
{
	juce::MessageManager* mm = juce::MessageManager::getInstanceWithoutCreating();
	assert(mm != nullptr);
	mm->runDispatchLoopUntil(1);
	Fl::add_timeout(G_GUI_REFRESH_RATE, juceDispatchLoop);
}
} // namespace giada::v
