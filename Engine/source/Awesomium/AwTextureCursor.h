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

#include "Core/iTickable.h"
#include "Platform/Types.h"
#include "Math/mPoint2.h"

/*
 *  AwTextureCursor
 *  -----------------------------------------------------------------------------------------------
 *	Optional feature used by AwShape to render a cursor texture.
 */
class AwTextureCursor : public ITickable
{
	friend class AwManager;

	bool mHasPositionBeenSet;
	Point2I mLastPosition;					// The previous position.
	Point2I mPosition;						// The current position.
	Point2I mRenderPosition;				// Interpolated position
	Point2I mNextPosition;					// Most recent position to be set on the next tick.

public:
	AwTextureCursor ();

	virtual void interpolateTick (F32 delta);
	virtual void processTick ();
	virtual void advanceTime (F32 timeDelta) {}

	void reset ();
	Point2I getRenderPosition () { return mRenderPosition; }
	void setPosition (const Point2I &pos) { mNextPosition = pos; }
};