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

#ifndef GD_CHANNEL_ROUTING_H
#define GD_CHANNEL_ROUTING_H

#include "src/glue/channel.h"
#include "src/gui/dialogs/window.h"
#include "src/gui/model.h"

namespace giada::v
{
class geVolumeTool;
class gePanTool;
class geCheck;
class geChoice;
class geTextButton;
class geLiquidScroll;
class gdChannelRouting : public gdWindow
{
public:
	gdChannelRouting(ID channelId, const Model&);
	~gdChannelRouting();

	void rebuild() override;

private:
	ID                      m_channelId;
	c::channel::RoutingData m_data;

	geVolumeTool*   m_volume;
	gePanTool*      m_pan;
	geTextButton*   m_close;
	geCheck*        m_sendToMaster;
	geLiquidScroll* m_outputs;
	geChoice*       m_addNewOutput;
};
} // namespace giada::v

#endif
