#include <sstream>

#include "LegacyCpp/IAICallback.h"

#include "./XAIEventLogger.hpp"

void XAICEventLogger::OnEvent(const XAIIEvent* e) {
	switch (e->type) {
		case XAI_EVENT_BASE: {
			// we should never receive this event
			log << (e->ToString());
		} break;

		case XAI_EVENT_UNIT_CREATED: {
			const XAIUnitCreatedEvent* ee = dynamic_cast<const XAIUnitCreatedEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_UNIT_FINISHED: {
			const XAIUnitFinishedEvent* ee = dynamic_cast<const XAIUnitFinishedEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_UNIT_DESTROYED: {
			const XAIUnitDestroyedEvent* ee = dynamic_cast<const XAIUnitDestroyedEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_UNIT_IDLE: {
			const XAIUnitIdleEvent* ee = dynamic_cast<const XAIUnitIdleEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_UNIT_DAMAGED: {
			const XAIUnitDamagedEvent* ee = dynamic_cast<const XAIUnitDamagedEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_ENEMY_DAMAGED: {
			const XAIEnemyDamagedEvent* ee = dynamic_cast<const XAIEnemyDamagedEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_UNIT_MOVE_FAILED: {
			const XAIUnitMoveFailedEvent* ee = dynamic_cast<const XAIUnitMoveFailedEvent*>(e); log << (ee->ToString());
		} break;

		case XAI_EVENT_ENEMY_ENTER_LOS: {
			const XAIEnemyEnterLOSEvent* ee = dynamic_cast<const XAIEnemyEnterLOSEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_ENEMY_LEAVE_LOS: {
			const XAIEnemyLeaveLOSEvent* ee = dynamic_cast<const XAIEnemyLeaveLOSEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_ENEMY_ENTER_RADAR: {
			const XAIEnemyEnterRadarEvent* ee = dynamic_cast<const XAIEnemyEnterRadarEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_ENEMY_LEAVE_RADAR: {
			const XAIEnemyLeaveRadarEvent* ee = dynamic_cast<const XAIEnemyLeaveRadarEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_ENEMY_DESTROYED: {
			const XAIEnemyDestroyedEvent* ee = dynamic_cast<const XAIEnemyDestroyedEvent*>(e); log << (ee->ToString());
		} break;

		case XAI_EVENT_UNIT_GIVEN: {
			const XAIUnitGivenEvent* ee = dynamic_cast<const XAIUnitGivenEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_UNIT_CAPTURED: {
			const XAIUnitCapturedEvent* ee = dynamic_cast<const XAIUnitCapturedEvent*>(e); log << (ee->ToString());
		} break;

		case XAI_EVENT_INIT: {
			const XAIInitEvent* ee = dynamic_cast<const XAIInitEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_UPDATE: {
			// const XAIUpdateEvent* ee = dynamic_cast<const XAIUpdateEvent*>(e); log << (ee->ToString());
		} break;
		case XAI_EVENT_RELEASE: {
			const XAIReleaseEvent* ee = dynamic_cast<const XAIReleaseEvent*>(e); log << (ee->ToString());
		} break;
	}

	if (eventsByType.find(e->type) == eventsByType.end()) {
		eventsByType[e->type] = 1;
	} else {
		eventsByType[e->type] += 1;
	}

	if (eventsByFrame.find(e->frame) == eventsByFrame.end()) {
		eventsByFrame[e->frame] = 1;
	} else {
		eventsByFrame[e->frame] += 1;
	}
}

void XAICEventLogger::WriteLog() {
	std::stringstream ss;
	std::map<XAIEventType, unsigned int>::const_iterator it0;
	std::map<unsigned int, unsigned int>::const_iterator it1;

	ss << "\n";
	ss << "[event type, events per type]\n";
	ss << "\n";

	for (it0 = eventsByType.begin(); it0 != eventsByType.end(); it0++) {
		ss << (it0->first) << "\t" << (it0->second) << "\n";
	}

	ss << "\n";
	ss << "[frame number, events per frame]\n";
	ss << "\n";

	for (it1 = eventsByFrame.begin(); it1 != eventsByFrame.end(); it1++) {
		ss << (it1->first) << "\t" << (it1->second) << "\n";
	}

	log << ss.str();
	log.flush();
}
