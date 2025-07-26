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

#include "src/gui/elems/plugin/pluginBrowser.h"
#include "src/glue/plugin.h"
#include "src/gui/const.h"
#include "src/gui/elems/basics/boxtypes.h"
#include "src/gui/ui.h"
#include "src/utils/gui.h"
#include <fmt/core.h>

extern giada::v::Ui* g_ui;

namespace giada::v
{
gePluginBrowser::gePluginBrowser(int x, int y, int w, int h)
: Fl_Browser(x, y, w, h)
, m_widths{0}
{
	box(G_CUSTOM_BORDER_BOX);
	textsize(G_GUI_FONT_SIZE_BASE);
	textcolor(G_COLOR_LIGHT_2);
	selection_color(G_COLOR_GREY_4);
	color(G_COLOR_GREY_2);

	this->scrollbar.color(G_COLOR_GREY_2);
	this->scrollbar.selection_color(G_COLOR_GREY_4);
	this->scrollbar.labelcolor(G_COLOR_LIGHT_1);
	this->scrollbar.slider(G_CUSTOM_BORDER_BOX);

	this->hscrollbar.color(G_COLOR_GREY_2);
	this->hscrollbar.selection_color(G_COLOR_GREY_4);
	this->hscrollbar.labelcolor(G_COLOR_LIGHT_1);
	this->hscrollbar.slider(G_CUSTOM_BORDER_BOX);

	type(FL_HOLD_BROWSER);

	computeWidths();

	column_widths(m_widths);
	column_char('\t'); // tabs as column delimiters

	refresh();

	end();
}

/* -------------------------------------------------------------------------- */

void gePluginBrowser::refresh()
{
	clear();

	add(g_ui->getI18Text(LangMap::PLUGINCHOOSER_HEADER));
	add("---\t---\t---\t---\t---");

	for (m::PluginManager::PluginInfo pi : c::plugin::getPluginsInfo())
	{
		std::string s;
		if (pi.isKnown)
		{
			std::string m = pi.exists ? "" : "@-";

			s = fmt::format("{0}{1}\t{0}{2}\t{0}{3}\t{0}{4}\t{0}{5}",
			    m, pi.name, pi.manufacturerName, pi.category, pi.format, pi.juceId);
		}
		else
			s = fmt::format("?\t?\t?\t?\t? {} ?", pi.juceId);

		add(s.c_str());
	}
}

/* -------------------------------------------------------------------------- */

void gePluginBrowser::computeWidths()
{
	constexpr int PADDDING = 60;

	for (m::PluginManager::PluginInfo pi : c::plugin::getPluginsInfo())
	{
		// Explicit type std::max<int> to fix MINMAX macro hell on Windows
		m_widths[0] = std::max<int>(u::gui::getStringRect(pi.name).w, m_widths[0]);
		m_widths[1] = std::max<int>(u::gui::getStringRect(pi.manufacturerName).w, m_widths[1]);
		m_widths[2] = std::max<int>(u::gui::getStringRect(pi.category).w, m_widths[2]);
		m_widths[3] = std::max<int>(u::gui::getStringRect(pi.format).w, m_widths[3]);
	}
	m_widths[0] += PADDDING;
	m_widths[1] += PADDDING;
	m_widths[2] += PADDDING;
	m_widths[3] += PADDDING;
	m_widths[4] = 0;
}
} // namespace giada::v