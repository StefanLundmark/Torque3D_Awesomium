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

#include "materials/matTextureTarget.h"  
#include "GFX/GFXTextureHandle.h"
#include "Console/SimObject.h"
#include "SFX/SFXTrack.h"
#include "SFX/SFXSource.h"

class AwContext;
class AwShape;

/*
 *  AwTextureTarget
 *  -----------------------------------------------------------------------------------------------
 *  A named texture target. The texture name can be used in materials to
 *	reference this AwTextureTarget. Please see the included zip file on the GitHub page for sample usage.
 */
class AwTextureTarget : public SimObject
{
	typedef SimObject Parent;

	friend class AwManager;
	friend class AwTextureCursor;

	U32 mRefCount;										// How many references this AwTextureTarget has. When this reaches zero, the target is freed.
	AwContext *mContext;								// The associated context.
	Point2I mResolution;								// The current resolution. Defaults to (640, 480).
	U8 mFramerate;										// The amount of frames per second to render. 0 means unlimited.
	U8 mActualFramerate;								// The actual framerate which is calculated based on how distance, mouse focus and other parameters.
	String mStartURL;									// The URL which is loaded initially.
	String mTexTargetName;								// Name of the texture target. The texture name can be used in materials to reference this AwTextureTarget.
	String mCursorBitmapPath;							// The bitmap which is used as a cursor. A default cursor will be used if none is set.
	U32 mLastRenderTime;
	F32 mLargestDistanceThisUpdate;
	U32 mNumShapesBound;
	bool mIsSingleFrame;								// Tells this AwTextureTarget to only generate a single frame. This consumes much less resources than a regular AwTextureTarget. Defaults to disabled.
	String mBitmapCachePath;							// Forces the BitmapCache filename instead of letting the system chose a filename automatically.
	GFXTexHandle mTexture;
	bool mHasWrittenToCache;
	bool mUseBitmapCache;								// If set, enables the bitmap cache. This cache is useful when the webpage is loading and you want the user to see something right away.
	bool mIsShowingCachedBitmap;

	static AwTextureTarget *sMouseInputTarget;			// The active texture target, if there is one.
	SFXTrack *mOnGainMouseInputSound;					// The sound profile to play when gaining mouse input.
	SFXTrack *mOnLoseMouseInputSound;					// The sound profile to play when losing mouse input.

	NamedTexTarget mTexTarget;							// Torque's named texture target.

	void initContext ();
	GFXTextureObject *onRender (U32 index);
	void update (U32 fps);
	void onLoseMouseInput ();
	void onGainMouseInput ();

public:
	AwTextureTarget ();
	DECLARE_CONOBJECT (AwTextureTarget);

	virtual bool onAdd ();
	virtual void onRemove ();

	void incrRef ();
	void decrRef ();

	static void setMouseInputTarget (AwTextureTarget *target);
	static AwTextureTarget *getMouseInputTarget () { return sMouseInputTarget; }
	SFXTrack *getOnGainMouseInputSound () { return mOnGainMouseInputSound; }
	SFXTrack *getOnLoseMouseInputSound () { return mOnLoseMouseInputSound; }

	U32 getActualFramerate () const { return mActualFramerate; } // The actual framerate which is calculated based on how distance, mouse focus and other parameters.
	U32 getFramerate () { return mFramerate; }			// The amount of frames per second to render. 0 means unlimited.
	GFXTexHandle getTexture ();							// Returns the texture. The data can be from the cache or from the context.
	
	void injectMouseMove (const Point2I &pos);
	void injectMouseDown ();
	void injectMouseUp ();

	void execJavaScript (const String &script);			// Executes JavaScript for this AwTextureTarget.
	bool isPaused ();									// Whether or not rendering of this view is paused.
	void setDistance (F32 distance) { if (distance > mLargestDistanceThisUpdate) mLargestDistanceThisUpdate = distance; } // TODO: Move this to AwShape, as it has no business in this general purpose class.
	void reload ();										// Reloads the view, optionally ignoring the cache.
	U32 getRefCount () { return mRefCount; }			// How many references this AwTextureTarget has. When this reaches zero, the target is freed.
	bool isSingleFrame () { return mIsSingleFrame; }	// Returns true if this AwTextureTarget only generates a single frame. This consumes much less resources than a regular AwTextureTarget.
	Point2I getResolution () { return mResolution; }	// Returns the current resolution.

	static void initPersistFields ();
};