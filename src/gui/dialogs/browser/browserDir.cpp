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

#include "src/gui/dialogs/browser/browserDir.h"
#include "src/gui/elems/basics/imageButton.h"
#include "src/gui/elems/basics/input.h"
#include "src/gui/elems/basics/textButton.h"
#include "src/gui/elems/fileBrowser.h"
#include "src/gui/ui.h"
#include "src/utils/fs.h"

extern giada::v::Ui* g_ui;

namespace giada::v
{
gdBrowserDir::gdBrowserDir(const std::string& title, const std::string& path,
    std::function<void(void*)> cb, const Model& model)
: gdBrowserBase(title, path, cb, 0, model)
{
	hidePathName();

	ok->label(g_ui->getI18Text(LangMap::COMMON_SELECT));
	ok->onClick = [this]()
	{ fireCallback(); };
}
} // namespace giada::v
