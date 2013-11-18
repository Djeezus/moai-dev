#include "stdafx.h"
#ifndef SLEDGEINPUTHANDLER_H
#define SLEDGEINPUTHANDLER_H

#include <moaicore/MOAIGlobals.h>
#include <moaicore/MOAIRtti.h>
#include <moaicore/MOAILua.h>

#include "UtilityTypes.h"
#include "SledgeInputManager.h"

/**
 * HID handler class; accessible from Lua as SledgeInputHandler. Provides
 * functions for setting thumbstick deadzones, and a bunch of other stuff too.
 *
 * @author	Jetha Chan
 * @date	14/09/2013
 */
class SledgeInputHandler :
	public MOAIGlobalClass <SledgeInputHandler, MOAILuaObject> {
private:
	
	void			DoAKUInit_Device		( SledgeDevice* p_sledgedevice );

	//----------------------------------------------------------------//
	static int		_setDeadzones				( lua_State* L );
	//static int		_getActiveControllerCount	( lua_State* L );
	static int		_quitGame ( lua_State* L );
	static SledgeInputManager* _manager;



public:	
	DECL_LUA_SINGLETON ( SledgeInputHandler )

	//----------------------------------------------------------------//
					SledgeInputHandler	();
					~SledgeInputHandler	();
	void			RegisterLuaClass	( MOAILuaState& state );
	void			RegisterLuaFuncs	( MOAILuaState& state );

	void			AKUInit				(  );

	static void		SetManager			( SledgeInputManager* p_manager );

};

#endif