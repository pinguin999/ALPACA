#include "lua_callback.hpp"

#include "spine_object.hpp"
#include <sol/protected_function_result.hpp>

LuaCallback::LuaCallback(sol::function callback, std::shared_ptr<sol::state> lua_state)
: callback(std::move(callback)), lua_state(std::move(lua_state)),
  thisObject((*this->lua_state)["this"]) {
}

bool LuaCallback::calls_function(const sol::function& function) {
	return callback == function;
}

void LuaCallback::operator()() {
	lua_state->set("this", thisObject);
	sol::protected_function_result result = callback();
	if(!result.valid())
	{
		const sol::error err = result;
		jngl::error("Error in Lua callback: {}", err.what());
	}
}

bool operator==(const LuaCallback& a, const LuaCallback& b) {
	return a.callback == b.callback && a.thisObject == b.thisObject;
}
