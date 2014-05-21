#ifndef XAI_CREDITS_HDR
#define XAI_CREDITS_HDR

#include "../exports/XAIExports.hpp"

#define AI_NAME        std::string("XAI ") + aiexport_getVersion()
#define AI_DATE        __DATE__
#define AI_VERSION     (AI_NAME + " (built " + AI_DATE + ")")
#define AI_CREDITS     "(developed by Kloot)"

#endif
