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

class AwContext;

/*
 *  AwGui
 *  -----------------------------------------------------------------------------------------------
 *	Renders an Awesomium surface onto a graphical user interface control.
 *  Supports both mouse and keyboard input.
 */
class AwGui : public GuiControl
{
	typedef GuiControl Parent;

	AwContext *mContext;											// The associated context.
	bool mShowLoadingScreen;										// Should we show a loading screen when the document isn't ready? Can be customized in TorqueScript. Defaults to disabled.
	bool mBringToFrontWhenClicked;									// If enabled will bring the control to the top of the GUI stack when clicked. Defaults to disabled.
	U8 mAlphaCutoff;												// If the amount of alpha is below this value, no mouse events will be processed for that pixel.
	Point2I mResolution;											// Forced resolution. Defaults to (0, 0) which lets AwGui and AwShape decide. In that case AwGui will set the resolution to the size of the Gui control and AwShape will set the size to 800 x 600.
	String mStartURL;												// The URL which is loaded initially.
	String mSessionPath;											// Path to a session file which will contain cookies, history, passwords etc. A blank path forces the control to use the default session.
	bool mUnloadOnSleep;											// Unloads all resources if the AwGui goes asleep. This can be used to keep the memory footprint down. Defaults to enabled.
	bool mIsTransparent;											// Whether this control supports transparency or not. Defaults to disabled.
	U8 mFramerate;													// The desired amount of frames per second to render. 0 means unlimited.
	bool mEnableRightMouseButton;									// Enables right-mouse clicks. If you're using Flash, this might not be desired as it can bring its context menu. Defaults to disabled.

	bool onAdd ();
	void onRemove ();

	void onMouseDown (const GuiEvent &evt);							// Called when the left mouse button is pressed.
	void onMouseUp (const GuiEvent &evt);							// Called when the left mouse button is depressed.
	void onMouseMove (const GuiEvent &evt);							// Called when the mouse moved and no buttons are pressed.
	bool onKeyDown (const GuiEvent &evt);							// Called when a key is pressed.
	bool onKeyUp (const GuiEvent &evt);								// Called when a key is depressed.
	bool onKeyRepeat (const GuiEvent &evt);							// Called every x milliseconds when the key is pressed.
	void onMouseDragged (const GuiEvent &evt);						// Called when the mouse is moved and the left mouse button is pressed.
	bool onMouseWheelUp (const GuiEvent &evt);						// Called when the mouse wheel is spinned upwards.
	bool onMouseWheelDown (const GuiEvent &evt);					// Called when the mouse wheel is spinned downwards.
      
	void onRightMouseDown (const GuiEvent &evt);					// Called when the right mouse button is pressed.
	void onRightMouseUp (const GuiEvent &evt);						// Called when the right mouse button is depressed.
	void onRightMouseDragged (const GuiEvent &evt);					// Called when the mouse is moved and the right mouse button is pressed.
      
	void onMiddleMouseDown (const GuiEvent &evt);					// Called when the middle mouse button is pressed.
	void onMiddleMouseUp (const GuiEvent &evt);						// Called when the midle mouse button is depressed.
	void onMiddleMouseDragged (const GuiEvent &evt);				// Called when the mouse is moved and the middle mouse button is pressed.

	bool onWake ();
	void onSleep ();
	void inspectPostApply ();
	bool resize (const Point2I &newPosition, const Point2I &newExtent);	// Redraws the context and resizes the control.

	void setFirstResponder ();

	void onGainFirstResponder ();
	void onLoseFirstResponder ();

	bool hasForcedResolution () { return mResolution.x != 0 && mResolution.y != 0; }

public:
	AwGui ();

	void onRender (Point2I, const RectI &);
	static void initPersistFields ();

	void execJavaScript (const String &script);
	void loadURL (const String &url);
	String getCurrentURL ();											// Returns the current URL, which might or might not have changed from the one which was specified when the control was created.
	String getStartURL () { return mStartURL; }							// Returns the URL which was specified when the control was created.
	void reload (bool ignoreCache = true);								// Reloads the context, optionally ignoring the cache.
	void stop ();														// Stops loading which is in progress. Does nothing if the document is ready.
	bool isLoading ();													// If the document is ready this will return true.

	DECLARE_CONOBJECT (AwGui);
};