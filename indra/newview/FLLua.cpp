/**
 * @file FLLua.cpp
 * @brief FlexLife Viewer Lua Integration Framework
 * @author N3X15
 *
 *  Copyright (C) 2008-2009 FlexLife Contributors
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * $Id$
 */

/**
* Structure:
*
* STATIC: FLLua
*  - sInstance=FLLua() (THREAD,INTERPRETER)
*/
#include "llviewerprecompiledheaders.h"
#include <vector>
#include <queue>
#include <boost/tokenizer.hpp>
#include "FLLua.h"

/* Lua libraries */
extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

/* LL Objs */
#include "llhttpclient.h"
#include "llnotify.h"
#include "lluuid.h"
#include "llagent.h"
#include "llconsole.h"
#include "llviewerobject.h"
#include "llchat.h"
#include "llfloaterchat.h"
#include "llstring.h"
#include "message.h"
#include "llviewercamera.h"
#include "llgl.h"
#include "llrender.h"
#include "llglheaders.h"
#include "llversionviewer.h"


/* Lua classes */
#include "LuaBase.h"
#include "LuaBase_f.h"

//#define LUA_HOOK_SPAM 

extern LLAgent gAgent;

extern "C" {
extern int luaopen_SL(lua_State* L); // declare the wrapped module
}

///////////////////////////////////////////////
// Hook Request 
///////////////////////////////////////////////

HookRequest::HookRequest(const char *name)
{
	mName=name;
}

void HookRequest::Add(const char *arg)
{
	mArgs.push_back(arg);
}


///////////////////////////////////////////////
// Lua Interpreter
///////////////////////////////////////////////
FLLua::FLLua() :
	LLThread("Lua")
{
	// Do nothing.
}

FLLua* FLLua::sInstance = NULL;
// static
FLLua* FLLua::getInstance()
{
	LL_WARNS("Lua") << "Lua interpreter should not be directly accessed!" << llendl;
	return sInstance;
}

// Static
void FLLua::init()
{
	LL_INFOS("Lua") << "Starting Lua..." << llendl;
	sInstance=new FLLua();
	sInstance->start();
	FLLua::callLuaHook("OnLuaInit",0);
	LL_INFOS("Lua") << "Lua started." << llendl;
}

/// Run the interpreter.
void FLLua::run()
{
	LL_INFOS("Lua") << "Thread initializing." << llendl;
	L = lua_open();

	LL_INFOS("Lua") << __LINE__ << ": Skipping lua_atpanic" << llendl;
	//lua_atpanic(L, luaOnPanic);


	LL_INFOS("Lua") << __LINE__ << ": Loading standard Lua libs" << llendl;
	luaL_openlibs(L);


	LL_INFOS("Lua") << __LINE__ << ": *** LOADING SWIG BINDINGS ***" << llendl;
	luaopen_SL(L);

	std::string  version; 

	LL_INFOS("Lua") << __LINE__ << ": Assigning _SLUA_VERSION" << llendl;
	// Assign _SLUA_VERSION, which contains the version number of the host viewer.
	version = llformat("_SLUA_VERSION=\"%d.%d.%d.%d\"",LL_VERSION_MAJOR,LL_VERSION_MINOR,LL_VERSION_PATCH,LL_VERSION_BUILD);
	luaL_dostring(L, version.c_str());


	LL_INFOS("Lua") << __LINE__ << ": Assigning _SLUA_CHANNEL" << llendl;	
	// Assign _SLUA_CHANNEL, which contains the channel name of the host client.
	version = llformat("_SLUA_CHANNEL=\"%s\"",LL_CHANNEL);
	luaL_dostring(L, version.c_str());


	LL_INFOS("Lua") << __LINE__ << ": Runfile (_init_.lua)" << llendl;
	RunFile(gDirUtilp->getExpandedFilename(FL_PATH_LUA,"_init_.lua"));

#if 0
	RunFile(gDirUtilp->getExpandedFilename(FL_PATH_MACROS,"unit_tests.lua"));
#endif


	LL_INFOS("Lua") << __LINE__ << ": *** THREAD LOOP STARTS HERE ***" << llendl;
	while(1)
	{
		// Process Hooks
		lockData();


		//LL_INFOS("Lua") << __LINE__ << ": Checking if hooks are empty" << llendl;
		if(!mQueuedHooks.empty())
		{
			//LL_INFOS("Lua") << __LINE__ << ": Hooks not empty" << llendl;
			// Peek at the top of the stack
			HookRequest *hr = NULL;
			hr = mQueuedHooks.front();
			mQueuedHooks.pop();
			this->ExecuteHook(hr);		
		} else {
			//LL_INFOS("Lua") << __LINE__ << ": Hooks empty" << llendl;
		}
		//LL_INFOS("Lua") << __LINE__ << ": Unlocking..." << llendl;
		unlockData();

		// Process Macros/Raw Commands
		//LL_INFOS("Lua") << __LINE__ << ": Locking again..." << llendl;
		lockData();

		//LL_INFOS("Lua") << __LINE__ << ": Checking if macro queue is empty..." << llendl;
		if(!mQueuedCommands.empty())
		{
		
			//LL_INFOS("Lua") << __LINE__ << ": Macro queued, executing it..." << llendl;

			// Top of stack
			std::string hr = mQueuedCommands.front();
			mQueuedCommands.pop();


			LL_INFOS("Lua") << __LINE__ << ": Processing a macro or command." << llendl;
			LL_INFOS("Lua") << __LINE__ << hr << llendl;

			// Is this shit a macro?
			if(FLLua::isMacro(hr))
				this->RunMacro(hr); // Run macro.
			else
				this->RunString(hr); // Run command.
		} else {			
			//LL_INFOS("Lua") << __LINE__ << ": Macro vector empty" << llendl;
		}
		unlockData();
		ms_sleep(10);
	}
}

void FLLua::callMacro(const std::string command)
{
	sInstance->mQueuedCommands.push(command);
}

void FLHooks_InitTable(lua_State *l, FLLua* lol)
{
	FLLua** ud = reinterpret_cast<FLLua**>(lua_newuserdata(l, sizeof(FLLua*)));
	
	*ud = lol;

	Lua_SetClass(l, "Hooks");
}
FLLua::~FLLua()
{
	lua_close(L);
}

void FLLua::RunString(std::string s)
{
	luaL_dostring(L,s.c_str());
}

void FLLua::RunFile(std::string  file)
{
	if(luaL_loadfile(L,file.c_str()) || lua_pcall(L,0,0,0))
	{
		std::string  errmsg(Lua_getErrorMessage(L));

		llwarns << "Unable to load " << file << ": " << errmsg << llendl;
			
		LuaError("Failed to load:");
		LuaError(file.c_str());
		LuaError(errmsg.c_str());
		LuaError("Aborting.");
	}
}

// static
bool FLLua::isMacro(const std::string str)
{
	return(str.substr(0,6)=="/macro" || str.substr(0,2)=="/m");
}

void FLLua::RunMacro(const std::string what)
{
	std::string  tokenized = std::string (what.c_str());

	BOOL found_macro = FALSE;
	BOOL first_token = TRUE;

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");
	tokenizer tokens(tokenized, sep);
	tokenizer::iterator token_iter;

	std::string  macroname="";
	std::stringstream args;
	for( token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		std::string  cur_token = token_iter->c_str();
		//llinfos << cur_token << llendl;
		
		if(cur_token=="/m" || cur_token=="/macro")
		{
			continue;
		}

		if(!found_macro)
		{
			macroname=std::string (cur_token);
			found_macro=true;
		}
		else
		{
			args << "\"" << cur_token << "\",";
		}
	}
	
	if(macroname=="")
	{
		LuaError("Macro Syntax: /m[acro] <MacroName>"
			//" [args ...]"
			);
		return;
	}
	std::string  macrofile="";
	
	macrofile =  gDirUtilp->getExpandedFilename(FL_PATH_MACROS,macroname+".lua");

	if(!gDirUtilp->fileExists(macrofile))
	{
		std::string  err("Unable to find " + macrofile + ". Aborting.");
		LuaError(err.c_str());
		return;
	}

	std::string  message("Executing Lua Macro "+macrofile);
	std::string  arglist("MACRO_ARGS={");
	// HACK.  Set global via luaL_dostring
	arglist.append(args.str()).append("};");
	luaL_dostring(L,arglist.c_str());
	this->RunFile(macrofile);
}

// Static
void FLLua::callLuaHook(const char *EventName,int numargs,...)
{
	va_list arglist;
    	va_start(arglist,numargs);

	HookRequest* hook=new HookRequest(EventName);
	for(int i = 0;i<numargs;i++)
        {
		hook->Add(va_arg(arglist,const char *));
        }

	FLLua::sInstance->mQueuedHooks.push(hook);
}

void FLLua::ExecuteHook(HookRequest *hook)
{
	lua_getglobal(L,"CallHook");
#ifdef LUA_HOOK_SPAM
	LL_INFOS("Lua") << "Firing event: " << hook->getName() << llendl;
#endif
	if(lua_isfunction(L,1))
	{
		lua_pushstring(L,hook->getName());
		for(int i = 0;i<hook->getNumArgs();i++)
        	{
			lua_pushstring(L,hook->getArg(i));
        	}

		if(lua_pcall(L,hook->getNumArgs()+1,0,0)!=0)
		{
			char errbuff[1024];
			sprintf(errbuff,"Error executing the %s hook: %s",hook->getName(),lua_tostring(L,-1));
			LuaError(errbuff);
		}
	}
}





void Lua_RegisterMethod(lua_State* l, const char* name, lua_CFunction fn)
{

	lua_pushstring(l, name);
	lua_pushvalue(l, -2);
	lua_pushcclosure(l, fn, 1);
	lua_settable(l, -3);

}

void Lua_CreateClassMetatable(lua_State* l, const char* name)
{

	lua_newtable(l);
	Lua_PushClass(l, name);
	lua_pushvalue(l, -2);
	lua_rawset(l, LUA_REGISTRYINDEX); // registry.name = metatable
	lua_pushvalue(l, -1);
	Lua_PushClass(l, name);
	lua_rawset(l, LUA_REGISTRYINDEX); // registry.metatable = name
	lua_pushliteral(l, "__index");
	lua_pushvalue(l, -2);
	lua_rawset(l, -3);
}

void Lua_CheckArgs(lua_State* l,int minArgs, int maxArgs, const char* errorMessage)
{
	int argc = lua_gettop(l);
	if (argc < minArgs || argc > maxArgs)
	{
		llerrs << errorMessage << llendl;
	}
}

 bool Lua_IsType(lua_State* l, int index, int id)
{
	// get registry[metatable]
	if (!lua_getmetatable(l, index))
		return false;
	
	lua_rawget(l, LUA_REGISTRYINDEX);

	if (lua_type(l, -1) != LUA_TSTRING)
	{
		llerrs << "Lua_IsType failed!  Unregistered class." << llendl; 
		lua_pop(l, 1);
		return false;
	}
	
	const char* classname = lua_tostring(l, -1);
	
	if (classname != NULL)
	{
		lua_pop(l, 1);
		return true;
	}
	
	lua_pop(l, 1);
	
	return false;
}

void Lua_PushClass(lua_State* l, const char* classname)
{
	lua_pushlstring(l, classname, strlen(classname));
}

int luaOnPanic(lua_State *L)
{	
	throw std::runtime_error("Lua Error: "+Lua_getErrorMessage(L));
	return 0;
}

std::string  Lua_getErrorMessage(lua_State *L)
{
	if (lua_gettop(L) > 0)
	{
		if (lua_isstring(L, -1))
			return lua_tostring(L, -1);
	}
	return "";
}

void Lua_SetClass(lua_State *l,const char* classname)
{
	Lua_PushClass(l,classname);
	lua_rawget(l, LUA_REGISTRYINDEX);
	if (lua_type(l, -1) != LUA_TTABLE)
		llwarns << "Metatable for " << classname << " not found!" << llendl;
	if (lua_setmetatable(l, -2) == 0)
		llwarns << "Error setting metatable for " << classname << llendl;
}


/*
void LuaSetAvOverlay(const char *uuid,int type)
{
	/// NOT IMPLEMENTED YET ///
}

LuaAvatarOverlay::LuaAvatarOverlay(LLViewerImage *ViewerImage):
mImage(ViewerImage)
{
}

LuaAvatarOverlay::~LuaAvatarOverlay()
{
}

void LuaAvatarOverlay::render()
{
	if(mImage==NULL)
		return;
	
	LLVector3 WORLD_UPWARD_DIRECTION = LLVector3( 0.0f, 0.0f, 1.0f );

	LLVector3 pos = mParentPos + WORLD_UPWARD_DIRECTION * 0.3f;
	LLGLSPipelineAlpha alpha_blend;
	LLGLDepthTest depth(GL_TRUE, GL_FALSE);
	LLVector3 l	= LLViewerCamera::getInstance()->getLeftAxis() * 0.192f;
	LLVector3 u	= LLViewerCamera::getInstance()->getUpAxis()   * 0.064f;

	LLVector3 bottomLeft	= pos + l - u;
	LLVector3 bottomRight	= pos - l - u;
	LLVector3 topLeft		= pos + l + u;
	LLVector3 topRight		= pos - l + u;

	mImage->bind(); 
		
	gGL.color4fv( LLColor4( 1.0f, 1.0f, 1.0f, 1.0f ).mV );	
	
	gGL.begin( LLVertexBuffer::TRIANGLE_STRIP );
		gGL.texCoord2i( 0,	0	); gGL.vertex3fv( bottomLeft.mV );
		gGL.texCoord2i( 1,	0	); gGL.vertex3fv( bottomRight.mV );
		gGL.texCoord2i( 0,	1	); gGL.vertex3fv( topLeft.mV );
	gGL.end();

	gGL.begin( LLVertexBuffer::TRIANGLE_STRIP );
		gGL.texCoord2i( 1,	0	); gGL.vertex3fv( bottomRight.mV );
		gGL.texCoord2i( 1,	1	); gGL.vertex3fv( topRight.mV );
		gGL.texCoord2i( 0,	1	); gGL.vertex3fv( topLeft.mV );
	gGL.end();
}

void LuaAvatarOverlay::setParentPos( const LLVector3 p )
{
	this->mParentPos=p;
}
*/
