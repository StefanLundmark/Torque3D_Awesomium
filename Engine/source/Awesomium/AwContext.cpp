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

#include "AwContext.h"
#include "AwManager.h"
#include "Core/Stream/FileStream.h"
#include "GFX/GFXTextureManager.h"

AwContext::AwContext ()
{
	mView = nullptr;
	mIsPaused = false;
	mFramerate = 0;
	mNextUpdateTime = 0;
	mCursorPos.set (0, 0);
	mCursorRenderPos.set (0, 0);
	mShowCursor = false;
	mIsJavaScriptReady = false;
	mRenderedCursorLastFrame = false;
	mCursorBitmap = GBitmap::load ("Awesomium/defaultCursor.png");
}

AwContext::~AwContext ()
{
	if (mView)
	{
		mView->Destroy ();
	}
}

void AwContext::blitCursorToTexture (GFXLockedRect *rect)
{
	// We only support bitmaps with alpha.
	if (mCursorBitmap->getBytesPerPixel () != 4)
	{
		return;
	}

	const U8 *bits = mCursorBitmap->getBits ();
	for (U32 y = 0; y < mCursorBitmap->getHeight (); y++)
	{
		for (U32 x = 0; x < mCursorBitmap->getWidth (); x++)
		{
			U32 index = ((mCursorPos.y + y) * rect->pitch) + (mCursorPos.x + x) * mTexture->getFormatByteSize ();
			U32 cursorIndex = (x + (y * mCursorBitmap->getWidth ())) * 4;

			if ((mCursorPos.x + x) < mTexture->getWidth () && (mCursorPos.y + y) < mTexture->getHeight ())
			{
				F32 mod = (F32)bits [cursorIndex + 3] / 255.0f;
				F32 invMod = 1.0f - mod;
				rect->bits [index]			= (rect->bits [index] * invMod) + (bits [cursorIndex + 2] * mod);
				rect->bits [index + 1]		= (rect->bits [index + 1] * invMod) + (bits [cursorIndex + 1] * mod);
				rect->bits [index + 2]		= (rect->bits [index + 2] * invMod) + (bits [cursorIndex] * mod);
				rect->bits [index + 3]		= (rect->bits [index + 3] * invMod) + (bits [cursorIndex + 3] * mod);
			}
		}
	}
}

void AwContext::copyToTexture ()
{
	// If the view has crashed, destroy it. We'll recreate it below.
	if (mView && mView->IsCrashed ())
	{
		mView->Destroy ();
		mView = nullptr;
		clearJavaScriptBinds ();
		initView ();
	}

	Awesomium::BitmapSurface *surface = mView ? (Awesomium::BitmapSurface *)mView->surface () : nullptr;
	
	// Check if we really need to redraw.
	if (surface && (surface->is_dirty () || mCursorPos != mCursorRenderPos || mRenderedCursorLastFrame != mShowCursor))
	{
		if (surface)
		{
			surface->set_is_dirty (false);
		}
		
		GFXLockedRect *rect = mTexture.lock ();

		// When we resize the Awesomium surface, this can take a while as it's asynchronous.
		// In this case we have to take a different approach when we blit the texture.
		//if (mTexture.getHeight () != surface->height () || mTexture.getWidth () != surface->width ())
		{
			U32 smallestWidth = mTexture.getWidth ();
			if (surface->width () < smallestWidth)
			{
				smallestWidth = surface->width ();
			}
			U32 smallestHeight = mTexture.getHeight ();
			if (surface->height () < smallestHeight)
			{
				smallestHeight = surface->height ();
			}
			for (U32 y = 0; y < smallestHeight; y++)
			{
				for (U32 x = 0; x < smallestWidth; x++)
				{
					U32 targetIndex = ((y * mTexture.getWidth ()) + x) * 4;
					U32 sourceIndex = ((y * surface->width ()) + x) * 4;
               if (GFX->getAdapterType() == OpenGL)
               {
                  rect->bits[targetIndex] = surface->buffer()[sourceIndex+2]; //swizzle
                  rect->bits[targetIndex + 1] = surface->buffer()[sourceIndex + 1];
                  rect->bits[targetIndex + 2] = surface->buffer()[sourceIndex]; //swizzle
                  rect->bits[targetIndex + 3] = surface->buffer()[sourceIndex + 3];
               }
               else
               {
                  rect->bits[targetIndex] = surface->buffer()[sourceIndex];
                  rect->bits[targetIndex + 1] = surface->buffer()[sourceIndex + 1];
                  rect->bits[targetIndex + 2] = surface->buffer()[sourceIndex + 2];
                  rect->bits[targetIndex + 3] = surface->buffer()[sourceIndex + 3];
               }
				}
			}
		}
      /*
		else
		{
			memcpy (rect->bits, surface->buffer (), mTexture.getHeight () * mTexture.getWidth () * 4);
		}
      else
         surface->CopyTo(rect->bits, mTexture.getHeight()*mTexture.getWidth(), mTexture.getDepth(), true, false);
      */
		if (mShowCursor && mCursorBitmap)
		{
			mCursorRenderPos = mCursorPos;
			blitCursorToTexture (rect);
			mRenderedCursorLastFrame = mShowCursor;
		}

		mTexture.unlock ();
	}
}

void AwContext::OnMethodCall (Awesomium::WebView *view, unsigned int id, const Awesomium::WebString &name, const Awesomium::JSArray &inArgs)
{
	JavaScriptObject *object;
	if (!mJavaScriptObjectsById.tryGetValue (id, object))
	{
		return;
	}

	// TODO: Is this size correct?
	char buffer [512];
	name.ToUTF8 (buffer, sizeof (buffer));

	Delegate <void (const Vector <String> &)> delegate;
	if (!object->delegates.tryGetValue (buffer, delegate))
	{
		return;
	}

	// Divide arguments into a list
	Vector <String> args;
	for (U32 i = 0; i < inArgs.size (); i++)
	{
		inArgs [i].ToString ().ToUTF8 (buffer, sizeof (buffer));
		args.push_back (buffer);
	}

	delegate (args);
}

void AwContext::OnDocumentReady (Awesomium::WebView *view, const Awesomium::WebURL &url)
{
	if (mIsJavaScriptReady)
	{
		return;
	}

	// The document structure is ready and this is a good time to create JavaScript objects.
	for (Map <String, JavaScriptObject *>::Iterator i = mJavaScriptObjectsByName.begin (); i != mJavaScriptObjectsByName.end (); i++)
	{
		Awesomium::JSValue value = view->CreateGlobalJavascriptObject (Awesomium::WebString::CreateFromUTF8 (i->key.c_str (), i->key.length ()));

		if (!value.IsObject ())
		{
			continue;
		}

		Awesomium::JSObject &obj = value.ToObject ();
		mJavaScriptObjectsById.insert (obj.remote_id (), i->value);

		// Add the functions.
		for (Map <String, Delegate <void (const Vector <String> &)>>::Iterator j = i->value->delegates.begin (); j != i->value->delegates.end (); j++)
		{
			obj.SetCustomMethod (Awesomium::WebString::CreateFromUTF8 (j->key.c_str (), j->key.length ()), false);
		}
	}

	mIsJavaScriptReady = true;
}

void AwContext::onTorqueScript (const Vector <String> &args)
{
	if (!args.size ())
	{
		return;
	}

	Con::evaluatef (args [0]);
}

void AwContext::clearJavaScriptBinds ()
{
	for (Map <U32, JavaScriptObject *>::Iterator iter = mJavaScriptObjectsById.begin (); iter != mJavaScriptObjectsById.end (); iter++)
	{
		delete iter->value;
	}

	mJavaScriptObjectsByName.clear ();
	mJavaScriptObjectsById.clear ();
	mIsJavaScriptReady = false;
}

void AwContext::bindJavaScript (const String &objName, const String &funcName, const Delegate <void (const Vector <String> &)> &delegate)
{
	JavaScriptObject *obj = nullptr;
	if (!mJavaScriptObjectsByName.tryGetValue (objName, obj))
	{
		obj = new JavaScriptObject;
		mJavaScriptObjectsByName.insert (objName, obj);
	}

	obj->delegates.insert (funcName, delegate);
}

void AwContext::execJavaScript (const String &script)
{
	Awesomium::WebString awScript = Awesomium::WebString::CreateFromUTF8 (script.c_str (), script.length ());
	mView->ExecuteJavascript (awScript, Awesomium::WebString ());
}

void AwContext::initView ()
{
	if (mView)
	{
		return;
	}

	mView = Awesomium::WebCore::instance ()->CreateWebView (mTexture.getWidth (), mTexture.getHeight (), AwManager::getSessionFromPath (mSessionPath));

	// Bind to TorqueScript by default.
	Delegate <void (const Vector <String> &)> delegate;
	delegate.bind (this, &AwContext::onTorqueScript);
	bindJavaScript ("TorqueScript", "call", delegate);

	mView->SetTransparent (mIsTransparent);
	mView->set_load_listener (this);
	mView->set_js_method_handler (this);
}

void AwContext::OnShowCertificateErrorDialog (Awesomium::WebView *view, bool isOverridable, const Awesomium::WebURL &url, Awesomium::CertError error)
{
	Con::printf ("Awesomium Warning: A certificate error has occured.");
}

void AwContext::OnBeginLoadingFrame (Awesomium::WebView *view, int64 frame_id, bool is_main_frame, const Awesomium::WebURL &url, bool is_error_page)
{
	char buffer [512];
	url.spec ().ToUTF8 (buffer, sizeof (buffer));
	mCurrentURL = buffer;

	// Remove any prefixes
	mCurrentURL.replace ("asset://torque/", "");
	S32 prefixPos = mCurrentURL.find ("://");
	if (prefixPos == String::NPos)
	{
		// Incase we allow file:/// which is used to access local files outside the Torque ResourceManager.
		prefixPos = mCurrentURL.find (":///");
		if (prefixPos != String::NPos)
			prefixPos += 4;
	}
}

void AwContext::enable ()
{
	if (mView)
	{
		mView->ResumeRendering ();
	}
}

void AwContext::disable ()
{
	if (mView)
	{
		mView->PauseRendering ();
	}
}

void AwContext::unfocus ()
{
	if (mView)
	{
		mView->Unfocus ();
	}
}

void AwContext::focus ()
{
	if (mView)
	{
		mView->Focus ();
	}
}

void AwContext::loadURL (String url)
{
	if (url.isEmpty ())
	{
		return;
	}

	mCurrentURL = url;

	// Mimic TorqueScripts logic and do not allow access outside the executable directory. This also forces the use of the ResourceManager.
	// If you want to disable this behaviour, comment out the two lines below.
	url.replace ("file://", "asset://torque/");
	url.replace ("file:///", "asset://torque/");

	// Awesomium normally won't accept just a www. address, and requires the protocol (http, https.. etc) to be specified. This makes
	// it default to http if nothing else is specified but there's a www. in the address.
	if (url.find ("www.") == 0 && url.find ("https://") == String::NPos && url.find ("http://") == String::NPos)
	{
		url.insert (0, "http://");
	}
	// If nothing else is specified we default to the asset prefix.
	else if (url.find ("://") == String::NPos)
	{
		url.insert (0, "asset://torque/");
	}

	initView ();
	mView->LoadURL (Awesomium::WebURL (Awesomium::WSLit (url)));
}

void AwContext::injectLeftMouseDown ()
{
	if (mView)
	{
		mView->InjectMouseDown (Awesomium::kMouseButton_Left);
	}
}

void AwContext::injectLeftMouseUp ()
{
	if (mView)
	{
		mView->InjectMouseUp (Awesomium::kMouseButton_Left);
	}
}

void AwContext::injectRightMouseDown ()
{	
	if (mView)
	{
		mView->InjectMouseDown (Awesomium::kMouseButton_Right);
	}
}

void AwContext::injectRightMouseUp ()
{
	if (mView)
	{
		mView->InjectMouseUp (Awesomium::kMouseButton_Right);
	}
}

void AwContext::injectMouseWheelDown (F32 amount)
{
	if (mView)
	{
		mView->InjectMouseWheel (amount, 0);
	}
}

void AwContext::injectMouseWheelUp (F32 amount)
{
	if (mView)
	{
		mView->InjectMouseWheel (amount, 0);
	}
}

void AwContext::injectMouseMove (const Point2I &pnt)
{
	if (!mView)
	{
		return;
	}

	mCursorPos = pnt;
	mView->InjectMouseMove (pnt.x, pnt.y);
}

void AwContext::injectMiddleMouseDown ()
{
	if (mView)
	{
		mView->InjectMouseDown (Awesomium::kMouseButton_Middle);
	}
}

void AwContext::injectMiddleMouseUp ()
{
	if (mView)
	{
		mView->InjectMouseUp (Awesomium::kMouseButton_Middle);
	}
}

void AwContext::injectKeyDown (U8 key, U16 ascii)
{
	if (!mView)
	{
		return;
	}

	Awesomium::WebKeyboardEvent kEvent;
	kEvent.type = Awesomium::WebKeyboardEvent::kTypeKeyDown;

	if (!AwManager::sKeyCodes.tryGetValue (key, kEvent.virtual_key_code))
	{
		kEvent.type = Awesomium::WebKeyboardEvent::kTypeChar;
		kEvent.text [0] = ascii;
		kEvent.text [1] = 0;
	}

	mView->InjectKeyboardEvent (kEvent);
}

void AwContext::injectKeyUp (U8 key)
{
	if (!mView)
	{
		return;
	}

	Awesomium::WebKeyboardEvent kEvent;
	kEvent.type = Awesomium::WebKeyboardEvent::kTypeKeyUp;

	if (!AwManager::sKeyCodes.tryGetValue (key, kEvent.virtual_key_code))
	{
		return;
	}

	mView->InjectKeyboardEvent (kEvent);
}

void AwContext::update ()
{
	if (mIsPaused)
	{
		return;
	}

	if (!mTexture || mNextUpdateTime < Platform::getRealMilliseconds ())
	{
		copyToTexture ();
		if (mFramerate > 0)
		{
			mNextUpdateTime = Platform::getRealMilliseconds () + (1000.0f / (F32)mFramerate);
		}
	}
}

bool AwContext::isLoading ()
{
	return mView ? mView->IsLoading () : false;
}

void AwContext::reload (bool ignoreCache)
{
	if (mView)
	{
		mView->Reload (ignoreCache);
	}
}

void AwContext::stop ()
{
	if (mView)
	{
		mView->Stop ();
	}
}

String AwContext::getCurrentURL ()
{
	return mCurrentURL;
}

void AwContext::setFramerate (U8 framerate)
{
	mFramerate = framerate;
}

void AwContext::setResolution (const Point2I &resolution)
{
	if (!mTexture || mTexture.getWidth () != resolution.x || mTexture.getHeight () != resolution.y)
	{
		mTexture = GFX->getTextureManager ()->createTexture (resolution.x, resolution.y, GFXFormatR8G8B8A8, &GFXDefaultPersistentProfile, 0, 0);
		if (mView)
		{
			mView->Resize (resolution.x, resolution.y);
		}
	}
}

void AwContext::setCursorBitmapPath (const String &path)
{
	if (path.isEmpty ())
	{
		return;
	}

	Resource <GBitmap> bitmap = GBitmap::load (path);
	if (bitmap)
	{
		mCursorBitmap = bitmap;
	}
}

void AwContext::setTransparent (bool isTransparent)
{
	mIsTransparent = isTransparent;
}

bool AwContext::isTransparent ()
{
	return mIsTransparent;
}

U8 AwContext::getAlphaAtPoint (const Point2I &pnt)
{
	if (!mView || !mIsTransparent|| !mView->surface () || mView->IsLoading ())
	{
		return 255;
	}

	if (mView->IsLoading () || mView->IsCrashed ())
	{
		return 255;
	}

	if (!mView->surface ())
	{
		return 255;
	}

	return ((Awesomium::BitmapSurface *)(mView->surface ()))->GetAlphaAtPoint (pnt.x, pnt.y);
}

void AwContext::showCursor ()
{
	mShowCursor = true;
}

void AwContext::setSessionPath (const String &sessionPath)
{
	mSessionPath = sessionPath;
}

void AwContext::pause ()
{
	if (mView)
	{
		mView->PauseRendering ();
		mIsPaused = true;
	}
}

void AwContext::resume ()
{
	if (mView)
	{
		mView->ResumeRendering ();
		mIsPaused = false;
	}
}

bool AwContext::isPaused ()
{
	return mIsPaused;
}

void AwContext::copy ()
{
	if (mView)
	{
		mView->Copy ();
	}
}

void AwContext::cut ()
{
	if (mView)
	{
		mView->Cut ();
	}
}

void AwContext::paste ()
{
	if (mView)
	{
		mView->Paste ();
	}
}

void AwContext::undo ()
{
	if (mView)
	{
		mView->Undo ();
	}
}

void AwContext::redo ()
{
	if (mView)
	{
		mView->Redo ();
	}
}