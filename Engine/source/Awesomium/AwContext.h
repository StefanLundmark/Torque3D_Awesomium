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

// Awesomium headers
#include <Awesomium/WebCore.h>
#include <Awesomium/BitmapSurface.h>
#include <Awesomium/STLHelpers.h>

#include "AwManager.h"
#include "console/console.h"
#include "GFX/GFXTextureManager.h"

/*
 *  AwContext
 *  -----------------------------------------------------------------------------------------------
 *	Core functionallity class used internally by both AwShape and AwGui.
 */
class AwContext : public Awesomium::WebViewListener::Load, public Awesomium::WebViewListener::Dialog, public Awesomium::JSMethodHandler
{
	friend class AwManager;

	Awesomium::WebView *mView;								// The associated view.
	U32 mNextUpdateTime;									// The next time we'll fetch a new texture.
	bool mIsTransparent;									// Does this context use transparency?
	String mSessionPath;									// The associated session path which can be used to store different cookies and settings.
	String mCurrentURL;										// The current URL.

	Resource <GBitmap> mCursorBitmap;						// The bitmap of the cursor. Has to contain alpha or it won't be used.
	GFXTexHandle mTexture;									// The most recent texture fetched from Awesomium.
	Point2I mCursorPos;										// The position of the cursor.
	Point2I mCursorRenderPos;								// Interpolated position of the cursor.
	U32 mFramerate;											// The estimated framerate.
	bool mIsPaused;											// Is the context paused and no longer painting?
	bool mShowCursor;										// Should we show the cursor bitmap?
	bool mIsJavaScriptReady;								// When JavaScript has been initialized, this will be set to true.
	bool mRenderedCursorLastFrame;							// If we rendered the cursor the last frame. Is used to force a redraw if the cursor was enabled but no new texture data was generated.

	void blitCursorToTexture (GFXLockedRect *rect);			// Blits the cursor to the texture. Supports 32-bit bitmaps only.
	void copyToTexture ();									// Reads the Awesomium surface and copies it to our texture.
	void initView ();										// Initializes the Awesomium view.

	struct JavaScriptObject
	{
		String name;
		Map <String, Delegate <void (const Vector <String> &)>> delegates;
	};

	Map <U32, JavaScriptObject *> mJavaScriptObjectsById;	// Lookup table used to fetch JavaScriptObjects by their id's. Used internally when a JavaScript method call has been processed.
	Map <String, JavaScriptObject *> mJavaScriptObjectsByName; // Lookup table used to fetch JavaScriptObjects by their names. Used to bind Torque methods to JavaScript methods.

	void onTorqueScript (const Vector <String> &args);		// Called when a TorqueScript method has been called from JavaScript.
	void clearJavaScriptBinds ();							// Clears all JavaScript binds used by the bridge.

	Awesomium::JSValue OnMethodCallWithReturnValue (Awesomium::WebView *view, unsigned int id, const Awesomium::WebString &name, const Awesomium::JSArray &args) { return Awesomium::JSValue (); }
	void OnMethodCall (Awesomium::WebView *view, unsigned int id, const Awesomium::WebString &name, const Awesomium::JSArray &inArgs);
	void OnShowCertificateErrorDialog (Awesomium::WebView *view, bool isOverridable, const Awesomium::WebURL &url, Awesomium::CertError error);
	void OnShowFileChooser (Awesomium::WebView *view, const Awesomium::WebFileChooserInfo &info) {}
	void OnShowLoginDialog (Awesomium::WebView *view, const Awesomium::WebLoginDialogInfo &info) {}
	void OnShowPageInfoDialog (Awesomium::WebView *view, const Awesomium::WebPageInfo &info) {}
	void OnBeginLoadingFrame (Awesomium::WebView *view, int64 frame_id, bool is_main_frame, const Awesomium::WebURL &url, bool is_error_page) ;
	void OnFailLoadingFrame (Awesomium::WebView *view, int64 frame_id, bool is_main_frame, const Awesomium::WebURL &url, int error_code, const Awesomium::WebString &error_desc) {}
	void OnFinishLoadingFrame (Awesomium::WebView *view, int64 frame_id, bool is_main_frame, const Awesomium::WebURL &url) {}
	
	void OnDocumentReady (Awesomium::WebView *view, const Awesomium::WebURL &url);	// Called when the document is ready. We use this to initialize our JavaScript bridge.

	void bindJavaScript (const String &objName, const String &funcName, const Delegate <void (const Vector <String> &)> &delegate);

public:
	void enable ();											// Enables the context, which resumes (unpauses) the view.
	void disable ();										// Disables the context, which pauses the view.
	void unfocus ();										// When focus is lost by the AwShape or AwGui, this gets called.
	void focus ();											// When focus is gained by the AwShape or AwGui, this gets called.
	void loadURL (String url);								// Loads the URL specified. Accepts Torque relative paths but denies access to data outside the game directory.
	String getCurrentURL ();								// Returns the current URL.

	void injectMouseMove (const Point2I &pnt);				// Injects mouse movement and updates the cursor.
	void injectLeftMouseDown ();							// Injects left mouse-down.
	void injectLeftMouseUp ();								// Injects left mouse-up.
	void injectRightMouseDown ();							// Injects right mouse-down.
	void injectRightMouseUp ();								// Injects right mouse-up.
	void injectKeyDown (U8 key, U16 ascii);					// Injects key-down. Accepts Torque keys and/or ascii.
	void injectKeyUp (U8 key);								// Injects key-up.
	void injectMouseWheelDown (F32 amount);					// Injects mouse-wheel-down.
	void injectMouseWheelUp (F32 amount);					// Injects mouse-wheel-up.
	void injectMiddleMouseDown ();							// Injects middle mouse-down.
	void injectMiddleMouseUp ();							// Injects middle mouse-up.

	void copy ();											// Copies the selected text to the clipboard.
	void cut ();											// Removes the selected text and copies it to the clipboard.
	void paste ();											// Copies the text from the clipboard and puts it in the current control.
	void undo ();
	void redo ();											

	void update ();											// Updates the texture if needed.
	bool isLoading ();										// If the document is ready this will return true.
	void reload (bool ignoreCache = false);					// Reloads the view, optionally ignoring the cache.
	void stop ();											// Stops loading which is in progress. Does nothing if the document is ready.			

	void setFramerate (U8 framerate);						// Sets the framerate.
	void setResolution (const Point2I &resolution);			// Sets the resolution and forces a redraw.
	void setSessionPath (const String &sessionPath);		// Sets the session path.
	void setTransparent (bool isTransparent);				// Tells the context that the texture contains opacity information. This consumes additional amounts of memory (~15-25% of the texture's size)
	void setCursorBitmapPath (const String &path);			// Sets the bitmap of the cursor.

	void execJavaScript (const String &script);

	bool isTransparent ();									// Returns true if the texture contains opacity information.
	U8 getAlphaAtPoint (const Point2I &pnt);				// Returns alpha at the given point.
	Point2I getResolution () { return Point2I (mTexture.getWidth (), mTexture.getHeight ()); }
	GFXTexHandle getTexture () { update (); return mTexture; } // Returns the texture after redrawing it.

	void showCursor ();
	void hideCursor () { mShowCursor = false; }
	bool isShowingCursor () { return mShowCursor; }

	void pause ();											// Same as disable () but has additional performance savings used when you want to use the context soon again.
	void resume ();											// Resumes from a paused state.
	bool isPaused ();										// Returns true if the context is paused.

	AwContext ();
	~AwContext ();
};