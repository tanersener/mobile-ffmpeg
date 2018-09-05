// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_UTILS_SCOPE_EXIT_H_
#define CHROMAPRINT_UTILS_SCOPE_EXIT_H_

namespace chromaprint {

#define SCOPE_EXIT_STRING_JOIN2(arg1, arg2) SCOPE_EXIT_DO_STRING_JOIN2(arg1, arg2)
#define SCOPE_EXIT_DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2

template <typename F>
struct ScopeExit {
	ScopeExit(F f) : f(f) {}
	~ScopeExit() { f(); }
	F f;
};

template <typename F>
ScopeExit<F> MakeScopeExit(F f) {
	return ScopeExit<F>(f);
};

#define SCOPE_EXIT(code) \
	auto SCOPE_EXIT_STRING_JOIN2(scope_exit_, __LINE__) = MakeScopeExit([&](){ code; })

};

#endif
