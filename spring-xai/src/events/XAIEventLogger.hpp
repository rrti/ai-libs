#ifndef XAI_EVENTLOGGER_HDR
#define XAI_EVENTLOGGER_HDR

#include <map>

#include "./XAIIEventReceiver.hpp"
#include "./XAIIEvent.hpp"
#include "../utils/XAIILogger.hpp"

class XAICEventLogger: public XAIIEventReceiver, public XAIILogger {
public:
	void OnEvent(const XAIIEvent*);
	void WriteLog();

private:
	std::map<XAIEventType, unsigned int> eventsByType;
	std::map<unsigned int, unsigned int> eventsByFrame;
};

#endif
