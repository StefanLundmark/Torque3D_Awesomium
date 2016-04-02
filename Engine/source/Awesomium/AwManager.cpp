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

#include "core/module.h"
#include "platform/platform.h"
#include "Core/Util/Path.h"
#include "TorqueConfig.h"
#include "Materials/BaseMatInstance.h"
#include "materials/materialDefinition.h"
#include "Materials/MatInstance.h"
#include "Materials/ProcessedMaterial.h"
#include "Materials/MaterialFeatureTypes.h"
#include "ts/tsShapeInstance.h"
#include "Materials/MaterialManager.h"
#include "T3D/GameBase/GameConnection.h"
#include "gui/3d/guiTSControl.h"
#include "Core/Stream/FileStream.h"

#include "AwManager.h"
#include "AwTextureTarget.h"
#include "AwContext.h"
#include "AwShape.h"
#include "AwTextureCursor.h"
#include "AwDataSource.h"

// Awesomium Headers
#include <Awesomium/WebCore.h>
#include <Awesomium/BitmapSurface.h>
#include <Awesomium/STLHelpers.h>

AwDataSource *AwManager::sDataSource										= nullptr;
U32	AwManager::sNumFrames													= 0;
U32	AwManager::sFramerate													= 60;
U32	AwManager::sNextUpdateTime												= 0;
AwTextureCursor *AwManager::sCursor											= nullptr;
F32	AwManager::sRayLengthScale												= 0.0f;
F32	AwManager::sImageDropSpeed												= 0.0f;
F32	AwManager::sLoadBalancingDistance										= 0.0f;
U32 AwManager::sCurrentTargetIndex											= 0;
U32 AwManager::sCurrentShapeIndex											= 0;
U32 AwManager::sMaxIterationsPerFrame										= 64;
Map <BaseMatInstance *, AwTextureTarget *> AwManager::sTargetsByMaterial;
Map <String, AwTextureTarget *> AwManager::sTextureTargetsByName;
Vector <AwTextureTarget *> AwManager::sTargets;
Vector <AwShape *> AwManager::sShapes;
Map <String, Awesomium::WebSession *> AwManager::sSessions;
Map <U8, int> AwManager::sKeyCodes;

/*
 *  Initialization macros which lets our module initialize after Torque's MaterialManager.
 */
MODULE_BEGIN (AwManager)
	MODULE_INIT_AFTER (MaterialManager)
	MODULE_INIT
{
	AwManager::init ();
}
MODULE_SHUTDOWN_AFTER (Sim)
MODULE_SHUTDOWN
{
	AwManager::shutdown ();
}
MODULE_END;

void AwManager::init ()
{
	if (Awesomium::WebCore::instance ())
	{
		return;
	}
	
	sCursor = new AwTextureCursor;
	sDataSource = new AwDataSource;
	Awesomium::WebConfig config;

	String userAgent = "Mozilla/5.0 (Windows NT 6.1; U;WOW64; en-US) Gecko Firefox/11.0";
	config.user_agent = Awesomium::WebString::CreateFromUTF8 (userAgent.utf8 (), userAgent.length ());
#ifdef _DEBUG
	config.log_level = Awesomium::kLogLevel_Verbose;
#else
	config.log_level = Awesomium::kLogLevel_None;
#endif
	Awesomium::WebCore::Initialize (config);

	setupInput ();
	readConsoleVariables ();

	SceneManager::getPreRenderSignal ().notify (onPreRender);
	GFXDevice::getDeviceEventSignal ().notify (onDeviceEvent);
}

void AwManager::setupInput ()
{
	// Map all the keycodes we want to catch. Some of the input handling is broken in Awesomium (2012-11-12) so for example TAB apparantly won't work
	// to switch between text input boxes.
	sKeyCodes.insert (KEY_BACKSPACE,	Awesomium::KeyCodes::AK_BACK);
	sKeyCodes.insert (KEY_TAB,          Awesomium::KeyCodes::AK_TAB);
	sKeyCodes.insert (KEY_RETURN,		Awesomium::KeyCodes::AK_RETURN);
	sKeyCodes.insert (KEY_CONTROL,      Awesomium::KeyCodes::AK_CONTROL);
	sKeyCodes.insert (KEY_ALT,          Awesomium::KeyCodes::AK_MENU);
	sKeyCodes.insert (KEY_SHIFT,        Awesomium::KeyCodes::AK_SHIFT);
	sKeyCodes.insert (KEY_PAUSE,        Awesomium::KeyCodes::AK_PAUSE);
	sKeyCodes.insert (KEY_CAPSLOCK,     Awesomium::KeyCodes::AK_CAPITAL);
	sKeyCodes.insert (KEY_ESCAPE,       Awesomium::KeyCodes::AK_ESCAPE);
	sKeyCodes.insert (KEY_SPACE,        Awesomium::KeyCodes::AK_SPACE);
	sKeyCodes.insert (KEY_PAGE_DOWN,    Awesomium::KeyCodes::AK_NEXT);
	sKeyCodes.insert (KEY_PAGE_UP,      Awesomium::KeyCodes::AK_PRIOR);
	sKeyCodes.insert (KEY_END,          Awesomium::KeyCodes::AK_END);
	sKeyCodes.insert (KEY_HOME,         Awesomium::KeyCodes::AK_HOME);
	sKeyCodes.insert (KEY_LEFT,         Awesomium::KeyCodes::AK_LEFT);
	sKeyCodes.insert (KEY_UP,           Awesomium::KeyCodes::AK_UP);
	sKeyCodes.insert (KEY_RIGHT,        Awesomium::KeyCodes::AK_RIGHT);
	sKeyCodes.insert (KEY_DOWN,         Awesomium::KeyCodes::AK_DOWN);
	sKeyCodes.insert (KEY_PRINT,        Awesomium::KeyCodes::AK_PRINT);
	sKeyCodes.insert (KEY_INSERT,       Awesomium::KeyCodes::AK_INSERT);
	sKeyCodes.insert (KEY_DELETE,		Awesomium::KeyCodes::AK_DELETE);
	sKeyCodes.insert (KEY_HELP,         Awesomium::KeyCodes::AK_HELP);

	sKeyCodes.insert (KEY_0,            Awesomium::KeyCodes::AK_0);
	sKeyCodes.insert (KEY_1,            Awesomium::KeyCodes::AK_1);
	sKeyCodes.insert (KEY_2,            Awesomium::KeyCodes::AK_2);
	sKeyCodes.insert (KEY_3,            Awesomium::KeyCodes::AK_3);
	sKeyCodes.insert (KEY_4,			Awesomium::KeyCodes::AK_4);
	sKeyCodes.insert (KEY_5,            Awesomium::KeyCodes::AK_5);
	sKeyCodes.insert (KEY_6,            Awesomium::KeyCodes::AK_6);
	sKeyCodes.insert (KEY_7,            Awesomium::KeyCodes::AK_7);
	sKeyCodes.insert (KEY_8,            Awesomium::KeyCodes::AK_8);
	sKeyCodes.insert (KEY_9,            Awesomium::KeyCodes::AK_9);

	sKeyCodes.insert (KEY_A,            Awesomium::KeyCodes::AK_A);
	sKeyCodes.insert (KEY_B,            Awesomium::KeyCodes::AK_B);
	sKeyCodes.insert (KEY_C,            Awesomium::KeyCodes::AK_C);
	sKeyCodes.insert (KEY_D,            Awesomium::KeyCodes::AK_D);
	sKeyCodes.insert (KEY_E,            Awesomium::KeyCodes::AK_E);
	sKeyCodes.insert (KEY_F,            Awesomium::KeyCodes::AK_F);
	sKeyCodes.insert (KEY_G,            Awesomium::KeyCodes::AK_G);
	sKeyCodes.insert (KEY_H,            Awesomium::KeyCodes::AK_H);
	sKeyCodes.insert (KEY_I,            Awesomium::KeyCodes::AK_I);
	sKeyCodes.insert (KEY_J,            Awesomium::KeyCodes::AK_J);
	sKeyCodes.insert (KEY_K,            Awesomium::KeyCodes::AK_K);
	sKeyCodes.insert (KEY_L,            Awesomium::KeyCodes::AK_L);
	sKeyCodes.insert (KEY_M,            Awesomium::KeyCodes::AK_M);
	sKeyCodes.insert (KEY_N,            Awesomium::KeyCodes::AK_N);
	sKeyCodes.insert (KEY_O,            Awesomium::KeyCodes::AK_O);
	sKeyCodes.insert (KEY_P,            Awesomium::KeyCodes::AK_P);
	sKeyCodes.insert (KEY_Q,            Awesomium::KeyCodes::AK_Q);
	sKeyCodes.insert (KEY_R,            Awesomium::KeyCodes::AK_R);
	sKeyCodes.insert (KEY_S,            Awesomium::KeyCodes::AK_S);
	sKeyCodes.insert (KEY_T,            Awesomium::KeyCodes::AK_T);
	sKeyCodes.insert (KEY_U,            Awesomium::KeyCodes::AK_U);
	sKeyCodes.insert (KEY_V,            Awesomium::KeyCodes::AK_V);
	sKeyCodes.insert (KEY_W,            Awesomium::KeyCodes::AK_W);
	sKeyCodes.insert (KEY_X,            Awesomium::KeyCodes::AK_X);
	sKeyCodes.insert (KEY_Y,            Awesomium::KeyCodes::AK_Y);
	sKeyCodes.insert (KEY_Z,            Awesomium::KeyCodes::AK_Z);

	sKeyCodes.insert (KEY_TILDE,		Awesomium::KeyCodes::AK_UNKNOWN);
	sKeyCodes.insert (KEY_MINUS,        Awesomium::KeyCodes::AK_OEM_MINUS);
	sKeyCodes.insert (KEY_EQUALS,       Awesomium::KeyCodes::AK_UNKNOWN);
	sKeyCodes.insert (KEY_LBRACKET,     Awesomium::KeyCodes::AK_OEM_4);
	sKeyCodes.insert (KEY_RBRACKET,     Awesomium::KeyCodes::AK_OEM_6);
	sKeyCodes.insert (KEY_BACKSLASH,    Awesomium::KeyCodes::AK_OEM_102);
	sKeyCodes.insert (KEY_SEMICOLON,    Awesomium::KeyCodes::AK_OEM_1);
	sKeyCodes.insert (KEY_APOSTROPHE,   Awesomium::KeyCodes::AK_UNKNOWN);
	sKeyCodes.insert (KEY_COMMA,        Awesomium::KeyCodes::AK_OEM_COMMA);
	sKeyCodes.insert (KEY_PERIOD,       Awesomium::KeyCodes::AK_OEM_PERIOD);
	sKeyCodes.insert (KEY_SLASH,        Awesomium::KeyCodes::AK_OEM_2);
	sKeyCodes.insert (KEY_NUMPAD0,      Awesomium::KeyCodes::AK_NUMPAD0);
	sKeyCodes.insert (KEY_NUMPAD1,      Awesomium::KeyCodes::AK_NUMPAD1);
	sKeyCodes.insert (KEY_NUMPAD2,      Awesomium::KeyCodes::AK_NUMPAD2);
	sKeyCodes.insert (KEY_NUMPAD3,      Awesomium::KeyCodes::AK_NUMPAD3);
	sKeyCodes.insert (KEY_NUMPAD4,      Awesomium::KeyCodes::AK_NUMPAD4);
	sKeyCodes.insert (KEY_NUMPAD5,      Awesomium::KeyCodes::AK_NUMPAD5);
	sKeyCodes.insert (KEY_NUMPAD6,      Awesomium::KeyCodes::AK_NUMPAD6);
	sKeyCodes.insert (KEY_NUMPAD7,      Awesomium::KeyCodes::AK_NUMPAD7);
	sKeyCodes.insert (KEY_NUMPAD8,      Awesomium::KeyCodes::AK_NUMPAD8);
	sKeyCodes.insert (KEY_NUMPAD9,      Awesomium::KeyCodes::AK_NUMPAD9);
	sKeyCodes.insert (KEY_MULTIPLY,     Awesomium::KeyCodes::AK_MULTIPLY);
	sKeyCodes.insert (KEY_ADD,			Awesomium::KeyCodes::AK_ADD);
	sKeyCodes.insert (KEY_SEPARATOR,    Awesomium::KeyCodes::AK_SEPARATOR);
	sKeyCodes.insert (KEY_SUBTRACT,     Awesomium::KeyCodes::AK_SUBTRACT);
	sKeyCodes.insert (KEY_DECIMAL,      Awesomium::KeyCodes::AK_DECIMAL);
	sKeyCodes.insert (KEY_DIVIDE,       Awesomium::KeyCodes::AK_DIVIDE);
	sKeyCodes.insert (KEY_NUMPADENTER,  Awesomium::KeyCodes::AK_RETURN);

	sKeyCodes.insert (KEY_F1,           Awesomium::KeyCodes::AK_F1);
	sKeyCodes.insert (KEY_F2,           Awesomium::KeyCodes::AK_F2);
	sKeyCodes.insert (KEY_F3,           Awesomium::KeyCodes::AK_F3);
	sKeyCodes.insert (KEY_F4,           Awesomium::KeyCodes::AK_F4);
	sKeyCodes.insert (KEY_F5,           Awesomium::KeyCodes::AK_F5);
	sKeyCodes.insert (KEY_F6,           Awesomium::KeyCodes::AK_F6);
	sKeyCodes.insert (KEY_F7,           Awesomium::KeyCodes::AK_F7);
	sKeyCodes.insert (KEY_F8,           Awesomium::KeyCodes::AK_F8);
	sKeyCodes.insert (KEY_F9,           Awesomium::KeyCodes::AK_F9);
	sKeyCodes.insert (KEY_F10,          Awesomium::KeyCodes::AK_F10);
	sKeyCodes.insert (KEY_F11,          Awesomium::KeyCodes::AK_F11);
	sKeyCodes.insert (KEY_F12,          Awesomium::KeyCodes::AK_F12);
	sKeyCodes.insert (KEY_F13,          Awesomium::KeyCodes::AK_F13);
	sKeyCodes.insert (KEY_F14,          Awesomium::KeyCodes::AK_F14);
	sKeyCodes.insert (KEY_F15,          Awesomium::KeyCodes::AK_F15);
	sKeyCodes.insert (KEY_F16,          Awesomium::KeyCodes::AK_F16);
	sKeyCodes.insert (KEY_F17,          Awesomium::KeyCodes::AK_F17);
	sKeyCodes.insert (KEY_F18,          Awesomium::KeyCodes::AK_F18);
	sKeyCodes.insert (KEY_F19,          Awesomium::KeyCodes::AK_F19);
	sKeyCodes.insert (KEY_F20,          Awesomium::KeyCodes::AK_F20);
	sKeyCodes.insert (KEY_F21,          Awesomium::KeyCodes::AK_F21);
	sKeyCodes.insert (KEY_F22,          Awesomium::KeyCodes::AK_F22);
	sKeyCodes.insert (KEY_F23,          Awesomium::KeyCodes::AK_F23);
	sKeyCodes.insert (KEY_F24,          Awesomium::KeyCodes::AK_F24);

	sKeyCodes.insert (KEY_NUMLOCK,      Awesomium::KeyCodes::AK_NUMLOCK);
	sKeyCodes.insert (KEY_SCROLLLOCK,   Awesomium::KeyCodes::AK_SCROLL);
	sKeyCodes.insert (KEY_LCONTROL,     Awesomium::KeyCodes::AK_LCONTROL);
	sKeyCodes.insert (KEY_RCONTROL,     Awesomium::KeyCodes::AK_RCONTROL);
	sKeyCodes.insert (KEY_LALT,         Awesomium::KeyCodes::AK_LMENU);
	sKeyCodes.insert (KEY_RALT,         Awesomium::KeyCodes::AK_RMENU);
	sKeyCodes.insert (KEY_LSHIFT,		Awesomium::KeyCodes::AK_LSHIFT);
	sKeyCodes.insert (KEY_RSHIFT,       Awesomium::KeyCodes::AK_RSHIFT);
}

void AwManager::readConsoleVariables ()
{
	sRayLengthScale	= Con::getFloatVariable ("$pref::Awesomium::RayLengthScale", 2.0f);
	sImageDropSpeed	= Con::getFloatVariable ("$pref::Awesomium::ImageDropSpeed", 2.0f);
	sLoadBalancingDistance = Con::getFloatVariable ("$pref::Awesomium::LoadBalancingDistance", 50.0f);
	sMaxIterationsPerFrame = Con::getIntVariable ("$pref::Awesomium::MaxIterationsPerFrame", 64);
}

void AwManager::onPreRender (SceneManager *sceneManager, const SceneRenderState *state)
{
	SceneObject *controlObject = GameConnection::getConnectionToServer () ? GameConnection::getConnectionToServer ()->getControlObject () : nullptr;

	if (!controlObject || sceneManager != controlObject->getSceneManager ())
	{
		return;
	}

	sNumFrames++;

	// Every second..
	U32 time = Platform::getRealMilliseconds ();
	if (sNextUpdateTime < time)
	{
		sFramerate = sNumFrames;
		sNumFrames = 0;

		readConsoleVariables ();

		sNextUpdateTime = time + 1000;
	}

	if (sFramerate > 0)
	{
		for (S32 i = 0; i + sCurrentTargetIndex < sTargets.size () && i < sMaxIterationsPerFrame; i++)
		{
			sTargets [sCurrentTargetIndex + i]->update (sFramerate);
		}

		sCurrentTargetIndex += sMaxIterationsPerFrame;
		if (sCurrentTargetIndex + 1 >= sTargets.size ())
		{
			sCurrentTargetIndex = 0;
		}

		for (S32 i = 0; i + sCurrentShapeIndex < sShapes.size () && i < sMaxIterationsPerFrame; i++)
		{
			AwShape *shape = sShapes [sCurrentShapeIndex + i];

			// Calculate shape distance. This value is used to load-balance and decrease the framerate of the targets.
			if (shape->getTextureTarget () && shape->getTextureTarget ()->getFramerate () == 0 && shape->getTextureTarget () != AwTextureTarget::sMouseInputTarget)
			{
				shape->getTextureTarget ()->setDistance ((shape->getRenderPosition () - controlObject->getRenderPosition ()).len ());
			}
		}

		sCurrentShapeIndex += sMaxIterationsPerFrame;
		if (sCurrentShapeIndex + 1 >= sShapes.size ())
		{
			sCurrentShapeIndex = 0;
		}
	}
}

bool AwManager::onDeviceEvent (GFXDevice::GFXDeviceEventType evt)
{
	if (AwTextureTarget::sMouseInputTarget)
	{
		// Inject mouse movement based on our interpolated render position.
		AwTextureTarget::sMouseInputTarget->injectMouseMove (sCursor->getRenderPosition ());
	}

	if (evt == GFXDevice::deStartOfFrame)
	{
		Awesomium::WebCore::instance ()->Update ();
	}

	return true;
}

AwTextureTarget *AwManager::findTextureTargetByMaterial (BaseMatInstance *mat)
{
	PROFILE_SCOPE (AwManager_findTextureTarget);
	AwTextureTarget *target;
	if (sTargetsByMaterial.tryGetValue (mat, target))
	{
		return target;
	}

	return nullptr;
}

void AwManager::addShape (AwShape *shape)
{
	sShapes.push_back (shape);

	TSShapeInstance *shapeInstance = shape->getShapeInstance ();
	TSMaterialList *list = shapeInstance->getMaterialList ();
	if (!shapeInstance || !list)
	{
		return;
	}

	// Go trough all materials on the AwShape and map them to their TextureTargets.
	for (U32 i = 0; i < list->size (); i++)
	{
		Material *def = (Material *)list->getMaterialInst (i)->getMaterial ();
		for (U32 j = 0; j < Material::MAX_STAGES; j++)
		{
			AwTextureTarget *target;
			if (sTextureTargetsByName.tryGetValue (def->mDiffuseMapFilename [j], target))
			{
				target->mNumShapesBound++;
				sTargetsByMaterial.insert (list->getMaterialInst (i), target);
				break;
			}
		}
	}
}

void AwManager::removeShape (AwShape *shape)
{
	sShapes.remove (shape); // TODO: This operation is O(n) which might be a problem with huge amounts of shapes or if shapes are often removed.
	if (shape->mTextureTarget)
	{
		shape->mTextureTarget->mNumShapesBound--;
		if (!shape->mTextureTarget->mNumShapesBound)
		{
			sTargetsByMaterial.erase (shape->mMatInstance);
		}
	}
}

void AwManager::addTextureTarget (AwTextureTarget *target)
{
	sTargets.push_back (target);
	sTextureTargetsByName.insert ("#" + target->mTexTargetName, target);
}

void AwManager::removeTextureTarget (AwTextureTarget *target)
{
	sTargets.remove (target); // TODO: This operation is O(n) which might be a problem with huge amounts of targets or if targets are often removed.
	sTextureTargetsByName.erase ("#" + target->mTexTargetName);
}

void AwManager::shutdown ()
{
	if (!Awesomium::WebCore::instance ())
	{
		return;
	}

	delete sDataSource;
	sDataSource = nullptr;

	Map <String, Awesomium::WebSession *>::Iterator iter;
	for (iter = sSessions.begin (); iter != sSessions.end (); iter++)
	{
		iter->value->Release ();
	}
	
	Awesomium::WebCore::Shutdown ();

	GFXDevice::getDeviceEventSignal ().remove (onDeviceEvent);
}

Awesomium::WebSession *AwManager::getSessionFromPath (const String &path)
{
	Awesomium::WebSession *session = nullptr;
	if (sSessions.tryGetValue (path, session))
	{
		return session;
	}

	// Creates a web session and registers the data source so we can use from Torque's file system.
	session = Awesomium::WebCore::instance ()->CreateWebSession (Awesomium::WebString::CreateFromUTF8 (path.utf8 (), path.length ()), Awesomium::WebPreferences ());
	if (session)
	{
		session->AddDataSource (Awesomium::WSLit ("torque"), sDataSource);
		sSessions.insert (path, session);	
	}
	return session;
}

