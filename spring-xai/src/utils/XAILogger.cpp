#include <ctime>
#include <sstream>

#include "LegacyCpp/IAICallback.h"

#include "./XAILogger.hpp"
#include "./XAIUtil.hpp"
#include "../main/XAIFolders.hpp"

std::string XAICLogger::GetLogName(IAICallback* cb) const {
	if (init) {
		return name;
	}

	time_t t;
	time(&t);
	struct tm* lt = localtime(&t);

	std::stringstream ss;
		ss << std::string(XAI_LOG_DIR);
		ss << "[";
		ss << XAIUtil::StringStripSpaces(cb->GetModName());
		ss << "-";
		ss << XAIUtil::StringStripSpaces(cb->GetMapName());
		ss << "][";
		ss << lt->tm_mon + 1;
		ss << "-";
		ss << lt->tm_mday;
		ss << "-";
		ss << lt->tm_year + 1900;
		ss << "][";
		ss << lt->tm_hour;
		ss << lt->tm_min;
		ss << "][team";
		ss << cb->GetMyTeam();
		ss << "]";

	std::string relName = ss.str();
	std::string absName = XAIUtil::GetAbsFileName(cb, relName);

	return absName;
}
