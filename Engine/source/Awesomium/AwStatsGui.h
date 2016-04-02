// Copyright (c) 2016 Stefan Lundmark (www.stefanlundmark.com)

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "platform/platform.h"
#include "platform/platformInput.h"
#include "gui/core/guiControl.h"
#include "gfx/gfxDrawUtil.h"
#include "console/engineAPI.h"
#include "gui/core/guiCanvas.h"
#include "AwManager.h"

/*
 *  AwStatsGui
 *  -----------------------------------------------------------------------------------------------
 *	Renders statistics for AwShapes while ingame which can be useful
 *  to profile performance and behaviour of the culling feature.
 */
class AwStatsGui : public GuiControl
{
	bool onAdd ();
	void onRemove ();

	static S32 QSORT_CALLBACK AwStatsGui::sortTargets (AwTextureTarget * const *a, AwTextureTarget * const *b);

public:
	void onRender (Point2I, const RectI &);
	DECLARE_CONOBJECT (AwStatsGui);
};