#include <sstream>
#include <exception>

#include "LegacyCpp/IGlobalAICallback.h"
#include "LegacyCpp/IAICallback.h"

#include "./XAI.hpp"
#include "./XAICredits.hpp"
#include "./XAIDefines.hpp"
#include "./XAIHelper.hpp"
#include "../utils/XAILogger.hpp"
#include "../utils/XAITimer.hpp"
#include "../commands/XAICommandTracker.hpp"
#include "../events/XAIIEvent.hpp"
#include "../events/XAIEventHandler.hpp"
#include "../units/XAIUnitDefHandler.hpp"

unsigned int XAI::xaiInstances = 0;

XAI::XAI(): xaiInstance(0), xaiHelper(0) {
	xaiHelper = new XAIHelper();
}
XAI::~XAI() {
	delete xaiHelper;
}



void XAI::InitAI(IGlobalAICallback* gcb, int team) {
	xaiInstance = xaiInstances++;
	xaiHelper->Init(gcb);

	LOG_BASIC(
		xaiHelper->logger,
		"[XAI::InitAI]\n"      <<
		"\tteam:             " << team << "\n"
		"\tinstance:         " << xaiInstance << "\n"
		"\tactive instances: " << xaiInstances << "\n"
	);

	std::string initMsg =
		std::string(AI_VERSION) + " initialized succesfully!";
	std::string logMsg =
		"logging events to " + (xaiHelper->logger->GetLogName(NULL)) + "[log].txt";

	xaiHelper->rcb->SendTextMsg(initMsg.c_str(), 0);
	xaiHelper->rcb->SendTextMsg(logMsg.c_str(), 0);

	XAICScopedTimer t("[XAI::InitAI]", xaiHelper->timer);

	XAI_BEG_EXCEPTION
		XAIInitEvent e;
			e.type  = XAI_EVENT_INIT;
			e.frame = 0;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::InitAI]")
}

void XAI::ReleaseAI() {
	XAI_BEG_EXCEPTION
		XAIReleaseEvent e;
			e.type  = XAI_EVENT_RELEASE;
			e.frame = xaiHelper->GetCurrFrame();
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::ReleaseAI]")

	xaiInstances--;
	xaiHelper->Release();
}



void XAI::UnitCreated(int unitID, int builderUnitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::UnitCreated]", xaiHelper->timer);
		XAIUnitCreatedEvent e;
			e.type      = XAI_EVENT_UNIT_CREATED;
			e.frame     = xaiHelper->GetCurrFrame();
			e.unitID    = unitID;
			e.builderID = builderUnitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::UnitCreated]")
}

void XAI::UnitFinished(int unitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::UnitFinished]", xaiHelper->timer);
		XAIUnitFinishedEvent e;
			e.type   = XAI_EVENT_UNIT_FINISHED;
			e.frame  = xaiHelper->GetCurrFrame();
			e.unitID = unitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::UnitFinished]")
}

void XAI::UnitDestroyed(int unitID, int attackerUnitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::UnitDestroyed]", xaiHelper->timer);
		XAIUnitDestroyedEvent e;
			e.type       = XAI_EVENT_UNIT_DESTROYED;
			e.frame      = xaiHelper->GetCurrFrame();
			e.unitID     = unitID;
			e.attackerID = attackerUnitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::UnitDestroyed]")
}

void XAI::UnitIdle(int unitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::UnitIdle]", xaiHelper->timer);
		XAIUnitIdleEvent e;
			e.type   = XAI_EVENT_UNIT_IDLE;
			e.frame  = xaiHelper->GetCurrFrame();
			e.unitID = unitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::UnitIdle]")
}

void XAI::UnitDamaged(int damagedUnitID, int attackerUnitID, float damage, float3 dir) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::UnitDamaged]", xaiHelper->timer);
		XAIUnitDamagedEvent e;
			e.type         = XAI_EVENT_UNIT_DAMAGED;
			e.frame        = xaiHelper->GetCurrFrame();
			e.unitID       = damagedUnitID;
			e.attackerID   = attackerUnitID;
			e.attackDamage = damage;
			e.attackDir    = dir;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::UnitDamaged]")
}

void XAI::EnemyDamaged(int damagedUnitID, int attackerUnitID, float damage, float3 dir) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::EnemyDamaged]", xaiHelper->timer);
		XAIEnemyDamagedEvent e;
			e.type         = XAI_EVENT_ENEMY_DAMAGED;
			e.frame        = xaiHelper->GetCurrFrame();
			e.unitID       = damagedUnitID;
			e.attackerID   = attackerUnitID;
			e.attackDamage = damage;
			e.attackDir    = dir;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::EnemyDamaged]")
}

void XAI::UnitMoveFailed(int unitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::UnitMoveFailed]", xaiHelper->timer);
		XAIUnitMoveFailedEvent e;
			e.type   = XAI_EVENT_UNIT_MOVE_FAILED;
			e.frame  = xaiHelper->GetCurrFrame();
			e.unitID = unitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::UnitMoveFailed]")
}


void XAI::EnemyEnterLOS(int enemyUnitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::EnemyEnterLOS]", xaiHelper->timer);
		XAIEnemyEnterLOSEvent e;
			e.type   = XAI_EVENT_ENEMY_ENTER_LOS;
			e.frame  = xaiHelper->GetCurrFrame();
			e.unitID = enemyUnitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::EnemyEnterLOS]")
}

void XAI::EnemyLeaveLOS(int enemyUnitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::EnemyLeaveLOS]", xaiHelper->timer);
		XAIEnemyLeaveLOSEvent e;
			e.type   = XAI_EVENT_ENEMY_LEAVE_LOS;
			e.frame  = xaiHelper->GetCurrFrame();
			e.unitID = enemyUnitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::EnemyLeaveLOS]")
}

void XAI::EnemyEnterRadar(int enemyUnitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::EnemyEnterRadar]", xaiHelper->timer);
		XAIEnemyEnterRadarEvent e;
			e.type   = XAI_EVENT_ENEMY_ENTER_RADAR;
			e.frame  = xaiHelper->GetCurrFrame();
			e.unitID = enemyUnitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::EnemyEnterRadar]")
}

void XAI::EnemyLeaveRadar(int enemyUnitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::EnemyLeaveRadar]", xaiHelper->timer);
		XAIEnemyLeaveRadarEvent e;
			e.type   = XAI_EVENT_ENEMY_LEAVE_RADAR;
			e.frame  = xaiHelper->GetCurrFrame();
			e.unitID = enemyUnitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::EnemyLeaveRadar]")
}

void XAI::EnemyDestroyed(int enemyUnitID, int attackerUnitID) {
	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::EnemyDestroyed]", xaiHelper->timer);
		XAIEnemyDestroyedEvent e;
			e.type       = XAI_EVENT_ENEMY_DESTROYED;
			e.frame      = xaiHelper->GetCurrFrame();
			e.unitID     = enemyUnitID;
			e.attackerID = attackerUnitID;
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::EnemyDestroyed]")
}


void XAI::GotChatMessage(const char* msg, int playerNum) {
}
void XAI::GotLuaMessage(const char* inData, const char** outData) {
}

int XAI::HandleEvent(int msgID, const void* msgData) {
	switch (msgID) {
		case AI_EVENT_UNITGIVEN: {
			const ChangeTeamEvent* cte = (const ChangeTeamEvent*) msgData;

			XAI_BEG_EXCEPTION
				XAICScopedTimer t("[XAI::HandleEvent::UnitGiven]", xaiHelper->timer);
				XAIUnitGivenEvent e;
					e.type        = XAI_EVENT_UNIT_GIVEN;
					e.frame       = xaiHelper->GetCurrFrame();
					e.unitID      = cte->unit;
					e.newUnitTeam = cte->newteam;
				xaiHelper->eventHandler->NotifyReceivers(&e);
			XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::HandleEvent::UnitGiven]")
		} break;
		case AI_EVENT_UNITCAPTURED: {
			const ChangeTeamEvent* cte = (const ChangeTeamEvent*) msgData;

			XAI_BEG_EXCEPTION
				XAICScopedTimer t("[XAI::HandleEvent::UnitCaptured]", xaiHelper->timer);
				XAIUnitCapturedEvent e;
					e.type        = XAI_EVENT_UNIT_CAPTURED;
					e.frame       = xaiHelper->GetCurrFrame();
					e.unitID      = cte->unit;
					e.oldUnitTeam = cte->oldteam;
				xaiHelper->eventHandler->NotifyReceivers(&e);
			XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::HandleEvent::UnitCaptured]")
		} break;
	}

	return 0;
}


void XAI::Update() {
	// first Update event is at frame 1, not 0
	xaiHelper->SetCurrFrame(xaiHelper->GetCurrFrame() + 1);

	XAI_BEG_EXCEPTION
		XAICScopedTimer t("[XAI::Update]", xaiHelper->timer);
		XAIUpdateEvent e;
			e.type  = XAI_EVENT_UPDATE;
			e.frame = xaiHelper->GetCurrFrame();
		xaiHelper->eventHandler->NotifyReceivers(&e);
	XAI_END_EXCEPTION(xaiHelper->logger, "[XAI::Update]")
}
