#include "LegacyCpp/IAICallback.h"

#include "./XAICommand.hpp"
#include "./XAICommandTracker.hpp"
#include "../main/XAIHelper.hpp"

int XAICommand::Send(XAIHelper* h) {
	Command* c = (Command*) this;

	if (tracked) {
		h->cmdTracker->Track(c, h->GetCurrFrame());
	}

	return (h->rcb->GiveOrder(unitID, c));
}
