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

#include "Core/Util/TDictionary.h"
#include "GFX/GFXDevice.h"
#include "Core/Util/tVector.h"

namespace Awesomium
{
	class WebCore;
	class WebSession;
};

class AwContext;
class SceneManager;
class SceneRenderState;
class BaseMatInstance;
class SceneObject;
class AwTextureCursor;
class AwDataSource;

/*
 *  AwManager
 *  -----------------------------------------------------------------------------------------------
 *	Book-keeping class which keeps track of all AwShapes and AwTextureTargets.
 */
class AwManager
{
	friend class AwTextureTarget;
	friend class AwShape;
	friend class AwContext;

	static AwDataSource *AwManager::sDataSource;							// Used to fetch data from Torque's filesystem. Required for using compressed packages.
	static Vector <AwShape *> sShapes;										// List of all currently instantiated AwShapes.
	static Vector <AwTextureTarget *> sTargets;								// List of all currently instantiated AwTargets.
	static Map <String, AwTextureTarget *> sTextureTargetsByName;			// Lookup table used to fetch AwTargets by their name.
	static Map <BaseMatInstance *, AwTextureTarget *> sTargetsByMaterial;	// Lookup table used to fetch AwTargets by their associated material instance.
	static Map <String, Awesomium::WebSession *> sSessions;					// Lookup table used to fetch sessions by their paths.
	static Map <U8, int> sKeyCodes;											// Lookup table used to translate keycodes between Awesomium and Torque.
	
	static U32 sCurrentTargetIndex;											// Index used to iterate trough lists in segments each frame instead of all at once which could result in too much time spent in one frame.							
	static U32 sCurrentShapeIndex;											// Index used to iterate trough lists in segments each frame instead of all at once which could result in too much time spent in one frame.		

	static U32 sNextUpdateTime;												// The next time we want to do periodic updates.									
	static U32 sNumFrames;													// The number of frames rendered so far the past second.
	static U32 sFramerate;													// Our estimated framerate, used in performance tuning of targets and shapes.												
	
	static AwTextureCursor *sCursor;

	static F32 sRayLengthScale;												// Scales the distance of the ray used to pick the input target.
	static F32 sImageDropSpeed;												// The speed at which the Players' ShapeImages are dropped when an AwShape receives focus. Set this to <= 0.0f to disable the feature.
	static F32 sLoadBalancingDistance;										// The distance for Load Balancing. A higher value sacrifices performance for quality.
	static U32 sMaxIterationsPerFrame;										// The maximum number of iterations done per frame.

	static Awesomium::WebSession *getSessionFromPath (const String &path);

	static void addShape (AwShape *shape);									// Adds the shape to the manager, goes trough the material and finds the texture targets. Requires that the targets have been added before this call.
	static void removeShape (AwShape *shape);								// Removes the shape from the manager.
	static void addTextureTarget (AwTextureTarget *target);					// Adds the target to the manager.
	static void removeTextureTarget (AwTextureTarget *target);				// Removes the target from the manager.
	static AwTextureTarget *findTextureTargetByMaterial (BaseMatInstance *mat); // Finds the texture target by passing in its associated material instance.

	static void readConsoleVariables ();	
	static void setupInput ();

	static bool onDeviceEvent (GFXDevice::GFXDeviceEventType evt);
	static void onPreRender (SceneManager *sceneManager, const SceneRenderState *state);
	static void onPostRender (SceneManager *sceneManager, const SceneRenderState *state);
	
public:
	static F32 getRayLengthScale () { return sRayLengthScale; }				// Scales the distance of the ray used to pick the input target.
	static F32 getImageDropSpeed () { return sImageDropSpeed; }				// The speed at which the Players' ShapeImages are dropped when an AwShape receives focus. Set this to 0.0f to disable the feature.
	static F32 getLoadBalancingDistance () { return sLoadBalancingDistance; } // The distance for Load Balancing. A higher value sacrifices performance for quality.
	
	static Vector <AwTextureTarget *> &getTargets () { return sTargets; }	// Returns all managed targets.

	static void init ();
	static void shutdown ();	
};