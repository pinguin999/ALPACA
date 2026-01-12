#include "lua_callback.hpp"

#include "spine_object.hpp"

LuaCallback::LuaCallback(sol::function callback, std::shared_ptr<sol::state> lua_state)
: callback(std::move(callback)), lua_state(std::move(lua_state)),
  thisObject((*this->lua_state)["this"]) {
}

bool LuaCallback::calls_function(const sol::function& function) {
	return callback == function;
}

void LuaCallback::operator()() {
	lua_state->set("this", thisObject);
	callback();
}

bool operator==(const LuaCallback& a, const LuaCallback& b) {
	return a.callback == b.callback && a.thisObject == b.thisObject;
}
