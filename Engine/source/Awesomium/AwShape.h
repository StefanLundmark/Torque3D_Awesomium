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

#include "scene/sceneObject.h"
#include "collision/convex.h"
#include "core/resource.h"
#include "sim/netStringTable.h"
#include "ts/tsShape.h"

#include "T3D/TSStatic.h"

class AwTextureTarget;
class AwContext;

/*
 *  AwShape
 *  -----------------------------------------------------------------------------------------------
 *	Renders an Awesomium surface onto a AwShape.
 *	Each shape is associated with a AwTextureTarget.
 */
class AwShape : public TSStatic
{
	typedef TSStatic Parent;

	friend class AwManager;

	static AwShape *sMouseInputShape;
	BaseMatInstance *mMatInstance;
	AwTextureTarget *mTextureTarget;
	bool mIsMouseDown;																	// Used to track if a mouse button has been used.

	void onGainMouseInput ();															// When mouse input is gained this gets called. Is used to play a sound.
	void onLoseMouseInput ();															// When mouse input is lost this gets called. Is used to play a sound.

public:
	void updateMapping ();
	AwTextureTarget *processAwesomiumHit (const Point3F &start, const Point3F &end);	// Processes a hit and injects the appropriate mouse events on the context.
	void execJavaScript (const String &script);											// Executes JavaScript for this AwShape.
	void setIsMouseDown (bool isMouseDown);
	bool onAdd ();
	void onRemove ();
	void onResourceChanged (const Torque::Path &path);									// Gets called when the resource associated with this AwShape changes.
	AwTextureTarget *getTextureTarget () { return mTextureTarget; }						// Returns the AwTextureTarget associated with this AwShape.

	static void setMouseInputShape (AwShape *shape);									// Sets this AwShape to be accepting mouse input.
	static AwShape *getMouseInputShape () { return sMouseInputShape; }					// Returns the AwShape which currently has input focus.

	AwShape ();
	DECLARE_CONOBJECT (AwShape);
};

