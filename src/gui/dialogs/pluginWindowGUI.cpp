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

#include "src/gui/dialogs/pluginWindowGUI.h"
#include "src/glue/plugin.h"
#include "src/utils/gui.h"
#include "src/utils/log.h"
#include <FL/platform.H>
#include <cassert>
#if G_OS_MAC
#import "src/utils/cocoa.h" // objective-c
#endif

namespace giada::v
{
gdPluginWindowGUI::gdPluginWindowGUI(const c::plugin::Plugin& p, ID wid)
#if G_OS_MAC
: gdWindow(u::gui::getCenterWinBounds({-1, -1, Fl::w(), Fl::h()}), "", wid)
#else
: gdWindow(u::gui::getCenterWinBounds({-1, -1, 320, 200}), "", wid)
#endif
, m_plugin(p)
{
	setTitle(m_plugin.name);

	createEditor();
	if (m_editor == nullptr)
		return;

	/* Make sure to wait_for_expose() before showing the editor: the window must
	be exposed and visible first. Don't fuck with multithreading! */

	adjustSize();
	show();
	wait_for_expose();
	showEditor();

	Fl::flush();
}

/* -------------------------------------------------------------------------- */

gdPluginWindowGUI::~gdPluginWindowGUI()
{
	closeEditor();
	u::log::print("[gdPluginWindowGUI::__cb_close] GUI closed, this={}\n", (void*)this);
}

/* -------------------------------------------------------------------------- */

void gdPluginWindowGUI::adjustSize()
{
	assert(m_editor != nullptr);

	const int               pluginW   = m_editor->getWidth();
	const int               pluginH   = m_editor->getHeight();
	const geompp::Rect<int> newBounds = getBounds().withSize(pluginW, pluginH).scaled(1 / m_plugin.uiScaling);

#if G_OS_WINDOWS

	/* Dragging around a resized window (this windows will get resized with the
	setBounds() call below) triggers some weird auto-resize mechanism if the
	system UI scale is set to a value greater than 100% (see https://github.com/monocasual/giada/issues/621).
	Setting this window resizable with size_range() seems to fix the issue. */

	size_range(1, 1);
#endif

	setBounds(u::gui::getCenterWinBounds(newBounds.withPosition({-1, -1})));
}

/* -------------------------------------------------------------------------- */

void gdPluginWindowGUI::createEditor()
{
	u::log::print("[gdPluginWindowGUI] Opening editor\n");

	m_editor.reset(m_plugin.createEditor());
	if (m_editor == nullptr)
	{
		u::log::print("[gdPluginWindowGUI::openEditor] unable to create editor!\n");
		return;
	}

	m_editor->setScaleFactor(m_plugin.uiScaling);

	m_plugin.setResizeCallback([this](int w, int h)
	{
		setBounds(getBounds().withSize(w, h));
	});
}

/* -------------------------------------------------------------------------- */

void gdPluginWindowGUI::showEditor()
{
	assert(m_editor != nullptr);

	m_editor->setOpaque(true);

#if G_OS_MAC

	void* cocoaWindow = (void*)fl_xid(this);
	m_editor->addToDesktop(0, cocoa_getViewFromWindow(cocoaWindow));

#else

	m_editor->addToDesktop(0, reinterpret_cast<void*>(fl_xid(this)));

#endif
}

/* -------------------------------------------------------------------------- */

void gdPluginWindowGUI::closeEditor()
{
	assert(m_editor != nullptr);

	m_plugin.setResizeCallback(nullptr);
	m_editor.reset();
}
} // namespace giada::v