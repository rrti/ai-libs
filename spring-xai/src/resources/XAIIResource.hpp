#ifndef XAI_IRESOURCE_HDR
#define XAI_IRESOURCE_HDR

#include <cstdio>
#include <string>
#include "System/float3.h"
#include "../main/XAIConstants.hpp"

struct UnitDef;
struct FeatureDef;

struct XAIResourceState {
public:
	XAIResourceState() {
		curIncome  = curUsage   = 0.0f;
		curLevel   = curStorage = 0.0f;
		cumIncome  = avgIncome  = 0.0f;
		cumUsage   = avgUsage   = 0.0f;
		initIncome = initUsage  = 0.0f;
		curGain                 = 0.0f;
		curLevelDelta           = 0.0f;
		stallTime               = 1e30f;
		numSlowUpdates          = 0;

		excess = false;
		stall  = false;
		need   = false;

		wantStorage  = false;
		wantProducer = false;
	}

	void SetIncome(float f)     { curIncome     = f; }
	void SetUsage(float f)      { curUsage      = f; }
	void SetLevel(float f)      { curLevel      = f; }
	void SetStorage(float f)    { curStorage    = f; }
	void SetCumIncome(float f)  { cumIncome     = f; }
	void SetCumUsage(float f)   { cumUsage      = f; }
	void SetAvgIncome(float f)  { avgIncome     = f; }
	void SetAvgUsage(float f)   { avgUsage      = f; }
	void SetInitIncome(float f) { initIncome    = f; }
	void SetInitUsage(float f)  { initUsage     = f; }
	void SetGain(float f)       { curGain       = f; }
	void SetLevelDelta(float f) { curLevelDelta = f; }

	float GetIncome() const     { return curIncome;     }
	float GetUsage() const      { return curUsage;      }
	float GetLevel() const      { return curLevel;      }
	float GetStorage() const    { return curStorage;    }
	float GetCumIncome() const  { return cumIncome;     }
	float GetCumUsage() const   { return cumUsage;      }
	float GetAvgIncome() const  { return avgIncome;     }
	float GetAvgUsage() const   { return avgUsage;      }
	float GetInitIncome() const { return initIncome;    }
	float GetInitUsage() const  { return initUsage;     }
	float GetGain() const       { return curGain;       }
	float GetLevelDelta() const { return curLevelDelta; }
	float GetStallTime() const  { return stallTime;     }

	bool HaveExcess() const { return excess; }
	bool HaveStall() const { return stall; }
	bool HaveNeed() const { return need; }
	bool WantStorage() const { return wantStorage; }
	bool WantProducer() const { return wantProducer; }

	void Update() {
		// note: curLevelDelta can periodically become less than zero even
		// when excessing, because the engine allows levels to temporarily
		// exceed storage capacity (so that its next act of throwing away
		// the excess looks like a drain)
		excess = ((curLevel >= (curStorage * 0.95f) && (curGain >  initIncome * 2.0f))      || (curStorage <=     0.0f));
		stall  = ((curLevel <= (curStorage * 0.05f) && (curGain <=              0.0f))); // || (curUsage   >  curLevel));

		need = ((curLevelDelta < 0.0f || curGain < 0.0f) && !(curLevel >= (curStorage * 0.95f)));

		if (curGain < 0.0f) {
			// note: curGain values are only updated every 32 frames
			stallTime = curLevel / std::fabs(curGain / TEAM_SU_INT_F);
		} else {
			stallTime = stall? 0.0f: 1e30f;
		}

		// RHS of wantStorage is almost always true, not useful?
		wantStorage  = ((curIncome >= (curStorage * 0.5f)) ||  excess                 );
		wantProducer = ((curLevel  <  (curStorage * 0.5f)) || (curIncome < curStorage));
	}

	void SlowUpdate() {
		numSlowUpdates += 1;

		cumIncome += curIncome;
		cumUsage  += curUsage;
		avgIncome  = cumIncome / numSlowUpdates;
		avgUsage   = cumUsage  / numSlowUpdates;
		curGain    = curIncome - curUsage;
	}

private:
	float curIncome,  curUsage;     // income, usage (per 32 frames; can never be less than 0.0f)
	float curLevel,   curStorage;   // level, storage (per  1 frame;  can never be less than 0.0f)
	float cumIncome,  avgIncome;    // cumulative and average income (per 32 frames)
	float cumUsage,   avgUsage;     // cumulative and average usage (per 32 frames)
	float initIncome, initUsage;    // initial income and usage
	float curGain;                  // equal to curIncome - curUsage (per 32 frames)
	float curLevelDelta;            // curLevel(t) - curLevel(t - 1)
	float stallTime;                // predicted time until stall in frames

	bool excess, stall, need;
	bool wantStorage, wantProducer;

	unsigned int numSlowUpdates;
};



enum XAIResourceTypeMasks {
	XAI_RESOURCETYPE_BASE   = (1 << 0),
	XAI_RESOURCETYPE_METAL  = (1 << 1),
	XAI_RESOURCETYPE_ENERGY = (1 << 2),
};

struct XAIIResource {
public:
	virtual ~XAIIResource() {}
	virtual std::string ToString() const {
		snprintf(s, 511, "pos: <%f, %f, %f>\n", pos.x, pos.y, pos.z);
		return std::string(s);
	}

	// can be a combined type (M & E)
	unsigned int typeMask;
	// y < 0.0f means submerged
	float3 pos;

private:
	mutable char s[512];
};


// these do not disappear when claimed
struct XAIExtractableResource: public XAIIResource {
public:
	float rExtractionValue; // raw "worth"
	float nExtractionValue; // normalized "worth"
};
// these disappear when claimed (one-time resources), except geos
struct XAIReclaimableResource: public XAIIResource {
public:
	XAIReclaimableResource(): uDef(0), fDef(0) {
	}

	const UnitDef* uDef;    // neutrals
	const FeatureDef* fDef; // rocks, trees, wrecks, geos
};

#endif
