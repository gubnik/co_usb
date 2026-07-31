#pragma once

#include <version>

#if defined(__cpp_lib_contracts)
#define COUSB_PRECONDITION(...) pre(__VA_ARGS__)
#else
#define COUSB_PRECONDITION(...)
#endif

#if defined(__cpp_lib_contracts)
#define COUSB_POSTCONDITION(...) post(__VA_ARGS__)
#else
#define COUSB_POSTCONDITION(...)
#endif

#if defined(__cpp_lib_contracts)
#define COUSB_ASSERT(...) contract_assert(__VA_ARGS__)
#else
#include <cassert>
#define COUSB_ASSERT(...) assert(__VA_ARGS__);
#endif
