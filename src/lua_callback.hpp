#pragma once

#include <sol/sol.hpp>

class SpineObject;

class LuaCallback {
public:
	explicit LuaCallback(sol::function callback, std::shared_ptr<sol::state> lua_state);
	void operator()();
	bool calls_function(const sol::function& function);

private:
	sol::function callback;
	std::shared_ptr<sol::state> lua_state;
	std::shared_ptr<SpineObject> thisObject;
	friend bool operator==(const LuaCallback&, const LuaCallback&);
};

bool operator==(const LuaCallback&, const LuaCallback&);
