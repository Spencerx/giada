/* -----------------------------------------------------------------------------
 *
 * Giada - Your Hardcore Loopmachine
 *
 * geScroll
 * Custom scroll with nice scrollbars and something else.
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

#ifndef GE_SCROLL_H
#define GE_SCROLL_H

#include "src/deps/geompp/src/rect.hpp"
#include <FL/Fl_Scroll.H>
#include <functional>

namespace giada::v
{
class geScroll : public Fl_Scroll
{
public:
	int  children() const = delete; // Use countChildren()
	void clear()          = delete; // use removeAllChildren();

	geScroll(int x, int y, int w, int h, int type = Fl_Scroll::BOTH);
	geScroll(int type = Fl_Scroll::BOTH);

	int countChildren() const;

	/* getViewportBounds
	Returns the viewport bounds without scrollbars taken into account. */

	geompp::Rect<int> getViewportBounds() const;

	int handle(int e) override;

	void removeAllChildren();

	std::function<void(int)> onScrollV{nullptr};
	std::function<void(int)> onScrollH{nullptr};

	bool autoscroll;

private:
	static void cb_onScrollV(Fl_Widget* w, void* p);
	static void cb_onScrollH(Fl_Widget* w, void* p);
	void        cb_onScrollV();
	void        cb_onScrollH();

	void doAutoscroll();
};
} // namespace giada::v

#endif
