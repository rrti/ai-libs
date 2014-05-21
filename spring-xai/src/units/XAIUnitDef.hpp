#ifndef XAI_UNITDEF_HDR
#define XAI_UNITDEF_HDR

#include <set>

#include "LegacyCpp/UnitDef.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/MoveTypes/MoveInfo.h"

#include "../main/XAIConstants.hpp"

// note: MOBILE and STATIC are mutually exclusive
// todo: make the MOBILE property its own bitmask?
enum UnitDefTypeMasks {
	MASK_BUILDER_MOBILE    = (1 <<  1), // con-vehicles, -bots, ... 
	MASK_BUILDER_STATIC    = (1 <<  2), // factories, hubs (build BUILDER_STATIC units)
	MASK_ASSISTER_MOBILE   = (1 <<  3), // mobile nanotowers (?)
	MASK_ASSISTER_STATIC   = (1 <<  4), // nanotowers
	MASK_E_PRODUCER_MOBILE = (1 <<  5),
	MASK_E_PRODUCER_STATIC = (1 <<  6), // windmills, solars
	MASK_M_PRODUCER_MOBILE = (1 <<  7),
	MASK_M_PRODUCER_STATIC = (1 <<  8), // extractors, makers
	MASK_E_STORAGE_MOBILE  = (1 <<  9),
	MASK_E_STORAGE_STATIC  = (1 << 10),
	MASK_M_STORAGE_MOBILE  = (1 << 11),
	MASK_M_STORAGE_STATIC  = (1 << 12),
	MASK_DEFENSE_STATIC    = (1 << 13), // turrets, shields, anti-nukes (?)
	MASK_DEFENSE_MOBILE    = (1 << 14), // mobile shields, mobile anti-nukes (?)
	MASK_OFFENSE_STATIC    = (1 << 15), // nukes, superweapons
	MASK_OFFENSE_MOBILE    = (1 << 16), // tanks, warships, ...
	MASK_INTEL_MOBILE      = (1 << 17), // mobile radars, ...
	MASK_INTEL_STATIC      = (1 << 18), // radar towers, ...
//	MASK_KAMIKAZE          = (1 << 19), // mines?
	NUM_TYPE_MASKS         = 18
};

// note: these masks apply to both static and
// mobile units (except MASK_AIR, a building
// needs to be placed on something)
// these are derived from the move{Type, Family}
// and the terrainClass MoveData enum members
enum UnitDefTerrainMasks {
	MASK_LAND             = (1 << 1), // tanks, kbots, hovercraft
	MASK_WATER_SURFACE    = (1 << 2), // ships, hovercraft
	MASK_WATER_SUBMERGED  = (1 << 3), // amphibious tanks and kbots, submarines
	MASK_AIR              = (1 << 4), // aircraft
	NUM_TERRAIN_MASKS     = 4
};

enum UnitDefWeaponMasks {
	MASK_ARMED             = (1 << 1),
	MASK_NUKE              = (1 << 2),
	MASK_ANTINUKE          = (1 << 3),
	MASK_SHIELD            = (1 << 4),
//	MASK_STOCKPILE         = (1 << 5),
//	MASK_MANUALFIRE        = (1 << 6),
	NUM_WEAPON_MASKS       = 4
};

/*
enum UnitDefClassMasks {
	MASK_SCOUT      = (1 << 1),
	MASK_RAIDER     = (1 << 2),
	MASK_ASSAULT    = (1 << 3),
	MASK_ARTILLERY  = (1 << 4),
	MASK_ANTIAIR    = (1 << 5),
	MASK_STRIKER    = (1 << 6), // snipers, etc
	NUM_CLASS_MASKS = 6
};
*/

// these serve as a straight mapping from the MoveData
// move{Type, Family} and terrainClass enum members to
// bitmasks so we can classify a static builder by the
// BW-OR'ed mapped masks of each of its build options
#define MOVEDATA_MT2MASK(mt) (1 << (mt + 0))
#define MOVEDATA_MF2MASK(mf) (1 << (mf + 4))
#define MOVEDATA_TC2MASK(tc) (1 << (tc + 9))
enum UnitDefMoveDataMasks {
	MASK_MOVEDATA_MT_GND = MOVEDATA_MT2MASK(MoveData::Ground_Move), // 1 << 0
	MASK_MOVEDATA_MT_HVR = MOVEDATA_MT2MASK(MoveData::Hover_Move),  // 1 << 1
	MASK_MOVEDATA_MT_SHP = MOVEDATA_MT2MASK(MoveData::Ship_Move),   // 1 << 2
	MASK_MOVEDATA_MT_AIR = MOVEDATA_MT2MASK(3),                     // 1 << 3

	MASK_MOVEDATA_MF_TNK = MOVEDATA_MF2MASK(MoveData::Tank),        // 1 << (0 + 4)
	MASK_MOVEDATA_MF_KBT = MOVEDATA_MF2MASK(MoveData::KBot),        // 1 << (1 + 4)
	MASK_MOVEDATA_MF_HVR = MOVEDATA_MF2MASK(MoveData::Hover),       // 1 << (2 + 4)
	MASK_MOVEDATA_MF_SHP = MOVEDATA_MF2MASK(MoveData::Ship),        // 1 << (3 + 4)
	MASK_MOVEDATA_MF_AIR = MOVEDATA_MF2MASK(8),                     // 1 << (4 + 4)

	MASK_MOVEDATA_TC_LND = MOVEDATA_TC2MASK(MoveData::Land),        // 1 << (0 + 9)
	MASK_MOVEDATA_TC_WTR = MOVEDATA_TC2MASK(MoveData::Water),       // 1 << (1 + 9)
	MASK_MOVEDATA_TC_MXD = MOVEDATA_TC2MASK(MoveData::Mixed),       // 1 << (2 + 9)
	MASK_MOVEDATA_TC_AIR = MOVEDATA_TC2MASK(12),                    // 1 << (3 + 9)

	NUM_MOVEDATA_MASKS = 13
};

// not much point to this, UnitDef is no longer
// a polymorphic type on the engine side (which
// rules out dynamic_cast'ing...)
struct XAIUnitDef /*: public UnitDef*/ {
public:
	XAIUnitDef(): sprUnitDef(0), dgunWeaponDef(0) {
	}

	void SetUnitDef(const UnitDef* def) { sprUnitDef = def; }
	void SetDGunWeaponDef(const UnitDef::UnitDefWeapon* def) { dgunWeaponDef = def; }

	float GetBuildDist() const { return (GetDef()->buildDistance); }
	float GetMaxSpeed() const { return (GetDef()->speed); }
	float GetBuildSpeed() const { return (GetDef()->buildSpeed); }
	float GetReclaimSpeed() const { return (GetDef()->reclaimSpeed); }
	float GetPower() const { return (GetDef()->power); } // hmm

	const char* GetName() const { return (GetDef()->name.c_str()); }

	// ticks: number of CTeam::SlowUpdate() calls that
	// would elapse while building a unit of this type;
	// a CTeam::SlowUpdate() occurs every 32 simulation
	// frames ie. every (32 / GAME_SPEED) seconds
	float GetBuildTimeTicks(float buildSpeed) const { return float(GetDef()->buildTime / buildSpeed); }
	float GetBuildTimeFrames(float buildSpeed) const { return float(GetBuildTimeTicks(buildSpeed) * TEAM_SU_INT_F); }
	float GetBuildTimeSeconds(float buildSpeed) const { return float(GetBuildTimeFrames(buildSpeed) / GAME_SPEED); }

	// per-frame costs of building a unit
	// of this type at a given build-speed
	float FrameCost(char resCode, float buildSpeed) const {
		switch (resCode) {
			case 'M': {
				const float bt = GetBuildTimeFrames(buildSpeed);
				const float mc = GetDef()->metalCost / bt;
				return mc;
			} break;
			case 'E': {
				const float bt = GetBuildTimeFrames(buildSpeed);
				const float ec = GetDef()->energyCost / bt;
				return ec;
			} break;
			default: {
				return 1.0f;
			} break;
		}
	}

	// return the total number of frames that it will
	// take to earn back the {M, E}-cost of this type
	// of unit (given the current net {M, E}-incomes)
	//
	// note: cheaper units will always win (for fixed
	// income values) from more expensive ones
	float GetReturnInvestmentTimeFrames(float mtlGain, float nrgGain) const {
		const float mc = GetDef()->metalCost;
		const float ec = GetDef()->energyCost;
		const float mROI = (mtlGain <= 0.0f && mc > 0.0f)? 1e30f: (mc / mtlGain);
		const float eROI = (nrgGain <= 0.0f && ec > 0.0f)? 1e30f: (ec / nrgGain);
		return ((mROI * TEAM_SU_INT_F) + (eROI * TEAM_SU_INT_F));
	}


	// metalMake: always created regardless of unit state
	// makesMetal: created when unit active and enough E
	// extractRadius: set to mapInfo->map.extractorRadius for ALL extractor UnitDefs
	// extractsMetal: scalar depth for M pulled from ground, varies per extractor UnitDef
	//                is conditional likes makesMetal, only counted when unit is active
	//
	// for metal makers, metalMake is usually zero and
	// the conditional makesMetal value comes into play
	//
	// important: these values are all per TEAM_SU_INT_I frames
	float ResMakeOff(char resCode, float avgWndE, float avgTdlE) const {
		switch (resCode) {
			case 'M': {
				// unconditional, excludes {extracts, makes}Metal
				return (GetDef()->metalMake  - GetDef()->metalUpkeep);
			} break;
			case 'E': {
				const float s = 1.0f;
				const float e = (GetDef()->energyMake - GetDef()->energyUpkeep);

				float wndE = 0.0f;
				float tdlE = 0.0f;

				if (avgWndE > 0.0f || avgTdlE > 0.0f) {
					// note: engine uses scalar of 0.5f since there
					// are two CUnit SlowUpdate()'s for every CTeam
					// SlowUpdate()
					wndE = GetDef()->windGenerator;
					tdlE = GetDef()->tidalGenerator * avgTdlE;

					if ((wndE > 0.0f) && (avgWndE >= wndE * 0.25f)) {
						wndE = ((wndE < avgWndE)? (avgWndE * s): (wndE * s));
					} else {
						wndE = 0.0f;
					}
				}

				return (e + (wndE + tdlE));
			} break;
			default: {
			} break;
		}

		return 0.0f;
	}

	float ResMakeOn(char resCode, float resPosMtlScale) const {
		switch (resCode) {
			case 'M': {
				return (GetDef()->makesMetal + (GetDef()->extractsMetal * resPosMtlScale));
			} break;
			case 'E': {
				// no conditional "makesEnergy" or "extractsEnergy" equivalent
				// (solars produce energy only when not closed, but this does
				// not depend on the current resource state)
			} break;
			default: {
			} break;
		}

		return 0.0f;
	}

	// secondary function; map-independent
	bool IsResGenerator(char resCode) const {
		switch (resCode) {
			case 'M': { return ((GetDef()->extractsMetal > 0.0f) || (GetDef()->makesMetal > 0.0f)); } break;
			case 'E': { return ((GetDef()->windGenerator > 0.0f) || (GetDef()->tidalGenerator > 0.0f)); } break;
			default: {} break;
		}
		return false;
	}



	// closer to 0 means less cost-efficient
	float GetResourceCostRatio(char resCode, float avgWndE, float avgTdlE) const {
		switch (resCode) {
			case 'M': {
				// M-income per 32 frames
				return ((ResMakeOff('M', 0.0f, 0.0f) + ResMakeOn('M', 0.0f)) / (GetDef()->metalCost + 1.0f));
			} break;
			case 'E': {
				// E-income per 32 frames
				return ((ResMakeOff('E', avgWndE, avgTdlE) + ResMakeOn('E', 0.0f)) / (GetDef()->energyCost + 1.0f));
			} break;
		}

		return 0.0f;
	}

	// closer to 0 means less cost-efficient (and
	// any storage with ratio < 1 is not worth it)
	float GetStorageCostRatio(char resCode) const {
		switch (resCode) {
			case 'M': {
				return (GetDef()->metalStorage / (GetDef()->metalCost + 1.0f));
			} break;
			case 'E': {
				return (GetDef()->energyStorage / (GetDef()->energyCost + 1.0f));
			} break;
			default: {
			} break;
		}

		return 0.0f;
	}

	float GetIncomeUsageRatio(char resCode, float /*curGain*/, float curAvgUsage, float avgWndE, float avgTdlE) const {
		float netResInc = 0.0f;

		switch (resCode) {
			case 'M': {
				// pass 0.0f to ResMakeOn(); this is only called
				// when metal extractors are not a feasible option
				netResInc = ResMakeOff('M', 0.0f, 0.0f) + ResMakeOn('M', 0.0f);
			} break;
			case 'E': {
				netResInc = ResMakeOff('E', avgWndE, avgTdlE) + ResMakeOn('E', 0.0f);
			} break;
			default: {
			} break;
		}

		if (netResInc > curAvgUsage) {
			assert(netResInc > 0.0f);

			// def produces more than we need, pick cheapest
			if (curAvgUsage > 0.0f) {
				return (curAvgUsage / netResInc);
			} else {
				return (1.0f / netResInc);
			}
		} else {
			// def produces less than we need, pick best
			if (curAvgUsage > 0.0f) {
				return (netResInc / curAvgUsage);
			} else {
				return netResInc; // note: should be in [0, 1]
			}
		}

		return 0.0f;
	}



	bool HaveBuildOptionDefID(int uDefID) const {
		return (buildOptionUDIDs.find(uDefID) != buildOptionUDIDs.end());
	}

	int GetID() const { return (GetDef()->id); }
	const MoveData* GetMoveData() const { return (GetDef()->movedata); }
	const UnitDef* GetDef() const { return sprUnitDef; }

	const UnitDef::UnitDefWeapon* GetDGunWeaponDef() const {
		if (dgunWeaponDef != 0) {
			return dgunWeaponDef;
		}

		typedef UnitDef::UnitDefWeapon UDefWeap;
		std::vector<UDefWeap>::const_iterator wit;

		for (wit = GetDef()->weapons.begin(); wit != GetDef()->weapons.end(); wit++) {
			if (wit->def->type == "DGun") {
				return &(*wit);
			}
		}

		return 0;
	}


	unsigned int typeMask;
	unsigned int terrainMask;
	unsigned int weaponMask;
	unsigned int classMask;      // general categorization (scout, raider, ...), unused for now
	unsigned int boMoveDataMask; // for build options, used by static builders ONLY

	bool isHubBuilder;
	bool isSpecialBuilder;       // if true, none of our build-options are builders
	bool isMobile;               // true if (non-zero maxSpeed and (non-NULL MoveData or aircraft))
	bool isAttacker;             // true if unit has at least one weapon (normal or special)
	bool isBuilder;              // true if unit has at least one build-option

	float minWeaponRange;        // in world-space
	float maxWeaponRange;        // in world-space

	float nBuildTime;            // normalized build time
	float nMetalCost;            // normalized metal cost (unused)
	float nEnergyCost;           // normalized energy cost (unused)
	float nMaxMoveSpeed;         // normalized max. movement speed
	float nExtractsMetal;        // normalized extraction depth (unused)

	std::set<int> buildOptionUDIDs;

private:
	const UnitDef* sprUnitDef;
	const UnitDef::UnitDefWeapon* dgunWeaponDef;
};

#endif
