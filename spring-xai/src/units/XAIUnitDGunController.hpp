#ifndef XAI_DGUNCONTROLLER_HDR
#define XAI_DGUNCONTROLLER_HDR

#include <vector>

#include "Sim/Misc/GlobalConstants.h"
#include "System/float3.h"

class IAICheats;
struct WeaponDef;
struct XAIHelper;

struct ControllerState {
public:
	ControllerState(void) {
		targetID             = -1;
		dgunOrderFrame       =  0;
		reclaimOrderFrame    =  0;
		captureOrderFrame    =  0;
		targetSelectionFrame =  0;
	}

	void Reset(unsigned int currentFrame, bool clearNow) {
		if (clearNow) {
			dgunOrderFrame    =  0;
			reclaimOrderFrame =  0;
			captureOrderFrame =  0;
			targetID          = -1;
		} else {
			if (dgunOrderFrame > 0 && (currentFrame - dgunOrderFrame) > (GAME_SPEED >> 0)) {
				// one second since last dgun order, mark as expired
				dgunOrderFrame =  0;
				targetID       = -1;
			}
			if (reclaimOrderFrame > 0 && (currentFrame - reclaimOrderFrame) > (GAME_SPEED << 2)) {
				// four seconds since last reclaim order, mark as expired
				reclaimOrderFrame =  0;
				targetID          = -1;
			}
			if (captureOrderFrame > 0 && (currentFrame - captureOrderFrame) > (GAME_SPEED << 3)) {
				// eight seconds since last capture order, mark as expired
				captureOrderFrame =  0;
				targetID          = -1;
			}
		}
	}

	int    dgunOrderFrame;
	int    reclaimOrderFrame;
	int    captureOrderFrame;
	int    targetSelectionFrame;
	int    targetID;
	float3 oldTargetPos;
};

class XAICUnitDGunController {
public:
	XAICUnitDGunController(XAIHelper*, int, const WeaponDef*);

	void EnemyDestroyed(int, int);
	void Update();
	void Stop() const;
	bool IsBusy() const;

private:
	void TrackAttackTarget(unsigned int);
	void SelectTarget(unsigned int);

	void IssueOrder(int, int, int) const;
	void IssueOrder(const float3&, int, int) const;

	const int        ownerID;
	const WeaponDef* ownerWD;

	std::vector<int> enemyUnitsByID;
	ControllerState state;

	XAIHelper* xaih;
};

#endif
