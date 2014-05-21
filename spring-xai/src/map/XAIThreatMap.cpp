#include <sstream>
#include <string>
#include <cassert>

#include <list>
#include <set>

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/IAICheats.h"
#include "LegacyCpp/UnitDef.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/float3.h"

#include "./XAIThreatMap.hpp"
#include "../events/XAIIEvent.hpp"
#include "../main/XAIHelper.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../utils/XAITimer.hpp"
#include "../utils/XAIUtil.hpp"

#define LUA_THREATMAP_DEBUG 1

XAIThreatMap::XAIThreatMap(XAIHelper* h):
XAIMap<float>(HEIGHT2THREAT(h->rcb->GetMapWidth()), HEIGHT2THREAT(h->rcb->GetMapHeight()), 0.0f, XAI_THREAT_MAP) {
	xaih = h;

	numUnitIDs    = 0;
	numUnitDefIDs = 0;

	avgThreat = 0.0f;
	maxThreat = 0.0f;
	sumThreat = 0.0f;
}

void XAIThreatMap::OnEvent(const XAIIEvent* e) {
	switch (e->type) {
		case XAI_EVENT_UNIT_CREATED: {} break;
		case XAI_EVENT_UNIT_FINISHED: {} break;
		case XAI_EVENT_UNIT_DESTROYED: {} break;
		case XAI_EVENT_UNIT_DAMAGED: {} break;
		case XAI_EVENT_UNIT_GIVEN: {} break;
		case XAI_EVENT_UNIT_CAPTURED: {} break;

		case XAI_EVENT_INIT: {
			Init();
		} break;
		case XAI_EVENT_UPDATE: {
			Update();
		} break;
		case XAI_EVENT_RELEASE: {
			unitIDs.clear();
			unitDefIDs.clear();
		} break;

		default: {
		} break;
	}
}

void XAIThreatMap::Init() {
	unitIDs.resize(MAX_UNITS);
	unitDefIDs.resize(xaih->rcb->GetNumUnitDefs() + 1, 0);
	threatCells.resize(mapx * mapy, ThreatCell());

	#if (LUA_THREATMAP_DEBUG == 1)
	std::stringstream luaDataStream;
		luaDataStream << "GG.AIThreatMap[\"threatMapSizeX\"] = " << mapx << ";\n";
		luaDataStream << "GG.AIThreatMap[\"threatMapSizeZ\"] = " << mapy << ";\n";
		luaDataStream << "GG.AIThreatMap[\"threatMapResX\"]  = " << (1 << THREATMAP_RESOLUTION) << ";\n";
		luaDataStream << "GG.AIThreatMap[\"threatMapResZ\"]  = " << (1 << THREATMAP_RESOLUTION) << ";\n";
		luaDataStream << "\n";
		luaDataStream << "local threatMapSizeX = GG.AIThreatMap[\"threatMapSizeX\"];\n";
		luaDataStream << "local threatMapSizeZ = GG.AIThreatMap[\"threatMapSizeZ\"];\n";
		luaDataStream << "local threatMapArray = GG.AIThreatMap;\n";
		luaDataStream << "\n";
		luaDataStream << "for row = 0, (threatMapSizeZ - 1) do\n";
		luaDataStream << "\tfor col = 0, (threatMapSizeX - 1) do\n";
		luaDataStream << "\t\tthreatMapArray[row * threatMapSizeX + col] = 0.0;\n";
		luaDataStream << "\tend\n";
		luaDataStream << "end\n";
	std::string luaDataStr = luaDataStream.str();

	xaih->rcb->CallLuaRules("[AI::XAI::ThreatMap::Init]", -1, NULL);
	xaih->rcb->CallLuaRules(luaDataStr.c_str(), -1, NULL);
	#endif
}






/*
void XAIThreatMap::FastUpdate() {
	XAICScopedTimer t("[XAIThreatMap::FastUpdate]", xaih->timer);

	avgThreat = 0.0f;
	maxThreat = 0.0f;
	sumThreat = 0.0f;

	numUnitIDs = xaih->ccb->GetEnemyUnits(&unitIDs[0], MAX_UNITS);

	std::list<int> unitsCreated;
	std::list<int> unitsDestroyed;
	std::set<int> units;

	for (int i = 0; i < numUnitIDs; i++) {
		const int      unitID  = unitIDs[i];
		const UnitDef* unitDef = xaih->ccb->GetUnitDef(unitID);

		assert(unitDef != NULL);
		// copy each unitID
		units.insert(unitID);

		if (enemyUnits.find(unitID) == enemyUnits.end()) {
			unitsCreated.push_back(unitID);

			if (unitDefIDs[unitDef->id] == 0) {
				numUnitDefIDs += 1;
			}

			unitDefIDs[unitDef->id] += 1;
		}
	}

	for (std::map<int, EnemyUnit>::const_iterator it = enemyUnits.begin(); it != enemyUnits.end(); it++) {
		if (units.find(it->first) == units.end()) {
			unitsDestroyed.push_back(it->first);

			const EnemyUnit& u = it->second;

			assert(unitDefIDs[u.unitDefID] > 0);
			unitDefIDs[u.unitDefID] -= 1;

			if (unitDefIDs[u.unitDefID] == 0) {
				numUnitDefIDs -= 1;
			}
		}
	}



	for (std::list<int>::const_iterator it = unitsCreated.begin(); it != unitsCreated.end(); it++) {
		enemyUnits[*it] = EnemyUnit(*it, (xaih->ccb->GetUnitDef(*it))->id, xaih->ccb->GetUnitPos(*it));
	}

	for (std::list<int>::const_iterator it = unitsDestroyed.begin(); it != unitsDestroyed.end(); it++) {
		const EnemyUnit& u = enemyUnits[*it];
		const XAIUnitDef* uDef = xaih->unitDefHandler->GetUnitDefByID(u.unitDefID);

		AddThreatX(u.pos, uDef->maxWeaponRange, -(uDef->GetPower() * u.pwr));
		enemyUnits.erase(*it);
	}


	for (std::map<int, EnemyUnit>::iterator it = enemyUnits.begin(); it != enemyUnits.end(); it++) {
		EnemyUnit& u = it->second;

		const XAIUnitDef* uDef = xaih->unitDefHandler->GetUnitDefByID(u.unitDefID);

		if (uDef->GetDef()->weapons.empty())
			continue;
		if (uDef->maxWeaponRange <= 0.0f)
			continue;

		// remove the unit's old threat-value
		AddThreatX(u.pos, uDef->maxWeaponRange, -(uDef->GetPower() * u.pwr));

		u.pwr = xaih->ccb->GetUnitHealth(u.unitID) / xaih->ccb->GetUnitMaxHealth(u.unitID);
		u.pos = xaih->ccb->GetUnitPos(u.unitID);

		// add the unit's new threat-value
		AddThreatX(u.pos, uDef->maxWeaponRange, +(uDef->GetPower() * u.pwr));
	}
}
*/






void XAIThreatMap::Update() {
	XAICScopedTimer t("[XAIThreatMap::Update]", xaih->timer);

	avgThreat = 0.0f;
	maxThreat = 0.0f;
	sumThreat = 0.0f;

	numUnitIDs    = xaih->ccb->GetEnemyUnits(&unitIDs[0], MAX_UNITS);
	numUnitDefIDs = 0;


	for (int defID = 1; defID <= xaih->rcb->GetNumUnitDefs(); defID++) {
		unitDefIDs[defID] = 0;
	}

	// reset all values (expensive)
	for (int tIdx = (mapx * mapy) - 1; tIdx >= 0; tIdx--) {
		pixels[tIdx].SetValue(0.0f);

		threatCells[tIdx].N = 0;
		threatCells[tIdx].M.clear();
	}

	for (int i = 0; i < numUnitIDs; i++) {
		const int      unitID  = unitIDs[i];
		const UnitDef* unitDef = xaih->ccb->GetUnitDef(unitID);

		if (unitDef == NULL)
			continue;

		if (unitDefIDs[unitDef->id] == 0) {
			numUnitDefIDs += 1;
		}

		unitDefIDs[unitDef->id] += 1;

		if (unitDef->weapons.empty()) {
			continue;
		}

		const XAIUnitDef* xaiUnitDef = xaih->unitDefHandler->GetUnitDefByID(unitDef->id);

		const float3& unitPos    = xaih->ccb->GetUnitPos(unitID);
		const float   unitHealth = xaih->ccb->GetUnitHealth(unitID) / xaih->ccb->GetUnitMaxHealth(unitID);
		const float   unitPower  = xaiUnitDef->GetPower() * unitHealth;
		const float   unitRange  = xaiUnitDef->maxWeaponRange * 1.25f;

		if (xaiUnitDef->maxWeaponRange > 0.0f) {
			const int tx = HEIGHT2THREAT(WORLD2HEIGHT(int(unitPos.x)));
			const int tz = HEIGHT2THREAT(WORLD2HEIGHT(int(unitPos.z)));
			const int tr = HEIGHT2THREAT(WORLD2HEIGHT(int(unitRange)));

			// track per-cell statistics for enemies
			threatCells[tz * mapx + tx].N              += 1;
			threatCells[tz * mapx + tx].M[unitDef->id] += 1;

			AddThreat(tx, tz, tr, unitPower);
		}

		sumThreat += unitPower;
		maxThreat  = std::max(maxThreat, unitPower);
	}

	avgThreat = sumThreat / (mapx * mapy);


	#if (LUA_THREATMAP_DEBUG == 1)
	if ((xaih->GetCurrFrame() % 15) == 0) {
		std::string luaDataStr;
		std::stringstream luaDataStream;
			luaDataStream << "local threatMapArray = GG.AIThreatMap;\n";

		// normalize the values for visualization
		for (int tIdx = (mapx * mapy) - 1; tIdx >= 0; tIdx--) {
			luaDataStream << "threatMapArray[" << tIdx << "]";
			luaDataStream << " = ";
			luaDataStream << (pixels[tIdx].GetValue() / maxThreat);
			luaDataStream << ";\n";
		}

		luaDataStr = luaDataStream.str();

		xaih->rcb->CallLuaRules("[AI::XAI::ThreatMap::Update]", -1, NULL);
		xaih->rcb->CallLuaRules(luaDataStr.c_str(), -1, NULL);
	}
	#endif
}



void XAIThreatMap::AddThreatExt(const float3& p, float r, float v) {
	// position is in world-coordinates; first
	// convert it to heightmap-space and then
	// to threatmap-space
	const int tx = HEIGHT2THREAT(WORLD2HEIGHT(int(p.x)));
	const int tz = HEIGHT2THREAT(WORLD2HEIGHT(int(p.z)));
	const int tr = HEIGHT2THREAT(WORLD2HEIGHT(int(r)));

	AddThreat(tx, tz, tr, v);
}

void XAIThreatMap::AddThreat(int tx, int tz, int tr, float v) {
	for (int i = -tr; i <= tr; i++) {
		for (int j = -tr; j <= tr; j++) {
			if ((tx + i) <     0) { continue; }
			if ((tx + i) >= mapx) { continue; }
			if ((tz + j) <     0) { continue; }
			if ((tz + j) >= mapy) { continue; }

			if (((i * i) + (j * j)) > (tr * tr))
				continue;

			const int   k = (tz + j) * mapx + (tx + i);
			const float w = pixels[k].GetValue();
			const float d = std::max(w + v, 0.0f);

			// d * XAIUtil::GaussDens(i + j, 0.0f, 1.0f);

			pixels[k].SetValue(d);
		}
	}
}

float XAIThreatMap::GetThreat(const float3& p) const {
	const int tx = HEIGHT2THREAT(WORLD2HEIGHT(int(p.x)));
	const int tz = HEIGHT2THREAT(WORLD2HEIGHT(int(p.z)));

	if (tx < 0 || tx >= mapx) { return 0.0f; }
	if (tz < 0 || tz >= mapy) { return 0.0f; }

	static const int kernelSize = 3;
	static const int kernelWidth = kernelSize >> 1;
	static const float kernelWeights[kernelSize][kernelSize] = {
		{1.0f, 2.0f, 1.0f},
		{2.0f, 5.0f, 2.0f},
		{1.0f, 2.0f, 1.0f}
	};

	float threat = 0.0f; // pixels[tz * mapx + tx].GetValue();

	for (int i = -kernelWidth; i <= kernelWidth; i++) {
		for (int j = -kernelWidth; j <= kernelWidth; j++) {
			if ((tx + i) <     0) { continue; }
			if ((tx + i) >= mapx) { continue; }
			if ((tz + j) <     0) { continue; }
			if ((tz + j) >= mapy) { continue; }

			const float t = pixels[(tz + j) * mapx + (tx + i)].GetValue();
			const float w = kernelWeights[i + kernelWidth][j + kernelWidth];

			threat += (t * w);
		}
	}

	return threat;
}
