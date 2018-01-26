#pragma once

#include <cstdint>
#include <atlstr.h>

#if _DEBUG
#define NDEBUG 1

#define checkf(expr, format, ...) if (!(expr))																\
{																											\
    fprintf(stdout, "CHECK FAILED: %s:%d:%s " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);	\
    CString dbgstr;																							\
    dbgstr.Format(("%s"), format);																			\
    MessageBox(NULL,dbgstr, "FATAL ERROR", MB_OK);															\
	DebugBreak();																							\
}
#else
#undef NDEBUG
#define checkf(expr, format, ...) ;
#endif
