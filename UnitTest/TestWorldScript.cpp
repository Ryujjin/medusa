/**
	@file TestWorldScript.cpp

	Drives WorldContext::runScript to confirm the World script path creates a Lua state,
	runs a script, and releases it again.
*/

#include "UnitTest.h"
#include "World/WorldContext.h"

extern "C" {
	#include "ThirdParty/LuaJIT/src/lua.h"
}

//---------------------------------------------------------------------------------------------------

class TestWorldScript : public UnitTest
{
public:
	TestWorldScript( const char * pName ) : UnitTest( pName )
	{}

	void test()
	{
		WorldContext world;
		const qword nScriptId = 1;

		const char * pScript =
			"result = 0\n"
			"for i = 1, 1000 do result = result + i end\n"
			"hasJit = (jit ~= nil)\n";
		Test( world.runScript( nScriptId, "UnitTest", pScript ) );
		Test( world.scriptCount() == 1 );

		lua_State * L = world.script( 0 ).pScript;
		Test( L != NULL );

		lua_getglobal( L, "result" );
		Test( lua_tonumber( L, -1 ) == 500500 );
		lua_pop( L, 1 );

#ifndef USE_PLAIN_LUA
		lua_getglobal( L, "hasJit" );
		Test( lua_toboolean( L, -1 ) != 0 );
		lua_pop( L, 1 );
#endif

		Test( world.killScript( nScriptId ) );
		Test( world.scriptCount() == 0 );
	}
} TEST_WORLD_SCRIPT( "TestWorldScript" );

//---------------------------------------------------------------------------------------------------
// EOF
