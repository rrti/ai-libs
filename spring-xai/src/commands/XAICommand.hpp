#ifndef XAI_COMMAND_HDR
#define XAI_COMMAND_HDR

#include "Sim/Units/CommandAI/Command.h"

struct XAIHelper;
struct XAICommand: public Command {
public:
	XAICommand(int uid, bool t): unitID(uid), tracked(t) {
	}

	XAICommand(int uid, bool t, const Command& c): unitID(uid), tracked(t) {
		*this = c;
	}

	XAICommand& operator = (const Command& c) {
		id      = c.id;
		options = c.options;
		params  = c.params;
		tag     = c.tag;
		timeOut = c.timeOut;

		return *this;
	}

	int Send(XAIHelper*);

private:
	int unitID;
	bool tracked;
};

#endif
