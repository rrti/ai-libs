#ifndef XAI_IEVENT_HDR
#define XAI_IEVENT_HDR

#include <cstdio>
#include <string>
#include "System/float3.h"

enum XAIEventType {
	XAI_EVENT_BASE              = -1,
	XAI_EVENT_UNIT_CREATED      =  0,
	XAI_EVENT_UNIT_FINISHED     =  1,
	XAI_EVENT_UNIT_DESTROYED    =  2,
	XAI_EVENT_UNIT_IDLE         =  3,
	XAI_EVENT_UNIT_DAMAGED      =  4,
	XAI_EVENT_ENEMY_DAMAGED     =  5,
	XAI_EVENT_UNIT_MOVE_FAILED  =  6,
	XAI_EVENT_ENEMY_ENTER_LOS   =  7,
	XAI_EVENT_ENEMY_LEAVE_LOS   =  8,
	XAI_EVENT_ENEMY_ENTER_RADAR =  9,
	XAI_EVENT_ENEMY_LEAVE_RADAR = 10,
	XAI_EVENT_ENEMY_DESTROYED   = 11,

	XAI_EVENT_UNIT_GIVEN        = 12,
	XAI_EVENT_UNIT_CAPTURED     = 13,

	XAI_EVENT_INIT              = 14,
	XAI_EVENT_UPDATE            = 15,
	XAI_EVENT_RELEASE           = 16
};

class XAICEventHandler;
struct XAIIEvent {
public:
	XAIIEvent(): type(XAI_EVENT_BASE), frame(0) {
	}

	virtual std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=EventBase]\n", frame);
		return std::string(s);
	};

	XAIEventType type;
	unsigned int frame;

protected:
	mutable char s[512];
};


struct XAIUnitCreatedEvent: public XAIIEvent {
	XAIUnitCreatedEvent(): XAIIEvent() {
		type = XAI_EVENT_UNIT_CREATED;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=UnitCreated][unitID=%d, builderID=%d]\n", frame, unitID, builderID);
		return std::string(s);
	}

	int unitID; int builderID;
};

struct XAIUnitFinishedEvent: public XAIIEvent {
	XAIUnitFinishedEvent(): XAIIEvent() {
		type = XAI_EVENT_UNIT_FINISHED;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=UnitFinished][unitID=%d]\n", frame, unitID);
		return std::string(s);
	}

	int unitID;
};

struct XAIUnitDestroyedEvent: public XAIIEvent {
	XAIUnitDestroyedEvent(): XAIIEvent() {
		type = XAI_EVENT_UNIT_DESTROYED;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=UnitDestroyed][unitID=%d][attackerID=%d]\n", frame, unitID, attackerID);
		return std::string(s);
	}

	int unitID; int attackerID;
};

struct XAIUnitIdleEvent: public XAIIEvent {
	XAIUnitIdleEvent(): XAIIEvent() {
		type = XAI_EVENT_UNIT_IDLE;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=UnitIdle][unitID=%d]\n", frame, unitID);
		return std::string(s);
	}

	int unitID;
};

struct XAIUnitDamagedEvent: public XAIIEvent {
	XAIUnitDamagedEvent(): XAIIEvent() {
		type = XAI_EVENT_UNIT_DAMAGED;
	}
	std::string ToString() const {
		snprintf(s, 511,
			"[frame=%d][event=UnitDamaged][unitID=%d][attackerID=%d][damage=%f][dir=<%f, %f, %f>]\n",
			frame, unitID, attackerID, attackDamage, attackDir.x, attackDir.y, attackDir.z
		);
		return std::string(s);
	}

	int unitID; int attackerID;
	float attackDamage; float3 attackDir;
};

struct XAIEnemyDamagedEvent: public XAIIEvent {
	XAIEnemyDamagedEvent(): XAIIEvent() {
		type = XAI_EVENT_ENEMY_DAMAGED;
	}
	std::string ToString() const {
		snprintf(s, 511,
			"[frame=%d][event=EnemyDamaged][unitID=%d][attackerID=%d][damage=%f][dir=<%f, %f, %f>]\n",
			frame, unitID, attackerID, attackDamage, attackDir.x, attackDir.y, attackDir.z
		);
		return std::string(s);
	}

	int unitID; int attackerID;
	float attackDamage; float3 attackDir;
};

struct XAIUnitMoveFailedEvent: public XAIIEvent {
	XAIUnitMoveFailedEvent(): XAIIEvent() {
		type = XAI_EVENT_UNIT_MOVE_FAILED;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=UnitMoveFailed][unitID=%d]\n", frame, unitID);
		return std::string(s);
	}

	int unitID;
};


struct XAIEnemyEnterLOSEvent: public XAIIEvent {
	XAIEnemyEnterLOSEvent(): XAIIEvent() {
		type = XAI_EVENT_ENEMY_ENTER_LOS;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=EnemyEnterLOS][unitID=%d]\n", frame, unitID);
		return std::string(s);
	}

	int unitID;
};

struct XAIEnemyLeaveLOSEvent: public XAIIEvent {
	XAIEnemyLeaveLOSEvent(): XAIIEvent() {
		type = XAI_EVENT_ENEMY_LEAVE_LOS;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=EnemyLeaveLOS][unitID=%d]\n", frame, unitID);
		return std::string(s);
	}

	int unitID;
};

struct XAIEnemyEnterRadarEvent: public XAIIEvent {
	XAIEnemyEnterRadarEvent(): XAIIEvent() {
		type = XAI_EVENT_ENEMY_ENTER_RADAR;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=EnemyEnterRadar][unitID=%d]\n", frame, unitID);
		return std::string(s);
	}

	int unitID;
};

struct XAIEnemyLeaveRadarEvent: public XAIIEvent {
	XAIEnemyLeaveRadarEvent(): XAIIEvent() {
		type = XAI_EVENT_ENEMY_LEAVE_RADAR;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=EnemyLeaveRadar][unitID=%d]\n", frame, unitID);
		return std::string(s);
	}

	int unitID;
};

struct XAIEnemyDestroyedEvent: public XAIIEvent {
	XAIEnemyDestroyedEvent(): XAIIEvent() {
		type = XAI_EVENT_ENEMY_DESTROYED;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=EnemyDestroyed][unitID=%d][attackerID=%d]\n", frame, unitID, attackerID);
		return std::string(s);
	}

	int unitID; int attackerID;
};


struct XAIUnitGivenEvent: public XAIIEvent {
	XAIUnitGivenEvent(): XAIIEvent() {
		type = XAI_EVENT_UNIT_GIVEN;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=UnitGiven][unitID=%d][newUnitTeam=%d]\n", frame, unitID, newUnitTeam);
		return std::string(s);
	}

	int unitID; int newUnitTeam;
};

struct XAIUnitCapturedEvent: public XAIIEvent {
	XAIUnitCapturedEvent(): XAIIEvent() {
		type = XAI_EVENT_UNIT_CAPTURED;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=UnitCaptured][unitID=%d][oldUnitTeam=%d]\n", frame, unitID, oldUnitTeam);
		return std::string(s);
	}

	int unitID; int oldUnitTeam;
};


struct XAIInitEvent: public XAIIEvent {
	XAIInitEvent(): XAIIEvent() {
		type = XAI_EVENT_INIT;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=Init]\n", frame);
		return std::string(s);
	}
};
struct XAIUpdateEvent: public XAIIEvent {
	XAIUpdateEvent(): XAIIEvent() {
		type = XAI_EVENT_UPDATE;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=Update]\n", frame);
		return std::string(s);
	}
};
struct XAIReleaseEvent: public XAIIEvent {
	XAIReleaseEvent(): XAIIEvent() {
		type = XAI_EVENT_RELEASE;
	}
	std::string ToString() const {
		snprintf(s, 511, "[frame=%d][event=Release]\n", frame);
		return std::string(s);
	}
};

#endif
