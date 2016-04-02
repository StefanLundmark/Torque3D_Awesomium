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

#include "AwStatsGui.h"
#include "AwContext.h"
#include "AwShape.h"
#include "AwTextureTarget.h"

IMPLEMENT_CONOBJECT (AwStatsGui);

bool AwStatsGui::onAdd ()
{
	if (!Parent::onAdd ())
	{
		return false;
	}

	return true;
}

void AwStatsGui::onRemove ()
{
	Parent::onRemove ();
}

S32 QSORT_CALLBACK AwStatsGui::sortTargets (AwTextureTarget * const *a, AwTextureTarget * const *b)
{
	if ((*a)->getActualFramerate () < (*b)->getActualFramerate ())
	{
		return 1;
	}
	else if ((*a)->getActualFramerate () > (*b)->getActualFramerate ())
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

void AwStatsGui::onRender (Point2I offset, const RectI &updateRect)
{
	GuiControl::onRender (offset, updateRect);

	// Gather and sort our list
	Vector <AwTextureTarget *> targets = AwManager::getTargets ();
	targets.sort (sortTargets);

	if (AwTextureTarget::getMouseInputTarget ())
	{
		String line = AwTextureTarget::getMouseInputTarget ()->getName ();
		GFX->getDrawUtil ()->setBitmapModulation (ColorI (255, 128, 64));
		GFX->getDrawUtil ()->drawText (mProfile->mFont, offset, line.c_str ());
		GFX->getDrawUtil ()->clearBitmapModulation ();
		offset.y += mProfile->mFont->getHeight ();
	}

	// Loop trough all active shapes and gather some information about them that we can use to show statistics.
	for (U32 i = 0; i < targets.size (); i++)
	{
		AwTextureTarget *target = targets [i];
		if (AwTextureTarget::getMouseInputTarget () == target || target->isPaused ())
		{
			continue;
		}
		String line = target->getName ();
		U32 framerate = target->getActualFramerate ();
		if (target->isSingleFrame ())
		{
			line += "   |   [Single Frame]";
		}
		else
		{
			line += "   |   [FPS: " + String::ToString ("%i", framerate) + "]";
		}

		line += "   (Refs: " + String::ToString ("%i", target->getRefCount ()) + ")";
		GFX->getDrawUtil ()->setBitmapModulation (ColorI (255, 255, 255));
		GFX->getDrawUtil ()->drawText (mProfile->mFont, offset, line.c_str ());
		GFX->getDrawUtil ()->clearBitmapModulation ();
		offset.y += mProfile->mFont->getHeight ();
	}
}