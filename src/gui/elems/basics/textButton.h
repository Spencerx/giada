/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * geButton
 * A regular button.
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

#ifndef GE_TEXT_BUTTON_H
#define GE_TEXT_BUTTON_H

#include "src/gui/elems/basics/button.h"

namespace giada::v
{
class geTextButton : public geButton
{
public:
	geTextButton(int x, int y, int w, int h, const char* l);
	geTextButton(const char* l);

	void draw() override;

	void setPadding(int);

protected:
	Fl_Color m_backgroundColorOff;
	Fl_Color m_backgroundColorOn;
	Fl_Color m_borderColor;
	Fl_Color m_textColor;

private:
	int m_padding;
};
} // namespace giada::v

#endif
