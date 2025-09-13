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

#include "src/gui/elems/mainWindow/keyboard/channelButton.h"
#include "src/glue/channel.h"
#include "src/gui/const.h"
#include "src/gui/drawing.h"
#include "src/gui/graphics.h"
#include "src/utils/string.h"
#include <FL/fl_draw.H>

namespace giada::v
{
geChannelButton::geChannelButton(int x, int y, int w, int h, const c::channel::Data& d)
: geTextButton(x, y, w, h, "")
, m_channel(d)
, m_imgExtraOuputs(std::make_unique<Fl_SVG_Image>(nullptr, graphics::extraOutputs))
{
}

/* -------------------------------------------------------------------------- */

void geChannelButton::refresh()
{
	switch (m_channel.getPlayStatus())
	{
	case ChannelStatus::OFF:
	case ChannelStatus::EMPTY:
		setDefaultMode();
		break;
	case ChannelStatus::PLAY:
		setPlayMode();
		break;
	case ChannelStatus::ENDING:
		setEndingMode();
		break;
	default:
		break;
	}
	switch (m_channel.getRecStatus())
	{
	case ChannelStatus::ENDING:
		setEndingMode();
		break;
	default:
		break;
	}
}

/* -------------------------------------------------------------------------- */

void geChannelButton::draw()
{
	geTextButton::draw();

	if (m_channel.key != 0)
	{
		const geompp::Rect bounds = geompp::Rect(x(), y(), 20, h()).reduced({1});
		drawRectf(bounds, G_COLOR_GREY_3);
		drawText(std::string(1, m_channel.key), bounds, FL_HELVETICA, G_GUI_FONT_SIZE_BASE, G_COLOR_LIGHT_2);
	}
	if (m_channel.extraOutputsCount > 0)
	{
		const auto parentCenter = geompp::Rect(x(), y(), w(), h()).getCenter();
		const auto bounds       = geompp::Rect(x(), y(), 10, 10).withShiftedX(w() - 16).withVerticalCenter(parentCenter);
		drawImage(*m_imgExtraOuputs, bounds);
	}
}

/* -------------------------------------------------------------------------- */

void geChannelButton::setInputRecordMode()
{
	m_backgroundColorOff = G_COLOR_RED;
}

/* -------------------------------------------------------------------------- */

void geChannelButton::setActionRecordMode()
{
	m_backgroundColorOff = G_COLOR_BLUE;
	m_textColor          = G_COLOR_LIGHT_2;
}

/* -------------------------------------------------------------------------- */

void geChannelButton::setDefaultMode(const char* l)
{
	m_backgroundColorOff = G_COLOR_GREY_2;
	m_borderColor        = G_COLOR_GREY_4;
	m_textColor          = G_COLOR_LIGHT_2;
	if (l != nullptr)
		label(l);
}

/* -------------------------------------------------------------------------- */

void geChannelButton::setPlayMode()
{
	m_backgroundColorOff = G_COLOR_LIGHT_1;
	m_borderColor        = G_COLOR_LIGHT_1;
	m_textColor          = G_COLOR_GREY_1;
}

/* -------------------------------------------------------------------------- */

void geChannelButton::setEndingMode()
{
	m_backgroundColorOff = G_COLOR_GREY_4;
}
} // namespace giada::v
