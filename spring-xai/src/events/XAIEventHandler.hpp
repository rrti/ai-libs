#ifndef XAI_EVENTHANDLER_HDR
#define XAI_EVENTHANDLER_HDR

#include <map>

struct XAIIEvent;
class XAIIEventReceiver;

class XAICEventHandler {
public:
	XAICEventHandler() {}
	~XAICEventHandler() {}

	void AddReceiver(XAIIEventReceiver* r, int k) {
		if (evtReceivers.find(k) != evtReceivers.end()) {
			return;
		}

		evtReceivers[k] = r;
	}
	void DelReceivers() {
		for (std::map<int, XAIIEventReceiver*>::iterator it = evtReceivers.begin(); it != evtReceivers.end(); it++) {
			evtReceivers.erase(it->first);
		}
	}

	void NotifyReceivers(const XAIIEvent*) const;

private:
	std::map<int, XAIIEventReceiver*> evtReceivers;
};

#endif
