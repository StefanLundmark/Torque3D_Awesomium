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
#include "platform/platform.h"
#include "AwShape.h"
#include "AwTextureTarget.h"
#include "AwManager.h"
#include "core/resourceManager.h"
#include "core/stream/bitStream.h"
#include "scene/sceneRenderState.h"
#include "scene/sceneManager.h"
#include "scene/sceneObjectLightingPlugin.h"
#include "lighting/lightManager.h"
#include "ts/tsShapeInstance.h"
#include "ts/tsMaterialList.h"
#include "console/consoleTypes.h"
#include "T3D/shapeBase.h"
#include "sim/netConnection.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTransformSaver.h"
#include "ts/tsRenderState.h"
#include "collision/boxConvex.h"
#include "materials/materialDefinition.h"
#include "materials/materialManager.h"
#include "materials/matInstance.h"
#include "materials/materialFeatureData.h"
#include "materials/materialFeatureTypes.h"
#include "console/engineAPI.h"
#include "SFX/SFXTypes.h"
#include "SFX/SFXSystem.h"
#include "AwManager.h"
#include "AwTextureCursor.h"

IMPLEMENT_CO_NETOBJECT_V1 (AwShape);

AwShape *AwShape::sMouseInputShape = nullptr;

AwShape::AwShape ()
{
	mTypeMask |= StaticObjectType | StaticShapeObjectType | AwShapeObjectType;
	mMatInstance = nullptr;
	mTextureTarget = nullptr;
	mIsMouseDown = false;
}

bool AwShape::onAdd ()
{
	if (!Parent::onAdd ())
	{
		return false;
	}

	// If on the server, do nothing.
	if (isServerObject ())
	{
		return true;
	}

	ResourceManager::get ().getChangedSignal ().notify (this, &AwShape::onResourceChanged);
	AwManager::addShape (this);
	updateMapping ();
	return true;
}

void AwShape::onRemove ()
{
	if (!isServerObject ())
	{
		if (AwShape::getMouseInputShape () == this)
		{
			sMouseInputShape = NULL;
		}
		AwManager::removeShape (this);
		ResourceManager::get ().getChangedSignal ().remove (this, &AwShape::onResourceChanged);
		if (mTextureTarget)
		{
			mTextureTarget->decrRef ();
		}
	}
	Parent::onRemove ();
}

void AwShape::onResourceChanged (const Torque::Path &path)
{
	AwManager::removeShape (this);
	AwManager::addShape (this);

	updateMapping ();
}

void AwShape::updateMapping ()
{
	mMatInstance = NULL;
	mTextureTarget = NULL;

	// Go trough all materials on the shape and map them to their texture targets.
	if (!mShapeInstance)
	{
		return;
	}

	TSMaterialList *list = mShapeInstance->getMaterialList ();
	if (!list)
	{
		return;
	}

	for (U32 i = 0; i < list->size (); i++)
	{
		BaseMatInstance *matInst = list->getMaterialInst (i);
		for (U32 j = 0; j < Material::MAX_STAGES; j++)
		{
			if (mTextureTarget)
			{
				mTextureTarget->decrRef ();
			}
			mTextureTarget = AwManager::findTextureTargetByMaterial (matInst);
			if (mTextureTarget)
			{
				// Let the texture target know we're using it.
				mTextureTarget->incrRef ();
				mMatInstance = matInst;
				return; // We only support one AwTextureTarget per shape.
			}
		}
	}
}

void AwShape::onGainMouseInput ()
{
	if (mTextureTarget && mTextureTarget->getOnGainMouseInputSound () && !mTextureTarget->isSingleFrame ())
	{
		SFX->playOnce (mTextureTarget->getOnGainMouseInputSound ());
	}
}

void AwShape::onLoseMouseInput ()
{
	if (mTextureTarget->isSingleFrame ())
	{
		return;
	}

	if (mTextureTarget && mTextureTarget->getOnLoseMouseInputSound ())
	{
		SFX->playOnce (mTextureTarget->getOnLoseMouseInputSound ());
	}

	setIsMouseDown (false);
}

void AwShape::setMouseInputShape (AwShape *shape) 
{ 
	if (sMouseInputShape == shape || (shape && shape->mTextureTarget && shape->mTextureTarget->isSingleFrame ()))
	{
		return;
	}

	if (sMouseInputShape)
	{
		sMouseInputShape->onLoseMouseInput ();
	}

	sMouseInputShape = shape;
	if (sMouseInputShape)
	{
		sMouseInputShape->onGainMouseInput ();	
	}
}

void AwShape::setIsMouseDown (bool isMouseDown)
{	
	mIsMouseDown = isMouseDown;
	if (mTextureTarget)
	{
		if (mIsMouseDown)
		{
			mTextureTarget->injectMouseDown ();
		}
		else
		{
			mTextureTarget->injectMouseUp ();
		}
	}
}

//----------------------------------------------------------------------------
AwTextureTarget *AwShape::processAwesomiumHit (const Point3F &start, const Point3F &end)
{
	if (!mTextureTarget)
	{
		return NULL;
	}

	Point3F localStart, localEnd;
	MatrixF mat = getTransform();
	mat.scale (Point3F (getScale ()));
	mat.inverse ();

	mat.mulP (start, &localStart);
	mat.mulP (end, &localEnd);

	RayInfo info;
	info.generateTexCoord = true;
	if (!mShapeInstance || !mShapeInstance->castRayOpcode (0, localStart, localEnd, &info))
	{
		return NULL;
	}

	if (info.texCoord.x != -1 && info.texCoord.y != -1 && info.material == mMatInstance)
	{
		Point2I pnt (info.texCoord.x * mTextureTarget->getResolution ().x, info.texCoord.y * mTextureTarget->getResolution ().y);
		
		AwManager::sCursor->setPosition (pnt);

		if (mIsMouseDown)
		{
			mTextureTarget->injectMouseDown ();
		}
		else
		{
			mTextureTarget->injectMouseUp ();
		}

		return mTextureTarget;
	}
	return NULL;
}

void AwShape::execJavaScript (const String &script)
{
	if (mTextureTarget)
	{
		mTextureTarget->execJavaScript (script);
	}
}

DefineEngineMethod (AwShape, execJavaScript, void, (const char *script),, "")
{
	object->execJavaScript (script);
}