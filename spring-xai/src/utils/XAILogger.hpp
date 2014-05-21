#ifndef XAI_LOGGER_HDR
#define XAI_LOGGER_HDR

#include <string>
#include <fstream>

#include "./XAIILogger.hpp"



class IAICallback;

enum LogLevel {
	LOG_BASIC = 0,
	LOG_DEBUG = 1,
	LOG_ERROR = 2,
};

class XAICLogger: public XAIILogger {
public:
	XAICLogger(IAICallback* cb): init(false) {
		name = GetLogName(cb);
		init = true;
	}
	~XAICLogger() {
	}

	std::string GetLogName(IAICallback*) const;

	template<typename T> XAICLogger& Log(const T& t, LogLevel lvl = LOG_BASIC) {
		switch (lvl) {
			case LOG_BASIC: {
				log << t; log << std::endl;
			} break;
			case LOG_DEBUG: {
			} break;
			case LOG_ERROR: {
			} break;
			default: {
			} break;
		}

		return *this;
	}

private:
	bool init;
	std::string name;
};

#define LOG_BASIC(l, m) { std::stringstream ss; ss << m; l->Log(ss.str(), LOG_BASIC); }
#define LOG_DEBUG(l, m) { std::stringstream ss; ss << m; l->Log(ss.str(), LOG_DEBUG); }
#define LOG_ERROR(l, m) { std::stringstream ss; ss << m; l->Log(ss.str(), LOG_ERROR); }

#endif
