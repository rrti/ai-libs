#ifndef XAI_LUA_HDR
#define XAI_LUA_HDR

#include "./XAIDefines.hpp"

#if (XAI_USE_LUA == 1)
	#ifdef BUILDING_AI
		#include "lua.h"
		#include "lualib.h"
		#include "lauxlib.h"
	#else
		#include <lua5.1/lua.hpp>
	#endif
#endif

#endif
