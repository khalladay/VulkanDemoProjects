#pragma once

#if _DEBUG
#define NDEBUG 1

#define checkf(expr, format, ...) if (!(expr))																\
{																											\
    fprintf(stdout, "CHECK FAILED: %s:%d:%s " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);	\
	DebugBreak();																							\
}
#else
#undef NDEBUG
#define checkf(expr, format, ...) ;
#endif