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

#ifndef GE_CHANNEL_BUTTON_H
#define GE_CHANNEL_BUTTON_H

#include "src/gui/elems/basics/textButton.h"
#include <FL/Fl_SVG_Image.H>

namespace giada::c::channel
{
struct Data;
}

namespace giada::v
{
class geChannelButton : public geTextButton
{
public:
	geChannelButton(int x, int y, int w, int h, const c::channel::Data& d);

	virtual void refresh();

	void draw() override;

	void setPlayMode();
	void setEndingMode();
	void setDefaultMode(const char* l = nullptr);
	void setInputRecordMode();
	void setActionRecordMode();

protected:
	const c::channel::Data& m_channel;

private:
	std::unique_ptr<Fl_SVG_Image> m_imgExtraOuputs;
};
} // namespace giada::v

#endif
