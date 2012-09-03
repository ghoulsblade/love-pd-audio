// myhello.cpp  ->  myhello.dll (msvc6 : dll example project, select c++:codegen:"multithreaded dll" and add luabin include&lib paths)
//~ #include <openal.h>
// win:vc6 : add openal.lib ?  to linker (and paths accordingly)

#include <stdio.h>
extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
}
#define PROJECT_TABLENAME "lovepdaudio"

#ifdef LUA_API
#undef LUA_API
#endif
#ifdef WIN32
#define LUA_API __declspec(dllexport)
#else
#define LUA_API
#endif

extern "C" {
	int LUA_API luaopen_lovepdaudio (lua_State *L);
}

static int L_helloworld (lua_State *L) {
	printf("lovepdaudio:hello world!\n");
	return 0;
}

int LUA_API luaopen_lovepdaudio (lua_State *L) {
	printf("luaopen_lovepdaudio\n");
	struct luaL_reg funlist[] = {
		{"helloworld",		L_helloworld},			
		{NULL, NULL},
	};
	luaL_openlib (L, PROJECT_TABLENAME, funlist, 0);
	return 1;
}

/* test.lua :
require("lovepdaudio")
lovepdaudio.helloworld() 
*/
