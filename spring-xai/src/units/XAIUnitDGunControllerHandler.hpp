#ifndef XAI_UNIT_DGUNCONTROLLER_HANDLER_HDR
#define XAI_UNIT_DGUNCONTROLLER_HANDLER_HDR

#include <map>
#include "../events/XAIIEventReceiver.hpp"

struct XAIIEvent;
class XAICUnitDGunController;
struct XAIHelper;

class XAICUnitDGunControllerHandler: public XAIIEventReceiver {
public:
	XAICUnitDGunControllerHandler(XAIHelper* h): xaih(h) {
	}

	bool AddController(int unitID);
	bool DelController(int unitID);

	XAICUnitDGunController* GetController(int unitID) const;

	void OnEvent(const XAIIEvent*);

private:
	std::map<int, XAICUnitDGunController*> controllers;
	XAIHelper* xaih;
};

#endif
