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

#include "AwTextureTarget.h"
#include "console/engineAPI.h"
#include "Materials/BaseMatInstance.h"
#include "AwManager.h"
#include "AwContext.h"
#include "AwShape.h"
#include "T3D/GameBase/GameConnection.h"
#include "SFX/SFXTypes.h"
#include "gui/3d/guiTSControl.h"
#include "Core/Stream/FileStream.h"

IMPLEMENT_CONOBJECT (AwTextureTarget);

AwTextureTarget *AwTextureTarget::sMouseInputTarget = nullptr;

void AwTextureTarget::initPersistFields ()  
{  
	addField ("StartURL",			TypeRealString,	Offset (mStartURL, AwTextureTarget), "The URL which is loaded initially.");
	addField ("TextureTargetName",	TypeRealString,	Offset (mTexTargetName, AwTextureTarget), "Name of the texture target. The texture name can be used in materials to reference this AwTextureTarget.");
	addField ("Framerate",			TypeS8,			Offset (mFramerate, AwTextureTarget), "The amount of frames per second to render. 0 means unlimited.");
	addField ("Resolution",			TypePoint2I,	Offset (mResolution, AwTextureTarget), "Resolution. Defaults to (640, 480).");
	addField ("CursorBitmap",		TypeRealString,	Offset (mCursorBitmapPath, AwTextureTarget), "The bitmap which is used as a cursor. A default cursor will be used if none is set.");

	addField ("IsSingleFrame",	 TypeBool,			Offset (mIsSingleFrame, AwTextureTarget), "Tells this AwTextureTarget to only generate a single frame. This consumes much less resources than a regular AwTextureTarget. Default: Disabled");
	addField ("UseBitmapCache",	 TypeBool,			Offset (mUseBitmapCache, AwTextureTarget), "If set, enables the bitmap cache. This cache is useful when the webpage is loading and you want the user to see something right away.");
	addField ("BitmapCachePath", TypeRealString,	Offset (mBitmapCachePath, AwTextureTarget), "Forces the BitmapCache filename instead of letting the system chose a filename automatically.");

	addField ("OnGainMouseInputSound", TypeSFXTrackName,  Offset (mOnGainMouseInputSound, AwTextureTarget), "The sound profile to play when gaining mouse input.");
	addField ("OnLoseMouseInputSound", TypeSFXTrackName,  Offset (mOnLoseMouseInputSound, AwTextureTarget), "The sound profile to play when losing mouse input.");
	Parent::initPersistFields ();
}

AwTextureTarget::AwTextureTarget ()
{
	mResolution.set (640, 480);
	mFramerate = 0;
	mActualFramerate = 0;
	mOnGainMouseInputSound = nullptr;
	mOnLoseMouseInputSound = nullptr;
	mLastRenderTime = 0;
	mLargestDistanceThisUpdate = 0;
	mNumShapesBound = 0;
	mRefCount = 0;
	mIsSingleFrame = false;
	mHasWrittenToCache = false;
	mUseBitmapCache = false;
	mIsShowingCachedBitmap = false;
}

bool AwTextureTarget::onAdd ()
{
	if (!Parent::onAdd ())
	{
		return false;
	}

	if (mStartURL.isEmpty ())
	{
		Con::errorf ("AwTextureTarget::onAdd - 'StartURL' must be set");
		return false;
	}

	mContext = nullptr;

	if (mTexTargetName.isEmpty() )
	{
		Con::errorf ("AwTextureTarget::onAdd - 'TextureTargetName' not set");
		return false;
	}

	if (!mTexTarget.registerWithName (mTexTargetName))
	{
		Con::errorf ("AwTextureTarget::onAdd - Could not register texture target '%s", mTexTargetName.c_str ());
		return false;
	}

	// Automatically generate the bitmap cache filename.
	if (mUseBitmapCache && mBitmapCachePath.isEmpty ())
	{
		mBitmapCachePath = String ("./") + String (getName ());
	}

	char temp [2048]; 
	Con::expandScriptFilename (&temp [0], sizeof (temp), mBitmapCachePath.c_str ());
	Torque::Path path = temp;
	path.setExtension ("png");
	mBitmapCachePath = path;

	mTexTarget.getTextureDelegate ().bind (this, &AwTextureTarget::onRender);
	AwManager::addTextureTarget (this);

	return true;
}

void AwTextureTarget::onRemove ()
{
	// Unregister the texture target.
	mTexTarget.unregister ();

	if (sMouseInputTarget == this)
	{
		sMouseInputTarget = nullptr;
	}

	delete mContext;
	mContext = nullptr;
	AwManager::removeTextureTarget (this);
	Parent::onRemove ();
}

GFXTexHandle AwTextureTarget::getTexture () 
{
	if (mTexture && !mHasWrittenToCache && mUseBitmapCache && mContext && !mContext->isLoading ())
	{
		mHasWrittenToCache = true;
		FileStream stream;
		if (!Platform::isFile (mBitmapCachePath) && stream.open (mBitmapCachePath, Torque::FS::File::Write))
		{
			GBitmap bmp (mTexture->getBitmapWidth (), mTexture->getBitmapHeight (), false, GFXFormatR8G8B8A8);
			if (mTexture->copyToBmp (&bmp))
			{
				bmp.writeBitmap ("png", stream);
				stream.close ();
			}
		}
	}

	if (mIsSingleFrame)
	{
		if (mContext && !mContext->isLoading ())
		{
			mTexture = mContext->getTexture ();
			delete mContext;
			mContext = nullptr;
		}
	}
	else if (mContext && (!mUseBitmapCache || (!mContext->isLoading () || !mIsShowingCachedBitmap)))
	{
		mTexture = mContext->getTexture ();
		// We only want to overwrite the cached texture with a texture from the context if the context has finished loading.
		mIsShowingCachedBitmap = false; 
	}

	return mTexture;
}

void AwTextureTarget::incrRef ()
{
	mRefCount++;
	initContext ();
}

void AwTextureTarget::decrRef ()
{
	mRefCount--;
	if (mRefCount == 0)
	{
		delete mContext;
		mContext = nullptr;
		mTexture = nullptr;
	}
}

void AwTextureTarget::initContext ()
{
	if (mContext || mTexture)
	{
		return;
	}

	if (mUseBitmapCache)
	{
		mTexture.set (mBitmapCachePath, &GFXDefaultStaticDiffuseProfile, "");
		mIsShowingCachedBitmap = true;
	}

	mContext = new AwContext;
	mContext->setFramerate (mFramerate);
	mContext->setResolution (mResolution);
	mContext->loadURL (mStartURL);
	mContext->setCursorBitmapPath (mCursorBitmapPath);
}

GFXTextureObject *AwTextureTarget::onRender (U32 index)
{
	mLastRenderTime = Platform::getRealMilliseconds ();
	return getTexture ();
}

void AwTextureTarget::injectMouseMove (const Point2I &pos)
{
	if (mContext)
	{
		mContext->injectMouseMove (pos);
	}
}

void AwTextureTarget::injectMouseDown ()
{
	if (mContext)
	{
		mContext->injectLeftMouseDown ();
	}
}

void AwTextureTarget::injectMouseUp ()
{
	if (mContext)
	{
		mContext->injectLeftMouseUp ();
	}
}

void AwTextureTarget::onGainMouseInput ()
{
	if (mContext)
	{
		mContext->showCursor ();
	}
}

void AwTextureTarget::onLoseMouseInput ()
{
	if (mContext)
	{
		mContext->hideCursor ();
	}
}

void AwTextureTarget::setMouseInputTarget (AwTextureTarget *target) 
{ 
	if (sMouseInputTarget == target || (target && target->mIsSingleFrame))
	{
		return;
	}

	if (sMouseInputTarget)
	{
		sMouseInputTarget->onLoseMouseInput ();
	}

	sMouseInputTarget = target;
	if (sMouseInputTarget)
	{
		sMouseInputTarget->onGainMouseInput ();	
	}
}

bool AwTextureTarget::isPaused ()
{
	if (mContext)
	{
		return mContext->isPaused ();
	}

	if (mIsSingleFrame)
	{
		return mLastRenderTime + 500 < Platform::getRealMilliseconds ();
	}

	return false;
}

void AwTextureTarget::update (U32 fps)
{
	if (!mContext || mIsSingleFrame)
	{
		return;
	}

	// If the target hasn't been used for a moment, pause the context. Do not let it pause if we're currently focused.
	// TODO: Make the time value configurable?
	if ((mLastRenderTime == 0 || mLastRenderTime + 500 < Platform::getRealMilliseconds ()) && sMouseInputTarget != this)
	{
		mContext->pause ();
		return;
	}

	if (mContext->isPaused ())
	{
		mContext->resume ();
	}

	if (sMouseInputTarget == this)
	{
		mActualFramerate = 0;
	}
	else
	{
		if (mFramerate == 0)
		{
			F32 targetFramerate = fps;
			if (mLargestDistanceThisUpdate >= AwManager::getLoadBalancingDistance ())
				mActualFramerate = 1;
			else
			{
				F32 framerate = 1.0f - (mLargestDistanceThisUpdate / AwManager::getLoadBalancingDistance ());
				mActualFramerate = framerate * targetFramerate;
			}
		}
		else
		{
			mActualFramerate = mFramerate;
		}
	}
	
	mLargestDistanceThisUpdate = 0;
	mContext->setFramerate (mActualFramerate);
}

void AwTextureTarget::execJavaScript (const String &script)
{
	if (mContext)
	{
		mContext->execJavaScript (script);
	}
}

void AwTextureTarget::reload ()
{
	if (mContext)
	{
		mContext->reload ();
	}
	else
	{
		mTexture = nullptr;
		initContext ();
	}
}

DefineEngineMethod (AwTextureTarget, execJavaScript, void, (const char *script),, "")
{
	object->execJavaScript (script);
}

DefineEngineMethod (AwTextureTarget, reload, void, (),, "")
{
	object->reload ();
}