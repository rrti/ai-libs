#ifndef XAI_DEFINES_HDR
#define XAI_DEFINES_HDR

#define XAI_USE_LUA          1
#define XAI_DEBUG_PRINTS     1
#define XAI_CATCH_EXCEPTIONS 1

#if (XAI_CATCH_EXCEPTIONS != 0)
#define XAI_BEG_EXCEPTION        try {
#define XAI_END_EXCEPTION(l, s)  } catch (std::exception& e) {                      \
                                     l->Log(std::string(s) + "[" + e.what() + "]"); \
                                 }
#else
#define XAI_BEG_EXCEPTION
#define XAI_END_EXCEPTION(l, s)
#endif

#endif
