#include "./XAIEventHandler.hpp"
#include "./XAIIEventReceiver.hpp"

void XAICEventHandler::NotifyReceivers(const XAIIEvent* e) const {
	for (std::map<int, XAIIEventReceiver*>::const_iterator it = evtReceivers.begin(); it != evtReceivers.end(); it++) {
		it->second->OnEvent(e);
	}
}
