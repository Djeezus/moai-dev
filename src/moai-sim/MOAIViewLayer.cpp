// Copyright (c) 2010-2017 Zipline Games, Inc. All Rights Reserved.
// http://getmoai.com

#include "pch.h"
#include <moai-sim/MOAICamera.h>
#include <moai-sim/MOAIDebugLines.h>
#include <moai-sim/MOAIDeck.h>
#include <moai-sim/MOAIFrameBufferTexture.h>
#include <moai-sim/MOAIGfxMgr.h>
#include <moai-sim/MOAIViewLayer.h>
#include <moai-sim/MOAIMaterialMgr.h>
#include <moai-sim/MOAIPartition.h>
#include <moai-sim/MOAIPartitionResultBuffer.h>
#include <moai-sim/MOAIPartitionResultMgr.h>
#include <moai-sim/MOAIRenderMgr.h>
#include <moai-sim/MOAIShaderMgr.h>
#include <moai-sim/MOAITextureBase.h>
#include <moai-sim/MOAITransform.h>
#include <moai-sim/MOAIVertexFormatMgr.h>
#include <moai-sim/MOAIViewProj.h>

//================================================================//
// local
//================================================================//

//----------------------------------------------------------------//
/**	@lua	getCamera
	@text	Get the camera associated with the layer.
	
	@in		MOAIViewLayer self
	@out	MOAICamera camera
*/
int MOAIViewLayer::_getCamera ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "U" )
	state.Push (( MOAILuaObject* )self->mCamera );
	return 1;
}

//----------------------------------------------------------------//
/**	@lua	getFitting
	@text	Computes a camera fitting for a given world rect along with
			an optional screen space padding. To do a fitting, compute
			the world rect based on whatever you are fitting to, use
			this method to get the fitting, then animate the camera
			to match.
	
	@in		MOAIViewLayer self
	@in		number xMin
	@in		number yMin
	@in		number xMax
	@in		number yMax
	@opt	number xPad
	@opt	number yPad
	@out	number x		X center of fitting (use for camera location).
	@out	number y		Y center of fitting (use for camera location).
	@out	number s		Scale of fitting (use for camera scale).
*/
int MOAIViewLayer::_getFitting ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "UNNNN" )

	ZLRect worldRect;
	worldRect.mXMin = state.GetValue < float >( 2, 0.0f );
	worldRect.mYMin = state.GetValue < float >( 3, 0.0f );
	worldRect.mXMax = state.GetValue < float >( 4, 0.0f );
	worldRect.mYMax = state.GetValue < float >( 5, 0.0f );

	worldRect.Bless ();

	float hPad = state.GetValue < float >( 6, 0.0f );
	float vPad = state.GetValue < float >( 7, 0.0f );

	float x = worldRect.mXMin + (( worldRect.mXMax - worldRect.mXMin ) * 0.5f );
	float y = worldRect.mYMin + (( worldRect.mYMax - worldRect.mYMin ) * 0.5f );

	lua_pushnumber ( state, x );
	lua_pushnumber ( state, y );

	float fitting = self->GetFitting ( worldRect, hPad, vPad );
	lua_pushnumber ( state, fitting );

	return 3;
}

//----------------------------------------------------------------//
/**	@lua	getFitting3D
	@text	Find a position for the camera where all given locations or
			props will be visible without changing the camera's orientation
			(i.e. orient the camera first, then call this to derive the
			correct position).
	
	@in		MOAIViewLayer self
	@in		table targets		A table of either props or locations. Locations are tables containing {x, y, z, r}.
	@out	number x
	@out	number y
	@out	number z
*/
int MOAIViewLayer::_getFitting3D ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "UT" )

	if (( !self->mViewport ) || ( !self->mCamera ) || ( self->mCamera->GetType () != MOAICamera::CAMERA_TYPE_3D )) return 0;
	
	ZLRect fitRect = state.GetValue < ZLRect >( 3, *self->mViewport );
	
	self->mCamera->ForceUpdate ();
	
	ZLFrustumFitter fitter;
	
	fitter.Init (
		*self->mViewport,
		fitRect,
		self->mCamera->GetFieldOfView (),
		self->mCamera->GetLocalToWorldMtx ()
	);

	u32 itr = state.PushTableItr ( 2 );
	while ( state.TableItrNext ( itr )) {
	
		int type = lua_type ( state, -1 );
		
		switch ( type ) {
		
			case LUA_TTABLE: {
			
				ZLVec3D loc;
				
				loc.mX = state.GetFieldValue < float >( -1, "x", 0.0f );
				loc.mY = state.GetFieldValue < float >( -1, "y", 0.0f );
				loc.mZ = state.GetFieldValue < float >( -1, "z", 0.0f );
				
				float r = state.GetFieldValue < float >( -1, "r", 0.0f );
				
				fitter.FitPoint( loc, r );
				
				break;
			}
			
			case LUA_TUSERDATA: {
			
				MOAIPartitionHull* hull = state.GetLuaObject < MOAIPartitionHull >( -1, true );
		
				if ( hull ) {
					ZLBox bounds = hull->GetWorldBounds ();
					
					ZLVec3D center;
					bounds.GetCenter ( center );
					fitter.FitBox ( bounds, 0.0f );
				}
				break;
			}
		}
	}
	
	ZLVec3D position = fitter.GetPosition ();
	
	state.Push ( position.mX );
	state.Push ( position.mY );
	state.Push ( position.mZ );

	return 3;
}

//----------------------------------------------------------------//
/**	@lua	getViewport
	@text	Return the viewport currently associated with the layer.
	
	@in		MOAIViewLayer self
	@out	MOAILuaObject viewport
*/
int MOAIViewLayer::_getViewport ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "U" )
	state.Push (( MOAILuaObject* )self->mViewport );
	return 1;
}

//----------------------------------------------------------------//
/**	@lua	setCamera
	@text	Sets a camera for the layer. If no camera is supplied,
			layer will render using the identity matrix as view/proj.
	
	@overload
	
		@in		MOAIViewLayer self
		@opt	MOAICamera camera		Default value is nil.
		@out	nil
	
	@overload
	
		@in		MOAIViewLayer self
		@opt	MOAICamera2D camera		Default value is nil.
		@out	nil
*/
int MOAIViewLayer::_setCamera ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "U" )

	self->mCamera.Set ( *self, state.GetLuaObject < MOAICamera >( 2, true ));

	return 0;
}

//----------------------------------------------------------------//
// TODO: doxygen
int MOAIViewLayer::_setDebugCamera ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "U" )

	self->mDebugCamera.Set ( *self, state.GetLuaObject < MOAICamera >( 2, true ));

	return 0;
}

//----------------------------------------------------------------//
/**	@lua	setParallax
	@text	Sets the parallax scale for this layer. This is simply a
			scalar applied to the view transform before rendering.
	
	@in		MOAIViewLayer self
	@opt	number xParallax	Default value is 1.
	@opt	number yParallax	Default value is 1.
	@opt	number zParallax	Default value is 1.
	@out	nil
*/
int MOAIViewLayer::_setParallax ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "U" )

	self->mParallax.mX = state.GetValue < float >( 2, 1.0f );
	self->mParallax.mY = state.GetValue < float >( 3, 1.0f );
	self->mParallax.mZ = state.GetValue < float >( 4, 1.0f );

	return 0;
}

//----------------------------------------------------------------//
/**	@lua	setViewport
	@text	Set the layer's viewport.
	
	@in		MOAIViewLayer self
	@in		MOAIViewport viewport
	@out	nil
*/
int MOAIViewLayer::_setViewport ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "UU" )

	self->mViewport.Set ( *self, state.GetLuaObject < MOAIViewport >( 2, true ));

	return 0;
}

//----------------------------------------------------------------//
/**	@lua	showDebugLines
	@text	Display debug lines for props in this layer.
	
	@in		MOAIViewLayer self
	@opt	boolean showDebugLines		Default value is 'true'.
	@out	nil
*/
int	MOAIViewLayer::_showDebugLines ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "U" )
	
	self->mShowDebugLines = state.GetValue < bool >( 2, true );
	
	return 0;
}

//----------------------------------------------------------------//
/**	@lua	wndToWorld
	@text	Project a point from window space into world space.
	
	@in		MOAIViewLayer self
	@in		number x
	@in		number y
	@out	number x
	@out	number y
	@out	number z
*/
int MOAIViewLayer::_wndToWorld ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "UNN" )

	ZLMatrix4x4 worldToWnd = self->GetWorldToWndMtx ();

	ZLMatrix4x4 wndToWorld = worldToWnd;
	wndToWorld.Inverse ();

	ZLVec4D loc;
	loc.mX = state.GetValue < float >( 2, 0.0f );
	loc.mY = state.GetValue < float >( 3, 0.0f );
	loc.mZ = worldToWnd.m [ ZLMatrix4x4::C3_R2 ] / worldToWnd.m [ ZLMatrix4x4::C3_R3 ];
	loc.mW = 1.0f;

	if ( self->mCamera && ( self->mCamera->GetType () == MOAICamera::CAMERA_TYPE_3D )) {
		wndToWorld.Project ( loc );
	}
	else {
		wndToWorld.Transform ( loc );
	}

	lua_pushnumber ( state, loc.mX );
	lua_pushnumber ( state, loc.mY );
	lua_pushnumber ( state, loc.mZ );

	return 3;
}

//----------------------------------------------------------------//
/**	@lua	wndToWorldRay
	@text	Project a point from window space into world space and return
			a normal vector representing a ray cast from the point into
			the world away from the camera (suitable for 3D picking).
	
	@in		MOAIViewLayer self
	@in		number x
	@in		number y
	@in		number d	If non-zero, scale normal by dist to plane d units away from camera. Default is zero.
	@out	number x
	@out	number y
	@out	number z
	@out	number xn
	@out	number yn
	@out	number zn
*/
int MOAIViewLayer::_wndToWorldRay ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "UNN" )

	if ( self->mCamera ) {
		self->mCamera->ForceUpdate ();
	}

	ZLMatrix4x4 wndToWorld = self->GetWndToWorldMtx ();

	ZLVec4D loc;
	loc.mX = state.GetValue < float >( 2, 0.0f );
	loc.mY = state.GetValue < float >( 3, 0.0f );
	loc.mZ = 0.0f;
	loc.mW = 1.0f;

	float d = state.GetValue < float >( 4, 0.0f );

	ZLVec4D origin;

	if ( self->mCamera  && ( self->mCamera->GetType () == MOAICamera::CAMERA_TYPE_3D )) {
		const ZLAffine3D& localToWorldMtx = self->mCamera->GetLocalToWorldMtx ();
		ZLVec3D cameraLoc = localToWorldMtx.GetTranslation ();
		origin.mX = cameraLoc.mX;
		origin.mY = cameraLoc.mY;
		origin.mZ = cameraLoc.mZ;
	}
	else {
		origin = loc;
		wndToWorld.Project ( origin );
	}
	
	lua_pushnumber ( state, origin.mX );
	lua_pushnumber ( state, origin.mY );
	lua_pushnumber ( state, origin.mZ );

	ZLVec3D norm;

	if ( self->mCamera  && ( self->mCamera->GetType () == MOAICamera::CAMERA_TYPE_3D )) {
	
		wndToWorld.Project ( loc );
	
		norm.mX = loc.mX - origin.mX;
		norm.mY = loc.mY - origin.mY;
		norm.mZ = loc.mZ - origin.mZ;
		norm.Norm ();
	}
	else {
		
		norm.mX = 0.0f;
		norm.mY = 0.0f;
		norm.mZ = -1.0f;
	}

	float ns = 1.0f;
	
	if ( d != 0.0f ) {
	
		if ( self->mCamera  && ( self->mCamera->GetType () == MOAICamera::CAMERA_TYPE_3D )) {
			const ZLAffine3D& localToWorldMtx = self->mCamera->GetLocalToWorldMtx ();
			ZLVec3D zAxis = localToWorldMtx.GetZAxis ();
			ns = -( d * zAxis.Dot ( norm ));
		}
		else {
			ns = d;
		}
	}
	
	lua_pushnumber ( state, norm.mX * ns );
	lua_pushnumber ( state, norm.mY * ns );
	lua_pushnumber ( state, norm.mZ * ns );

	return 6;
}

//----------------------------------------------------------------//
/**	@lua	worldToWnd
	@text	Transform a point from world space to window space.
	
	@in		MOAIViewLayer self
	@in		number x
	@in		number y
	@in		number Z
	@out	number x
	@out	number y
	@out	number z
*/
int MOAIViewLayer::_worldToWnd ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAIViewLayer, "UNN" )

	ZLVec4D loc;
	loc.mX = state.GetValue < float >( 2, 0.0f );
	loc.mY = state.GetValue < float >( 3, 0.0f );
	loc.mZ = state.GetValue < float >( 4, 0.0f );
	loc.mW = 1.0f;

	ZLMatrix4x4 worldToWnd = self->GetWorldToWndMtx ();
	worldToWnd.Project ( loc );

	lua_pushnumber ( state, loc.mX );
	lua_pushnumber ( state, loc.mY );
	lua_pushnumber ( state, loc.mZ );

	return 3;
}

//================================================================//
// MOAIViewLayer
//================================================================//

//----------------------------------------------------------------//
float MOAIViewLayer::GetFitting ( ZLRect& worldRect, float hPad, float vPad ) {

	if ( !( this->mCamera && this->mViewport )) return 1.0f;

	ZLRect viewRect = this->mViewport->GetRect ();
	
	float hFit = ( viewRect.Width () - ( hPad * 2 )) / worldRect.Width ();
	float vFit = ( viewRect.Height () - ( vPad * 2 )) / worldRect.Height ();
	
	return ( hFit < vFit ) ? hFit : vFit;
}

//----------------------------------------------------------------//
ZLMatrix4x4 MOAIViewLayer::GetWndToWorldMtx () const {

	return MOAIViewProj::GetWndToWorldMtx ( this->mViewport, this->mCamera, this->mLocalToWorldMtx, this->mParallax );
}

//----------------------------------------------------------------//
ZLMatrix4x4 MOAIViewLayer::GetWorldToWndMtx () const {

	return MOAIViewProj::GetWorldToWndMtx ( this->mViewport, this->mCamera, this->mLocalToWorldMtx, this->mParallax );
}

//----------------------------------------------------------------//
MOAIViewLayer::MOAIViewLayer () :
	mParallax ( 1.0f, 1.0f, 1.0f ),
	mShowDebugLines ( true ) {
	
	RTTI_BEGIN
		RTTI_EXTEND ( MOAIGraphicsProp )
		RTTI_EXTEND ( MOAILayer )
	RTTI_END
}

//----------------------------------------------------------------//
MOAIViewLayer::~MOAIViewLayer () {

	this->mCamera.Set ( *this, 0 );
	this->mDebugCamera.Set ( *this, 0 );
	this->mViewport.Set ( *this, 0 );
}

//----------------------------------------------------------------//
void MOAIViewLayer::RegisterLuaClass ( MOAILuaState& state ) {

	MOAIGraphicsProp::RegisterLuaClass ( state );
	MOAILayer::RegisterLuaClass ( state );
}

//----------------------------------------------------------------//
void MOAIViewLayer::RegisterLuaFuncs ( MOAILuaState& state ) {
	
	MOAIGraphicsProp::RegisterLuaFuncs ( state );
	MOAILayer::RegisterLuaFuncs ( state );
	
	luaL_Reg regTable [] = {
		{ "getCamera",				_getCamera },
		{ "getFitting",				_getFitting },
		{ "getFitting3D",			_getFitting3D },
		{ "getViewport",			_getViewport },
		{ "setDebugCamera",			_setDebugCamera },
		{ "setCamera",				_setCamera },
		{ "setParallax",			_setParallax },
		{ "setViewport",			_setViewport },
		{ "showDebugLines",			_showDebugLines },
		{ "wndToWorld",				_wndToWorld },
		{ "wndToWorldRay",			_wndToWorldRay },
		{ "worldToWnd",				_worldToWnd },
		{ NULL, NULL }
	};
	
	luaL_register ( state, 0, regTable );
}

//================================================================//
// ::implementation::
//================================================================//

//----------------------------------------------------------------//
void MOAIViewLayer::MOAIDrawable_Draw ( int subPrimID ) {
	UNUSED ( subPrimID );
    
   	if ( !this->IsVisible ()) return;
	if ( !this->mViewport ) return;
	if ( this->IsClear ()) return;
	
	MOAIGfxMgr& gfxMgr = MOAIGfxMgr::Get ();
	
	gfxMgr.mGfxState.SetFrameBuffer ( this->GetFrameBuffer ());
	
	MOAIViewport& viewport = *this->mViewport;
	ZLRect viewportRect = viewport;

	ZLMatrix4x4 mtx ( this->mLocalToWorldMtx );
	mtx.Transform ( viewportRect );

	gfxMgr.mGfxState.SetViewRect ( viewportRect );
	gfxMgr.mGfxState.SetScissorRect ( viewportRect );
	
	this->ClearSurface ();
	
	gfxMgr.mGfxState.SetViewProj ( this->mViewport, this->mCamera, this->mDebugCamera, this->mParallax );
	gfxMgr.mGfxState.SetMtx ( MOAIGfxGlobalsCache::MODEL_TO_WORLD_MTX );
	
	// set up the ambient color
	gfxMgr.mGfxState.SetAmbientColor ( this->mColor );
	
	this->MOAIViewLayer_Draw ();
	
	if ( MOAIDebugLinesMgr::Get ().IsVisible () && this->mShowDebugLines ) {
		if ( this->mCamera ) {
			this->mCamera->DrawDebug ();
		}
	}
	gfxMgr.mGfxState.SetFrameBuffer ( this->GetFrameBuffer ());
}

//----------------------------------------------------------------//
ZLBounds MOAIViewLayer::MOAIPartitionHull_GetModelBounds () {
	
	if ( this->mViewport ) {
		ZLBounds bounds;
		bounds.Init ( this->mViewport->GetRect ());
		return bounds;
	}
	return ZLBounds::EMPTY;
}
