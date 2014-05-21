#ifndef XAI_THREATMAP_HDR
#define XAI_THREATMAP_HDR

#include <map>
#include <vector>

#include "System/float3.h"
#include "./XAIMap.hpp"
#include "../events/XAIIEventReceiver.hpp"

class float3;
struct XAIIEvent;
struct XAIHelper;
struct XAIThreatMap: public XAIMap<float>, public XAIIEventReceiver {
public:
	XAIThreatMap(XAIHelper*);

	void OnEvent(const XAIIEvent*);
	void AddThreat(int, int, int, float);
	void AddThreatExt(const float3&, float, float);

	float GetThreat(const float3&) const;
	int GetNumEnemies() const { return numUnitIDs; }
	int GetEnemyID(int i) const { return unitIDs[i]; }

private:
	void Init();
	void FastUpdate();
	void Update();

	int numUnitIDs;              // number of enemy units present
	int numUnitDefIDs;           // number of unique UnitDef types
	std::vector<int> unitIDs;    // IDs of enemy units filled via GetEnemyUnits
	std::vector<int> unitDefIDs; // unit counts per unique UnitDefID

	struct EnemyUnit {
		EnemyUnit(): unitID(-1), unitDefID(-1), pwr(0.0f), pos(ZeroVector) {}
		EnemyUnit(int uID, int defID, const float3& p): unitID(uID), unitDefID(defID), pwr(0.0f), pos(p) {}

		bool operator < (const EnemyUnit& u) const {
			return (unitID < u.unitID);
		}

		int unitID;
		int unitDefID;

		float pwr;      // last-frame health / maxHealth
		float3 pos;     // last-frame position
	};
	std::map<int, EnemyUnit> enemyUnits;

	struct ThreatCell {
		ThreatCell(): N(0) {
		}

		int GetUnitCount() const { return N; }
		int GetUnitCount(int unitDefID) const {
			const std::map<int, int>::const_iterator it = M.find(unitDefID);

			if (it != M.end()) {
				return it->second;
			} else {
				return 0;
			}
		}

		int N;
		std::map<int, int> M;
	};
	std::vector<ThreatCell> threatCells;

	float avgThreat;
	float maxThreat;
	float sumThreat;

	XAIHelper* xaih;
};

#endif
