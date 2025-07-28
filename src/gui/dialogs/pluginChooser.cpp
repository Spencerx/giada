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

#include "src/gui/dialogs/pluginChooser.h"
#include "src/glue/plugin.h"
#include "src/gui/elems/basics/box.h"
#include "src/gui/elems/basics/choice.h"
#include "src/gui/elems/basics/flex.h"
#include "src/gui/elems/basics/textButton.h"
#include "src/gui/elems/plugin/pluginBrowser.h"
#include "src/gui/ui.h"
#include "src/utils/gui.h"

extern giada::v::Ui* g_ui;

namespace giada::v
{
gdPluginChooser::gdPluginChooser(ID channelId, const Model& model)
: gdWindow(u::gui::getCenterWinBounds(model.pluginChooserBounds), g_ui->getI18Text(LangMap::PLUGINCHOOSER_TITLE), WID_FX_CHOOSER)
, m_channelId(channelId)
{
	geFlex* container = new geFlex(getContentBounds().reduced({G_GUI_OUTER_MARGIN}), Direction::VERTICAL, G_GUI_OUTER_MARGIN);
	{
		geFlex* header = new geFlex(Direction::HORIZONTAL, G_GUI_INNER_MARGIN);
		{
			sortMethod = new geChoice(g_ui->getI18Text(LangMap::PLUGINCHOOSER_SORTBY), 0);
			sortDir    = new geChoice();
			header->addWidget(sortMethod, 180);
			header->addWidget(sortDir, 40);
			header->addWidget(new geBox());
			header->end();
		}

		browser = new v::gePluginBrowser();

		geFlex* footer = new geFlex(Direction::HORIZONTAL, G_GUI_OUTER_MARGIN);
		{
			addBtn    = new geTextButton(g_ui->getI18Text(LangMap::COMMON_ADD));
			cancelBtn = new geTextButton(g_ui->getI18Text(LangMap::COMMON_CANCEL));
			footer->addWidget(new geBox());
			footer->addWidget(cancelBtn, 80);
			footer->addWidget(addBtn, 80);
			footer->end();
		}

		container->addWidget(header, G_GUI_UNIT);
		container->addWidget(browser);
		container->addWidget(footer, G_GUI_UNIT);
		container->end();
	}

	add(container);
	resizable(container);

	sortMethod->addItem(g_ui->getI18Text(LangMap::PLUGINCHOOSER_SORTBY_NAME));
	sortMethod->addItem(g_ui->getI18Text(LangMap::PLUGINCHOOSER_SORTBY_CATEGORY));
	sortMethod->addItem(g_ui->getI18Text(LangMap::PLUGINCHOOSER_SORTBY_MANUFACTURER));
	sortMethod->addItem(g_ui->getI18Text(LangMap::PLUGINCHOOSER_SORTBY_FORMAT));
	sortMethod->showItem(static_cast<int>(model.pluginChooserSortMode.method));
	sortMethod->onChange = [this](ID)
	{
		c::plugin::sortPlugins(getSortMode());
		browser->rebuild();
	};

	sortDir->addItem("A-Z");
	sortDir->addItem("Z-A");
	sortDir->showItem(static_cast<int>(model.pluginChooserSortMode.dir));
	sortDir->onChange = [this](ID)
	{
		c::plugin::sortPlugins(getSortMode());
		browser->rebuild();
	};

	browser->onClickRow = [this](int row, int /*col*/, bool doubleClick)
	{
		if (doubleClick)
			c::plugin::addPlugin(row, m_channelId);
	};

	addBtn->onClick = [this]()
	{
		const auto selection = browser->getSelection();
		if (!selection.isValid())
			return;
		c::plugin::addPlugin(selection.a, m_channelId);
		do_callback();
	};
	addBtn->shortcut(FL_Enter);

	cancelBtn->onClick = [this]()
	{
		do_callback();
	};

	show();
}

/* -------------------------------------------------------------------------- */

gdPluginChooser::~gdPluginChooser()
{
	g_ui->model.pluginChooserBounds   = getBounds();
	g_ui->model.pluginChooserSortMode = getSortMode();
}

/* -------------------------------------------------------------------------- */

m::PluginManager::SortMode gdPluginChooser::getSortMode() const
{
	return {
	    static_cast<m::PluginManager::SortMethod>(sortMethod->getSelectedId()),
	    static_cast<m::PluginManager::SortDir>(sortDir->getSelectedId())};
}
} // namespace giada::v