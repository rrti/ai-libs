#ifndef XAI_IEVENTRECEIVER_HDR
#define XAI_IEVENTRECEIVER_HDR

struct XAIIEvent;
class XAIIEventReceiver {
public:
	XAIIEventReceiver() {}
	virtual ~XAIIEventReceiver() {}

	virtual void OnEvent(const XAIIEvent*) {}
};

#endif
