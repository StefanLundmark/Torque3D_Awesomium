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

#include "AwGui.h"
#include "AwContext.h"

IMPLEMENT_CONOBJECT (AwGui);

AwGui::AwGui ()
{
	mContext = nullptr;
	mAlphaCutoff = 0;
	mShowLoadingScreen = false;
	mBringToFrontWhenClicked = false;

	mFramerate = 0;
	mIsTransparent = false;
	mResolution.set (0, 0);
	mEnableRightMouseButton = false;
	mUnloadOnSleep = true;
}

void AwGui::initPersistFields ()
{
	addField ("StartURL",				TypeRealString,		Offset (mStartURL, AwGui),					"The URL which is loaded initially.");
	addField ("SessionPath",			TypeRealString,		Offset (mSessionPath, AwGui),				"Path to a session file which will contain cookies, history, passwords etc. A blank path forces the control to use the default session.");
	addField ("Framerate",				TypeS8,				Offset (mFramerate, AwGui),					"The desired amount of frames per second to render. 0 means unlimited.");
	addField ("IsTransparent",			TypeBool,			Offset (mIsTransparent, AwGui),				"Whether this control supports transparency or not. Default: Disabled");
	addField ("Resolution",				TypePoint2I,		Offset (mResolution, AwGui),				"Forced resolution. Defaults to (0, 0) which lets AwGui and AwShape decide. In that case AwGui will set the resolution to the size "
		"of the Gui control and AwShape will set the size to 800 x 600.");
	addField ("UnloadOnSleep",			TypeBool,			Offset (mUnloadOnSleep, AwGui),				"Unloads all resources if the AwGui goes asleep. This can be used to keep the memory footprint down. Default: Enabled");

	addField ("AlphaCutoff",			TypeS8,				Offset (mAlphaCutoff, AwGui),				"If the amount of alpha is below this value, no mouse events will be processed for that pixel.");
	addField ("ShowLoadingScreen",		TypeBool,			Offset (mShowLoadingScreen, AwGui),			"Shows the loading screen if true. Default: Disabled");
	addField ("BringToFrontWhenClicked",TypeBool,			Offset (mBringToFrontWhenClicked, AwGui),	"If enabled will bring the control to the top of the GUI stack when clicked. Default: Disabled");
	addField ("EnableRightMouseButton",	TypeBool,			Offset (mEnableRightMouseButton, AwGui),	"Enables right-mouse clicks. If you're using Flash, this might not be desired as it can bring up its context menu. Default: Disabled");
	Parent::initPersistFields ();
}

bool AwGui::onAdd ()
{
	if (!Parent::onAdd ())
	{
		return false;
	}

	return true;
}

void AwGui::onRemove ()
{
	delete mContext;
	mContext = nullptr;
	Parent::onRemove ();
}

bool AwGui::onWake ()
{
	if (!Parent::onWake ())
	{
		return false;
	}

	mContext = new AwContext;
	mContext->setFramerate (mFramerate);
	mContext->setSessionPath (mSessionPath);
	mContext->setTransparent (mIsTransparent);
	mContext->setResolution (hasForcedResolution () ? mResolution : getExtent ());
	mContext->loadURL (mStartURL);

	if (mContext)
	{
		mContext->enable ();
	}

	return true;
}

void AwGui::onSleep ()
{
	Parent::onSleep ();
	if (mContext)
	{
		if (mUnloadOnSleep)
		{
			delete mContext;
			mContext = nullptr;
		}
		else
		{
			mContext->disable ();
		}
	}
}

void AwGui::inspectPostApply ()
{
	Parent::inspectPostApply ();

	mContext->setFramerate (mFramerate);
	mContext->setSessionPath (mSessionPath);
	mContext->setTransparent (mIsTransparent);
	mContext->setResolution (hasForcedResolution () ? mResolution : getExtent ());
	mContext->loadURL (mStartURL);
}

bool AwGui::resize (const Point2I &newPosition, const Point2I &newExtent)
{
	if (!Parent::resize (newPosition, newExtent))
	{
		return false;
	}

	mContext->setResolution (hasForcedResolution () ? mResolution : getExtent ());

	return true;
}

void AwGui::onRender (Point2I offset, const RectI &updateRect)
{
	// Parent render.
	Parent::onRender (offset, updateRect);

	// If the click was made in a transparent area, make sure all mouse events pass trough to controls behind it.
	// This is very useful for non-rectangular GUI shapes.
	if (mContext->isTransparent ())
	{
		if (cursorInControl ())
		{
			Point2I pnt = globalToLocalCoord (getRoot ()->getCursorPos ());

			if (hasForcedResolution ())
			{
				pnt.x = F32 ((F32)pnt.x / (F32)getWidth ()) * (F32)mContext->getResolution ().x;
				pnt.y = F32 ((F32)pnt.y / (F32)getHeight ()) * (F32)mContext->getResolution ().y;
			}
			mCanHit = mContext->getAlphaAtPoint (pnt) >= mAlphaCutoff;
		}
	}
	else
	{
		mCanHit = true;
	}

	if (mContext->getTexture ())
	{
		GFX->getDrawUtil ()->clearBitmapModulation ();

		// If there is a forced resolution set, we stretch the bitmap across the control.
		if (hasForcedResolution ())
		{
			GFX->getDrawUtil ()->drawBitmapStretch (mContext->getTexture (), updateRect);
		}
		else
		{
			GFX->getDrawUtil ()->drawBitmap (mContext->getTexture (), offset);
		}
	}

	if (mShowLoadingScreen && mContext->isLoading ())
	{
		GFX->getDrawUtil ()->drawRectFill (updateRect, ColorI (96, 96, 96, 196));

		char urlBuffer [128];
		dSprintf (urlBuffer, sizeof (urlBuffer), "Loading %s..", mContext->getCurrentURL ().c_str ());
		offset.x += (getWidth () - mProfile->mFont->getStrWidth ((const UTF8 *)urlBuffer)) / 2;
		offset.y += (getHeight () - mProfile->mFont->getHeight ()) / 2;
		GFX->getDrawUtil ()->setBitmapModulation (ColorI (255, 255, 255));
		GFX->getDrawUtil ()->drawText (mProfile->mFont, offset, urlBuffer);
		GFX->getDrawUtil ()->clearBitmapModulation ();
	}
}

void AwGui::onMouseDown (const GuiEvent &evt)
{
	Parent::onMouseDown (evt);
	mContext->injectLeftMouseDown ();
	if (mBringToFrontWhenClicked && getParent ())
	{
		getParent ()->pushObjectToBack (this);
	}
	setFirstResponder ();
	mouseLock ();
}

void AwGui::onMouseUp (const GuiEvent &evt)
{
	Parent::onMouseUp (evt);
	mContext->injectLeftMouseUp ();
	mouseUnlock ();
}

void AwGui::onMouseMove (const GuiEvent &evt)
{
	Parent::onMouseMove (evt);
	
	Point2I pnt = globalToLocalCoord (evt.mousePoint);

	if (hasForcedResolution ())
	{
		pnt.x = F32 ((F32)pnt.x / (F32)getWidth ()) * (F32)mContext->getResolution ().x;
		pnt.y = F32 ((F32)pnt.y / (F32)getHeight ()) * (F32)mContext->getResolution ().y;
	}

	mContext->injectMouseMove (pnt);
}

void AwGui::onMouseDragged (const GuiEvent &evt)
{
	onMouseMove (evt);
}

bool AwGui::onKeyDown (const GuiEvent &evt)
{
	Parent::onKeyDown (evt);

	// Handle copy and paste here
	if (evt.modifier & SI_CTRL)
	{
		if (evt.keyCode == KEY_X)
		{
			mContext->cut (); 
			return true;
		}
		else if (evt.keyCode == KEY_C)
		{
			mContext->copy (); 
			return true;
		}
		else if (evt.keyCode == KEY_V)
		{
			mContext->paste (); 
			return true;
		}
		else if (evt.keyCode == KEY_Z)
		{
			mContext->undo (); 
			return true;
		}
		else if (evt.keyCode == KEY_Y)
		{
			mContext->redo (); 
			return true;
		}
	}
	
	mContext->injectKeyDown (evt.keyCode, evt.ascii);
	return true;
}

bool AwGui::onKeyUp (const GuiEvent &evt)
{
	Parent::onKeyUp (evt);
	mContext->injectKeyUp (evt.keyCode);
	return true;
}

bool AwGui::onKeyRepeat (const GuiEvent &evt)
{
	return onKeyDown (evt);
}

bool AwGui::onMouseWheelUp (const GuiEvent &evt)
{
	Parent::onMouseWheelUp (evt);
	mContext->injectMouseWheelUp (evt.fval);
	return true;
}

bool AwGui::onMouseWheelDown (const GuiEvent &evt)
{
	Parent::onMouseWheelDown (evt);
	mContext->injectMouseWheelDown (evt.fval);
	return true;
}

void AwGui::onRightMouseDown (const GuiEvent &evt)
{
	if (!mEnableRightMouseButton)
	{
		return;
	}

	Parent::onRightMouseDown (evt);
	mContext->injectRightMouseDown ();
	if (mBringToFrontWhenClicked && getParent ())
	{
		getParent ()->pushObjectToBack (this);
	}
	setFirstResponder ();
	mouseLock ();
}

void AwGui::onRightMouseUp (const GuiEvent &evt)
{
	if (!mEnableRightMouseButton)
	{
		return;
	}
	Parent::onRightMouseUp (evt);
	mContext->injectRightMouseUp ();
	mouseUnlock ();
}

void AwGui::onRightMouseDragged (const GuiEvent &evt)
{
	onMouseMove (evt);
}

void AwGui::onMiddleMouseDown (const GuiEvent &evt)
{
	Parent::onMiddleMouseDown (evt);
	mContext->injectMiddleMouseDown ();
	if (mBringToFrontWhenClicked && getParent ())
	{
		getParent ()->pushObjectToBack (this);
	}
	setFirstResponder ();
	mouseLock ();
}

void AwGui::onMiddleMouseUp (const GuiEvent &evt)
{
	Parent::onMiddleMouseUp (evt);
	mContext->injectMiddleMouseUp ();
	mouseUnlock ();
}

void AwGui::onMiddleMouseDragged (const GuiEvent &evt)
{
	onMouseMove (evt);
}

void AwGui::setFirstResponder ()
{
	Parent::setFirstResponder ();

	GuiCanvas *root = getRoot ();
	if (root)
	{
		root->enableKeyboardTranslation ();
		root->setNativeAcceleratorsEnabled (false);
	}
}

void AwGui::onGainFirstResponder ()
{
	Parent::onGainFirstResponder ();
	if (mContext)
	{
		mContext->focus ();
	}
}

void AwGui::onLoseFirstResponder ()
{
	GuiCanvas *root = getRoot ();
	if (root)
	{
		root->setNativeAcceleratorsEnabled (true);
		root->disableKeyboardTranslation ();
	}

	// Lost Responder
	Parent::onLoseFirstResponder ();
	if (mContext)
	{
		mContext->unfocus ();
	}
}

void AwGui::loadURL (const String &url) 
{
	mContext->loadURL (url);
}

String AwGui::getCurrentURL () 
{ 
	return mContext->getCurrentURL (); 
}

void AwGui::reload (bool ignoreCache) 
{ 
	mContext->reload (ignoreCache); 
}

void AwGui::stop () 
{ 
	mContext->stop (); 
}

bool AwGui::isLoading () 
{ 
	return mContext->isLoading (); 
}

void AwGui::execJavaScript (const String &script)
{
	mContext->execJavaScript (script);
}

DefineEngineMethod (AwGui, execJavaScript, void, (const char *script),, "")
{
	object->execJavaScript (script);
}

DefineEngineMethod (AwGui, loadURL, void, (const char *url),, "@brief Loads the specified URL.")
{
	object->loadURL (url);
}

DefineEngineMethod (AwGui, getStartURL, const char *, (),, "@brief Returns the start URL.")
{
	return object->getStartURL ();
}

DefineEngineMethod (AwGui, getCurrentURL, const char *, (),, "@brief Returns the current URL.")
{
	return object->getCurrentURL ();
}

DefineEngineMethod (AwGui, reload, void, (),, "@brief Refreshes the current page.")
{
	object->reload ();
}

DefineEngineMethod (AwGui, stop, void, (),, "@brief If there's a page being loaded, this stops it from doing so.")
{
	object->stop ();
}