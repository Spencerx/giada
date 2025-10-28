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

#include "src/gui/dialogs/actionEditor/baseActionEditor.h"
#include "src/glue/channel.h"
#include "src/gui/const.h"
#include "src/gui/drawing.h"
#include "src/gui/elems/actionEditor/gridTool.h"
#include "src/gui/elems/actionEditor/splitScroll.h"
#include "src/gui/elems/basics/choice.h"
#include "src/gui/elems/basics/flex.h"
#include "src/gui/elems/basics/flexResizable.h"
#include "src/gui/elems/basics/imageButton.h"
#include "src/gui/elems/basics/scrollPack.h"
#include "src/gui/graphics.h"
#include "src/gui/ui.h"
#include "src/utils/gui.h"
#include "src/utils/string.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <cassert>
#include <fmt/core.h>
#include <limits>
#include <string>

extern giada::v::Ui* g_ui;

namespace giada::v
{
gdBaseActionEditor::gdBaseActionEditor(ID channelId, const Model& model)
: gdWindow(u::gui::getCenterWinBounds(model.actionEditorBounds), g_ui->getI18Text(LangMap::ACTIONEDITOR_TITLE), WID_ACTION_EDITOR)
, channelId(channelId)
, gridTool(new geGridTool(0, 0, model))
, m_zoomInBtn(new geImageButton(graphics::plusOff, graphics::plusOn))
, m_zoomOutBtn(new geImageButton(graphics::minusOff, graphics::minusOn))
, m_splitScroll(new geSplitScroll(0, 0, 0, 0))
, m_legends(new geFlexResizable(Direction::VERTICAL, geResizerBar::Mode::RESIZE))
, m_ratio(model.actionEditorZoom)
{
	m_zoomInBtn->onClick = [this]()
	{ zoomIn(); };
	m_zoomInBtn->copy_tooltip(g_ui->getI18Text(LangMap::COMMON_ZOOMIN));

	m_zoomOutBtn->onClick = [this]()
	{ zoomOut(); };
	m_zoomOutBtn->copy_tooltip(g_ui->getI18Text(LangMap::COMMON_ZOOMOUT));
}

/* -------------------------------------------------------------------------- */

gdBaseActionEditor::~gdBaseActionEditor()
{
	g_ui->model.actionEditorBounds = getBounds();
	g_ui->model.actionEditorSplitH = m_splitScroll->getTopContentH();
	g_ui->model.actionEditorZoom   = m_ratio;
}

/* -------------------------------------------------------------------------- */

int gdBaseActionEditor::getMouseOverContent() const
{
	return m_splitScroll->getScrollX() + (Fl::event_x() - G_GUI_OUTER_MARGIN - LEGEND_WIDTH);
}

/* -------------------------------------------------------------------------- */

void gdBaseActionEditor::computeWidth(Frame framesInSeq, Frame framesInLoop)
{
	fullWidth = frameToPixel(framesInSeq);
	loopWidth = frameToPixel(framesInLoop);
}

/* -------------------------------------------------------------------------- */

Pixel gdBaseActionEditor::frameToPixel(Frame f) const
{
	return f / m_ratio;
}

Frame gdBaseActionEditor::pixelToFrame(Pixel p, Frame framesInBeat, bool snap) const
{
	return snap ? gridTool->getSnapFrame(p * m_ratio, framesInBeat) : p * m_ratio;
}

/* -------------------------------------------------------------------------- */

void gdBaseActionEditor::zoomIn()
{
	// Explicit type std::max<int> to fix MINMAX macro hell on Windows
	zoomAbout([&r = m_ratio]()
	{ return std::max<int>(r / RATIO_STEP, MIN_RATIO); });
}

/* -------------------------------------------------------------------------- */

void gdBaseActionEditor::zoomOut()
{
	// Explicit type std::max<int> to fix MINMAX macro hell on Windows
	zoomAbout([&r = m_ratio]()
	{ return std::min<int>(r * RATIO_STEP, MAX_RATIO); });
}

/* -------------------------------------------------------------------------- */

void gdBaseActionEditor::prepareWindow()
{
	set_non_modal();
	size_range(640, 284);
	show();
}

/* -------------------------------------------------------------------------- */

void gdBaseActionEditor::updateTitleWithScene(std::size_t scene)
{
	setTitle(fmt::format("{} - {} {}", g_ui->getI18Text(LangMap::ACTIONEDITOR_TITLE),
	    g_ui->getI18Text(LangMap::COMMON_SCENE), scene + 1));
}

/* -------------------------------------------------------------------------- */

int gdBaseActionEditor::handle(int e)
{
	switch (e)
	{
	case FL_MOUSEWHEEL:
		/* Zoom with mousewheel (or two fingers gesture on the mac trackpad) only
		works if Ctrl or Cmd are pressed. */
		if (!Fl::event_command())
			return Fl_Group::handle(e);
		Fl::event_dy() <= 0 ? zoomIn() : zoomOut();
		return 1;
	default:
		return Fl_Group::handle(e);
	}
}

/* -------------------------------------------------------------------------- */

void gdBaseActionEditor::draw()
{
	gdWindow::draw();

	const geompp::Rect splitBounds = m_splitScroll->getBoundsNoScrollbar();
	const geompp::Line playhead    = splitBounds.getHeightAsLine().withX(currentFrameToPixel());

	if (splitBounds.contains(playhead))
		drawLine(playhead, G_COLOR_LIGHT_2);
}

/* -------------------------------------------------------------------------- */

void gdBaseActionEditor::zoomAbout(std::function<float()> f)
{
	const float ratioPrev = m_ratio;
	const int   minWidth  = w() - (G_GUI_OUTER_MARGIN * 2) - LEGEND_WIDTH;

	m_ratio = f();

	/* Make sure the new content width doesn't underflow the window space (i.e.
	the minimum width allowed). */

	if (frameToPixel(m_data.framesInSeq) < minWidth)
	{
		m_ratio = m_data.framesInSeq / static_cast<float>(minWidth);
		m_splitScroll->setScrollX(0);
	}

	/* 1. Store the current x-position, then the new x-position affected by the
	zoom change. */

	const int mpre = getMouseOverContent();
	const int mnow = mpre / (m_ratio / ratioPrev);

	/* 2. Rebuild everything and adjust scrolling given the change occurred in
	the x-position. This effectively centers the view on the mouse cursor. */

	rebuild();
	m_splitScroll->setScrollX(m_splitScroll->getScrollX() + (mnow - mpre));
	redraw();
}

/* -------------------------------------------------------------------------- */

void gdBaseActionEditor::refresh()
{
	redraw();
}

/* -------------------------------------------------------------------------- */

Pixel gdBaseActionEditor::currentFrameToPixel() const
{
	return (frameToPixel(m_data.getCurrentFrame()) + m_splitScroll->x()) - m_splitScroll->getScrollX();
}
} // namespace giada::v