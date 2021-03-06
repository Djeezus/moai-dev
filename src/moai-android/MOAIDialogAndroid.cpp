// Copyright (c) 2010-2017 Zipline Games, Inc. All Rights Reserved.
// http://getmoai.com

#include "moai-core/pch.h"
#include "moai-sim/pch.h"

#include <jni.h>

#include <moai-android/JniUtils.h>
#include <moai-android/MOAIDialogAndroid.h>

extern JavaVM* jvm;

//================================================================//
// lua
//================================================================//

//----------------------------------------------------------------//
/**	@lua	showDialog
	@text	Show a native dialog to the user.
				
	@in		string		title			The title of the dialog box. Can be nil.
	@in		string		message			The message to show the user. Can be nil.
	@in		string		positive		The text for the positive response dialog button. Can be nil.
	@in		string		neutral			The text for the neutral response dialog button. Can be nil.
	@in		string		negative		The text for the negative response dialog button. Can be nil.
	@in		boolean		cancelable		Specifies whether or not the dialog is cancelable
	@opt	function	callback		A function to callback when the dialog is dismissed. Default is nil.
	@out 	nil
*/
int MOAIDialogAndroid::_showDialog ( lua_State* L ) {
	
	MOAILuaState state ( L );	
	
	cc8* title = lua_tostring ( state, 1 );
	cc8* message = lua_tostring ( state, 2 );
	cc8* positive = lua_tostring ( state, 3 );
	cc8* neutral = lua_tostring ( state, 4 );
	cc8* negative = lua_tostring ( state, 5 );
	bool cancelable = lua_toboolean ( state, 6 );
	
	if ( state.IsType ( 7, LUA_TFUNCTION )) {
		
		// NOTE: This is fragile. We're storing the callback function in a global variable,
		// effectively. Invoking the showDialog method multiple times in succession can
		// therefore lead to unpredictable results. In fact, it's unknown how Android itself
		// handles multiple invocations - are they queued? On iOS, UIAlertView is LIFO and
		// new invocations supersede previous ones, but once dismissed, the system continues
		// down the alert stack.
		MOAIDialogAndroid::Get ().mDialogCallback.SetRef ( state, 7 );
	}	
	
	JNI_GET_ENV ( jvm, env );

	MOAIJString jtitle = JNI_GET_JSTRING ( title );
	MOAIJString jmessage = JNI_GET_JSTRING ( message );
	MOAIJString jpositive = JNI_GET_JSTRING ( positive );
	MOAIJString jnuetral = JNI_GET_JSTRING ( neutral );
	MOAIJString jnegative = JNI_GET_JSTRING ( negative );

	jclass moai = env->FindClass ( "com/moaisdk/core/Moai" );
    if ( moai == NULL ) {

		ZLLogF ( ZLLog::CONSOLE, "MOAIDialogAndroid: Unable to find java class %s", "com/ziplinegames/moai/Moai" );
    }
    else {

    	jmethodID showDialog = env->GetStaticMethodID ( moai, "showDialog", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Z)V" );
    	if ( showDialog == NULL ) {

			ZLLogF ( ZLLog::CONSOLE, "MOAIDialogAndroid: Unable to find static java method %s", "showDialog" );
    	}
    	else {

			env->CallStaticVoidMethod ( moai, showDialog, ( jstring )jtitle, ( jstring )jmessage, ( jstring )jpositive, ( jstring )jnuetral, ( jstring )jnegative, cancelable );	
		}
	}
	
	return 0;
}

//================================================================//
// MOAIDialogAndroid
//================================================================//

//----------------------------------------------------------------//
MOAIDialogAndroid::MOAIDialogAndroid () {

	RTTI_SINGLE ( MOAILuaObject )
}

//----------------------------------------------------------------//
MOAIDialogAndroid::~MOAIDialogAndroid () {
}

//----------------------------------------------------------------//
void MOAIDialogAndroid::RegisterLuaClass ( MOAILuaState& state ) {

	state.SetField ( -1, "DIALOG_RESULT_POSITIVE",	( u32 )DIALOG_RESULT_POSITIVE );
	state.SetField ( -1, "DIALOG_RESULT_NEUTRAL",	( u32 )DIALOG_RESULT_NEUTRAL );
	state.SetField ( -1, "DIALOG_RESULT_NEGATIVE",	( u32 )DIALOG_RESULT_NEGATIVE );
	state.SetField ( -1, "DIALOG_RESULT_CANCEL",	( u32 )DIALOG_RESULT_CANCEL );

	luaL_Reg regTable [] = {
		{ "showDialog",	_showDialog },
		{ NULL, NULL }
	};

	luaL_register ( state, 0, regTable );
}

//----------------------------------------------------------------//
void MOAIDialogAndroid::NotifyDialogDismissed ( int dialogResult ) {

	if ( !mDialogCallback.IsNil ()) {
		
		MOAIScopedLuaState state = mDialogCallback.GetSelf ();

		lua_pushinteger ( state, dialogResult );
		
		state.DebugCall ( 1, 0 );
		
		mDialogCallback.Clear ();
	}
}

//================================================================//
// Miscellaneous JNI Functions
//================================================================//

//----------------------------------------------------------------//
extern "C" JNIEXPORT void JNICALL Java_com_ziplinegames_moai_Moai_AKUAppDialogDismissed ( JNIEnv* env, jclass obj, jint code ) {

	MOAIDialogAndroid::Get ().NotifyDialogDismissed ( code );
}