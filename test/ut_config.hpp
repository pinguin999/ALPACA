#pragma once

#include <boost/ut.hpp>

// boost.ut's default config uses reporter_junit, which redirects std::cout into
// an internal buffer during tests and only flushes it on failure, making any
// output produced while tests run invisible. Override the config to use the
// plain reporter, which writes through to std::cout directly.
//
// This must be included (before any test registration) by every translation
// unit that uses boost.ut, otherwise some TUs would instantiate the default
// config while others use this one, which is an ODR violation. The `inline`
// keyword gives the specialization vague linkage so the definitions merge.
template <>
inline auto boost::ut::cfg<boost::ut::override> =
    boost::ut::runner<boost::ut::reporter<>>{};
