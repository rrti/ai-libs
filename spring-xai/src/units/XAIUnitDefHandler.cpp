#include <sstream>
#include <cassert>

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/UnitDef.h"
#include "Sim/MoveTypes/MoveInfo.h"

#include "./XAIUnitDefHandler.hpp"
#include "./XAIUnitDef.hpp"
#include "../main/XAIHelper.hpp"
#include "../utils/XAIUtil.hpp"
#include "../utils/XAILogger.hpp"

XAICUnitDefHandler::XAICUnitDefHandler(XAIHelper* h): xaih(h) {
	unitDefIDSets.push_back(&mobileBuilderUnitDefIDs);
	unitDefIDSets.push_back(&staticBuilderUnitDefIDs);
	unitDefIDSets.push_back(&mobileAssisterUnitDefIDs);
	unitDefIDSets.push_back(&staticAssisterUnitDefIDs);
	unitDefIDSets.push_back(&mobileEProducerUnitDefIDs);
	unitDefIDSets.push_back(&staticEProducerUnitDefIDs);
	unitDefIDSets.push_back(&mobileMProducerUnitDefIDs);
	unitDefIDSets.push_back(&staticMProducerUnitDefIDs);
	unitDefIDSets.push_back(&mobileEStorageUnitDefIDs);
	unitDefIDSets.push_back(&staticEStorageUnitDefIDs);
	unitDefIDSets.push_back(&mobileMStorageUnitDefIDs);
	unitDefIDSets.push_back(&staticMStorageUnitDefIDs);
	unitDefIDSets.push_back(&staticDefenseUnitDefIDs);
	unitDefIDSets.push_back(&mobileDefenseUnitDefIDs);
	unitDefIDSets.push_back(&staticOffenseUnitDefIDs);
	unitDefIDSets.push_back(&mobileOffenseUnitDefIDs);
	unitDefIDSets.push_back(&mobileIntelUnitDefIDs);
	unitDefIDSets.push_back(&staticIntelUnitDefIDs);

	unitDefIDSets.push_back(&landUnitDefIDs);
	unitDefIDSets.push_back(&surfaceWaterUnitDefIDs);
	unitDefIDSets.push_back(&submergedWaterUnitDefIDs);
	unitDefIDSets.push_back(&airUnitDefIDs);

	unitDefIDSets.push_back(&armedUnitDefIDs);
	unitDefIDSets.push_back(&nukeUnitDefIDs);
	unitDefIDSets.push_back(&antiNukeUnitDefIDs);
	unitDefIDSets.push_back(&shieldUnitDefIDs);
	// unitDefIDSets.push_back(&stockpileUnitDefIDs);
	// unitDefIDSets.push_back(&manualFireUnitDefIDs);


	sprUnitDefsByID.resize(xaih->rcb->GetNumUnitDefs() + 1, 0);
	xaiUnitDefsByID.resize(xaih->rcb->GetNumUnitDefs() + 1, 0);

	xaih->rcb->GetUnitDefList(&sprUnitDefsByID[1]);

	float maxBuildTime     = 0.0f;
	float maxMetalCost     = 0.0f;
	float maxEnergyCost    = 0.0f;
	float maxMaxMoveSpeed  = 0.0f;
	float maxExtractsMetal = 0.0f;

	// first pass: categorization
	for (int id = 1; id <= xaih->rcb->GetNumUnitDefs(); id++) {
		if (sprUnitDefsByID[id] == NULL) {
			continue;
		}

		xaiUnitDefsByID[id] = new XAIUnitDef();
		// assert(sprUnitDefsByID[id] != NULL);
		// assert(xaiUnitDefsByID[id] != NULL);

		CategorizeUnitDefByID(id);
		InsertUnitDefByID(id);

		maxBuildTime     = std::max(sprUnitDefsByID[id]->buildTime,     maxBuildTime);
		maxMetalCost     = std::max(sprUnitDefsByID[id]->metalCost,     maxMetalCost);
		maxEnergyCost    = std::max(sprUnitDefsByID[id]->energyCost,    maxEnergyCost);
		maxMaxMoveSpeed  = std::max(sprUnitDefsByID[id]->speed,         maxMaxMoveSpeed);
		maxExtractsMetal = std::max(sprUnitDefsByID[id]->extractsMetal, maxExtractsMetal);
	}

	if (maxBuildTime     == 0.0f) { maxBuildTime     = 1.0f; }
	if (maxMetalCost     == 0.0f) { maxMetalCost     = 1.0f; }
	if (maxEnergyCost    == 0.0f) { maxEnergyCost    = 1.0f; }
	if (maxMaxMoveSpeed  == 0.0f) { maxMaxMoveSpeed  = 1.0f; }
	if (maxExtractsMetal == 0.0f) { maxExtractsMetal = 1.0f; }


	// second pass: normalization (TODO)
	for (int id = 1; id <= xaih->rcb->GetNumUnitDefs(); id++) {
		if (sprUnitDefsByID[id] == NULL) {
			continue;
		}

		XAIUnitDef* xaiUnitDef = const_cast<XAIUnitDef*>(xaiUnitDefsByID[id]);
			xaiUnitDef->nBuildTime     = xaiUnitDef->GetDef()->buildTime     / maxBuildTime;
			xaiUnitDef->nMetalCost     = xaiUnitDef->GetDef()->metalCost     / maxMetalCost;
			xaiUnitDef->nEnergyCost    = xaiUnitDef->GetDef()->energyCost    / maxEnergyCost;
			xaiUnitDef->nMaxMoveSpeed  = xaiUnitDef->GetDef()->speed         / maxMaxMoveSpeed;
			xaiUnitDef->nExtractsMetal = xaiUnitDef->GetDef()->extractsMetal / maxExtractsMetal;

		std::set<int>& boUDIDs = xaiUnitDef->buildOptionUDIDs;
		std::set<int>::const_iterator boUDIDsIt;

		bool isStaticBuilder  = ((xaiUnitDef->typeMask & MASK_BUILDER_STATIC) != 0);
		bool isMobileBuilder  = ((xaiUnitDef->typeMask & MASK_BUILDER_MOBILE) != 0);
		bool isSpecialBuilder = (isStaticBuilder || isMobileBuilder);

		if (isStaticBuilder || isMobileBuilder) {
			assert(!boUDIDs.empty());
		}

		// mark all static builder units that are hubs; also
		// classify them by each build option's move-class
		for (boUDIDsIt = boUDIDs.begin(); boUDIDsIt != boUDIDs.end(); boUDIDsIt++) {
			const XAIUnitDef* xaiBuildOptionDef = xaiUnitDefsByID[*boUDIDsIt];
			const UnitDef*    sprBuildOptionDef = sprUnitDefsByID[*boUDIDsIt];

			// NOTE: the engine always sets hovercraft to
			// the "Land" terrain-class only, but it still
			// gives us both the MT_HVR and MF_HVR flags
			if (isStaticBuilder) {
				if (sprBuildOptionDef->movedata != NULL) {
					xaiUnitDef->boMoveDataMask |= MOVEDATA_MT2MASK(sprBuildOptionDef->movedata->moveType);
					xaiUnitDef->boMoveDataMask |= MOVEDATA_MF2MASK(sprBuildOptionDef->movedata->moveFamily);
					xaiUnitDef->boMoveDataMask |= MOVEDATA_TC2MASK(sprBuildOptionDef->movedata->terrainClass);
				} else if (xaiBuildOptionDef->isMobile) {
					xaiUnitDef->boMoveDataMask |= MASK_MOVEDATA_MT_AIR;
					xaiUnitDef->boMoveDataMask |= MASK_MOVEDATA_MF_AIR;
					xaiUnitDef->boMoveDataMask |= MASK_MOVEDATA_TC_AIR;
				}

				if ((xaiBuildOptionDef->typeMask & MASK_BUILDER_STATIC) != 0) {
					xaiUnitDef->isHubBuilder = true;
				}
			}

			if (!xaiBuildOptionDef->buildOptionUDIDs.empty()) {
				isSpecialBuilder = false;
			}
		}

		xaiUnitDef->isSpecialBuilder = isSpecialBuilder;
	}
}

XAICUnitDefHandler::~XAICUnitDefHandler() {
	unitDefIDSets.clear();

	for (int id = 1; id <= xaih->rcb->GetNumUnitDefs(); id++) {
		delete xaiUnitDefsByID[id]; xaiUnitDefsByID[id] = 0;
	}

	xaiUnitDefsByID.clear();
	sprUnitDefsByID.clear();
}

void XAICUnitDefHandler::WriteLog() {
	std::stringstream msg;

	for (int id = 1; id <= xaih->rcb->GetNumUnitDefs(); id++) {
		if (sprUnitDefsByID[id] == NULL) {
			continue;
		}

		const XAIUnitDef* xaiUnitDef = xaiUnitDefsByID[id];

		msg << "UnitDef ID: " << id << ", name: " << (sprUnitDefsByID[id]->name) << "\n";
		msg << "\t(normalized) ";
		msg << "bld. time: " << xaiUnitDef->nBuildTime << ", ";
		msg << "mtl. cost: " << xaiUnitDef->nMetalCost << ", ";
		msg << "nrg. cost: " << xaiUnitDef->nEnergyCost << ", ";
		msg << "max. speed: " << xaiUnitDef->nMaxMoveSpeed << ", ";
		msg << "extr. depth: " << xaiUnitDef->nExtractsMetal << "\n";
		msg << "\n\t";
		msg << "isHubBuilder: " << xaiUnitDef->isHubBuilder << ", ";
		msg << "isSpecialBuilder: " << xaiUnitDef->isSpecialBuilder << "\n";
		msg << "\n";
		msg << "\tType, Terrain, Weapon [, Class] Masks:\n";

		if (xaiUnitDef->typeMask & MASK_BUILDER_MOBILE   ) { msg << "\t\tMASK_BUILDER_MOBILE    = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_BUILDER_STATIC   ) { msg << "\t\tMASK_BUILDER_STATIC    = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_ASSISTER_MOBILE  ) { msg << "\t\tMASK_ASSISTER_MOBILE   = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_ASSISTER_STATIC  ) { msg << "\t\tMASK_ASSISTER_STATIC   = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_E_PRODUCER_MOBILE) { msg << "\t\tMASK_E_PRODUCER_MOBILE = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_E_PRODUCER_STATIC) { msg << "\t\tMASK_E_PRODUCER_STATIC = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_M_PRODUCER_MOBILE) { msg << "\t\tMASK_M_PRODUCER_MOBILE = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_M_PRODUCER_STATIC) { msg << "\t\tMASK_M_PRODUCER_STATIC = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_E_STORAGE_MOBILE ) { msg << "\t\tMASK_E_STORAGE_MOBILE  = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_E_STORAGE_STATIC ) { msg << "\t\tMASK_E_STORAGE_STATIC  = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_M_STORAGE_MOBILE ) { msg << "\t\tMASK_M_STORAGE_MOBILE  = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_M_STORAGE_STATIC ) { msg << "\t\tMASK_M_STORAGE_STATIC  = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_DEFENSE_STATIC   ) { msg << "\t\tMASK_DEFENSE_STATIC    = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_DEFENSE_MOBILE   ) { msg << "\t\tMASK_DEFENSE_MOBILE    = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_OFFENSE_STATIC   ) { msg << "\t\tMASK_OFFENSE_STATIC    = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_OFFENSE_MOBILE   ) { msg << "\t\tMASK_OFFENSE_MOBILE    = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_INTEL_MOBILE     ) { msg << "\t\tMASK_INTEL_MOBILE      = 1\n"; }
		if (xaiUnitDef->typeMask & MASK_INTEL_STATIC     ) { msg << "\t\tMASK_INTEL_STATIC      = 1\n"; }

		if (xaiUnitDef->terrainMask & MASK_LAND           ) { msg << "\t\tMASK_LAND              = 1\n"; }
		if (xaiUnitDef->terrainMask & MASK_WATER_SURFACE  ) { msg << "\t\tMASK_WATER_SURFACE     = 1\n"; }
		if (xaiUnitDef->terrainMask & MASK_WATER_SUBMERGED) { msg << "\t\tMASK_WATER_SUBMERGED   = 1\n"; }
		if (xaiUnitDef->terrainMask & MASK_AIR            ) { msg << "\t\tMASK_AIR               = 1\n"; }

		if (xaiUnitDef->weaponMask & MASK_ARMED     ) { msg << "\t\tMASK_ARMED             = 1\n"; }
		if (xaiUnitDef->weaponMask & MASK_NUKE      ) { msg << "\t\tMASK_NUKE              = 1\n"; }
		if (xaiUnitDef->weaponMask & MASK_ANTINUKE  ) { msg << "\t\tMASK_ANTINUKE          = 1\n"; }
		if (xaiUnitDef->weaponMask & MASK_SHIELD    ) { msg << "\t\tMASK_SHIELD            = 1\n"; }
	//	if (xaiUnitDef->weaponMask & MASK_STOCKPILE ) { msg << "\t\t\n"; }
	//	if (xaiUnitDef->weaponMask & MASK_MANUALFIRE) { msg << "\t\t\n"; }

		if (xaiUnitDef->typeMask & MASK_BUILDER_STATIC) {
			msg << "\n\tBuildOption MoveData Masks:\n";

			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_MT_GND) { msg << "\t\tMASK_MOVEDATA_MT_GND   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_MT_HVR) { msg << "\t\tMASK_MOVEDATA_MT_HVR   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_MT_SHP) { msg << "\t\tMASK_MOVEDATA_MT_SHP   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_MT_AIR) { msg << "\t\tMASK_MOVEDATA_MT_AIR   = 1\n"; }

			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_MF_TNK) { msg << "\t\tMASK_MOVEDATA_MF_TNK   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_MF_KBT) { msg << "\t\tMASK_MOVEDATA_MF_KBT   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_MF_HVR) { msg << "\t\tMASK_MOVEDATA_MF_HVR   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_MF_SHP) { msg << "\t\tMASK_MOVEDATA_MF_SHP   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_MF_AIR) { msg << "\t\tMASK_MOVEDATA_MF_AIR   = 1\n"; }

			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_TC_LND) { msg << "\t\tMASK_MOVEDATA_TC_LND   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_TC_WTR) { msg << "\t\tMASK_MOVEDATA_TC_WTR   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_TC_MXD) { msg << "\t\tMASK_MOVEDATA_TC_MXD   = 1\n"; }
			if (xaiUnitDef->boMoveDataMask & MASK_MOVEDATA_TC_AIR) { msg << "\t\tMASK_MOVEDATA_TC_AIR   = 1\n"; }
		}

		msg << "\n";
	}

	log << (msg.str());
}



/*
bool XAICUnitDefHandler::CanBuild(unsigned int typeMaskBit, unsigned int terrMaskBit, unsigned int weapMaskBit) const {
	bool b0 = false;
	bool b1 = false;
	bool b2 = false;

	switch (typeMaskBit) {
		case MASK_BUILDER_MOBILE:    { b0 = (!mobileBuilderUnitDefIDs.empty());   } break;
		case MASK_BUILDER_STATIC:    { b0 = (!staticBuilderUnitDefIDs.empty());   } break;
		case MASK_ASSISTER_MOBILE:   { b0 = (!mobileAssisterUnitDefIDs.empty());  } break;
		case MASK_ASSISTER_STATIC:   { b0 = (!staticAssisterUnitDefIDs.empty());  } break;
		case MASK_E_PRODUCER_MOBILE: { b0 = (!mobileEProducerUnitDefIDs.empty()); } break;
		case MASK_E_PRODUCER_STATIC: { b0 = (!staticEProducerUnitDefIDs.empty()); } break;
		case MASK_M_PRODUCER_MOBILE: { b0 = (!mobileMProducerUnitDefIDs.empty()); } break;
		case MASK_M_PRODUCER_STATIC: { b0 = (!staticMProducerUnitDefIDs.empty()); } break;
		case MASK_E_STORAGE_MOBILE:  { b0 = (!mobileEStorageUnitDefIDs.empty());  } break;
		case MASK_E_STORAGE_STATIC:  { b0 = (!staticEStorageUnitDefIDs.empty());  } break;
		case MASK_M_STORAGE_MOBILE:  { b0 = (!mobileMStorageUnitDefIDs.empty());  } break;
		case MASK_M_STORAGE_STATIC:  { b0 = (!staticMStorageUnitDefIDs.empty());  } break;
		case MASK_DEFENSE_STATIC:    { b0 = (!staticDefenseUnitDefIDs.empty());   } break;
		case MASK_DEFENSE_MOBILE:    { b0 = (!mobileDefenseUnitDefIDs.empty());   } break;
		case MASK_OFFENSE_STATIC:    { b0 = (!staticOffenseUnitDefIDs.empty());   } break;
		case MASK_OFFENSE_MOBILE:    { b0 = (!mobileOffenseUnitDefIDs.empty());   } break;
		case MASK_INTEL_MOBILE:      { b0 = (!mobileIntelUnitDefIDs.empty());     } break;
		case MASK_INTEL_STATIC:      { b0 = (!staticIntelUnitDefIDs.empty());     } break;
		default:                     { } break;
	}

	switch (terrMaskBit) {
		case MASK_LAND:            { b1 = (!landUnitDefIDs.empty());           } break;
		case MASK_WATER_SURFACE:   { b1 = (!surfaceWaterUnitDefIDs.empty());   } break;
		case MASK_WATER_SUBMERGED: { b1 = (!submergedWaterUnitDefIDs.empty()); } break;
		case MASK_AIR:             { b1 = (!airUnitDefIDs.empty());            } break;
		default:                   { } break;
	}

	switch (weapMaskBit) {
		case MASK_ARMED:      { b2 = (!armedUnitDefIDs.empty());    } break;
		case MASK_NUKE:       { b2 = (!nukeUnitDefIDs.empty());     } break;
		case MASK_ANTINUKE:   { b2 = (!antiNukeUnitDefIDs.empty()); } break;
		case MASK_SHIELD:     { b2 = (!shieldUnitDefIDs.empty());   } break;
//		case MASK_STOCKPILE:  { } break;
//		case MASK_MANUALFIRE: { } break;
		default:              { } break;
	}

	return (b0 && b1 && b2);
}
*/



std::list< std::set<int>* > XAICUnitDefHandler::GetUnitDefIDSetsForID(int uDefID) {
	std::list< std::set<int>* > uSets;
	std::vector< std::set<int>* >::iterator uSetIt;

	for (uSetIt = unitDefIDSets.begin(); uSetIt != unitDefIDSets.end(); uSetIt++) {
		std::set<int>* uSet = *uSetIt;

		if (uSet->find(uDefID) != uSet->end()) {
			uSets.push_back(uSet);
		}
	}

	return uSets;
}

std::list<std::set<int>* > XAICUnitDefHandler::GetUnitDefIDSetsForMask(unsigned int typeMaskBits, unsigned int terrMaskBits, unsigned int weapMaskBits) {
	std::list< std::set<int>* > uSets;

	if (typeMaskBits != 0) {
		if (typeMaskBits & MASK_BUILDER_MOBILE   ) { uSets.push_back(&mobileBuilderUnitDefIDs); }
		if (typeMaskBits & MASK_BUILDER_STATIC   ) { uSets.push_back(&staticBuilderUnitDefIDs); }
		if (typeMaskBits & MASK_ASSISTER_MOBILE  ) { uSets.push_back(&mobileAssisterUnitDefIDs); }
		if (typeMaskBits & MASK_ASSISTER_STATIC  ) { uSets.push_back(&staticAssisterUnitDefIDs); }
		if (typeMaskBits & MASK_E_PRODUCER_MOBILE) { uSets.push_back(&mobileEProducerUnitDefIDs); }
		if (typeMaskBits & MASK_E_PRODUCER_STATIC) { uSets.push_back(&staticEProducerUnitDefIDs); }
		if (typeMaskBits & MASK_M_PRODUCER_MOBILE) { uSets.push_back(&mobileMProducerUnitDefIDs); }
		if (typeMaskBits & MASK_M_PRODUCER_STATIC) { uSets.push_back(&staticMProducerUnitDefIDs); }
		if (typeMaskBits & MASK_E_STORAGE_MOBILE ) { uSets.push_back(&mobileEStorageUnitDefIDs); }
		if (typeMaskBits & MASK_E_STORAGE_STATIC ) { uSets.push_back(&staticEStorageUnitDefIDs); }
		if (typeMaskBits & MASK_M_STORAGE_MOBILE ) { uSets.push_back(&mobileMStorageUnitDefIDs); }
		if (typeMaskBits & MASK_M_STORAGE_STATIC ) { uSets.push_back(&staticMStorageUnitDefIDs); }
		if (typeMaskBits & MASK_DEFENSE_STATIC   ) { uSets.push_back(&staticDefenseUnitDefIDs); }
		if (typeMaskBits & MASK_DEFENSE_MOBILE   ) { uSets.push_back(&mobileDefenseUnitDefIDs); }
		if (typeMaskBits & MASK_OFFENSE_STATIC   ) { uSets.push_back(&staticOffenseUnitDefIDs); }
		if (typeMaskBits & MASK_OFFENSE_MOBILE   ) { uSets.push_back(&mobileOffenseUnitDefIDs); }
		if (typeMaskBits & MASK_INTEL_MOBILE     ) { uSets.push_back(&mobileIntelUnitDefIDs); }
		if (typeMaskBits & MASK_INTEL_STATIC     ) { uSets.push_back(&staticIntelUnitDefIDs); }
	}

	if (terrMaskBits != 0) {
		if (terrMaskBits & MASK_LAND           ) { uSets.push_back(&landUnitDefIDs); }
		if (terrMaskBits & MASK_WATER_SURFACE  ) { uSets.push_back(&surfaceWaterUnitDefIDs); }
		if (terrMaskBits & MASK_WATER_SUBMERGED) { uSets.push_back(&submergedWaterUnitDefIDs); }
		if (terrMaskBits & MASK_AIR            ) { uSets.push_back(&airUnitDefIDs); }
	}

	if (weapMaskBits != 0) {
		if (weapMaskBits & MASK_ARMED     ) { uSets.push_back(&armedUnitDefIDs); }
		if (weapMaskBits & MASK_NUKE      ) { uSets.push_back(&nukeUnitDefIDs); }
		if (weapMaskBits & MASK_ANTINUKE  ) { uSets.push_back(&antiNukeUnitDefIDs); }
		if (weapMaskBits & MASK_SHIELD    ) { uSets.push_back(&shieldUnitDefIDs); }
	//	if (wMaskBits & MASK_STOCKPILE ) { uSets.push_back(&stockpileUnitDefIDs); }
	//	if (wMaskBits & MASK_MANUALFIRE) { uSets.push_back(&manualFireUnitDefIDs); }
	}

	return uSets;
}

std::set<int> XAICUnitDefHandler::GetUnitDefIDsForMask(unsigned int typeBits, unsigned int terrBits, unsigned int weapBits, bool intersection) {
	std::list<std::set<int>* > uSets = GetUnitDefIDSetsForMask(typeBits, terrBits, weapBits);
	std::set<int> uDefIDs;

	if (intersection) {
		// turn list of sets [<A1, A2><B1, B2, B3><C1>] into
		// a single joint set <A1 ^ A2 ^ B1 ^ B2 ^ B3 ^ C1>
		std::list<std::set<int>* >::iterator slit;

		// copy s to uDefIDs
		std::set<int>* s = *(uSets.begin());
		std::copy(s->begin(), s->end(), std::inserter(uDefIDs, uDefIDs.begin()));

		for (slit = ++(uSets.begin()); slit != uSets.end(); slit++) {
			uDefIDs = XAIUtil::IntersectSets(uDefIDs, *(*slit));
		}
	} else {
		// turn list of sets [<A1, A2><B1, B2, B3><C1>] into
		// a single set <(A1 v A2) ^ (B1 v B2 v B3) ^ (C1)>
		std::set<int> s1, s2, s3;
		std::set<int>* s0 = NULL;
		std::list<std::set<int>* >::iterator slit = uSets.begin();

		unsigned int numTypeMaskBits    = XAIUtil::CountOneBits(typeBits);
		unsigned int numTerrainMaskBits = XAIUtil::CountOneBits(terrBits);
		unsigned int numWeaponMaskBits  = XAIUtil::CountOneBits(weapBits);

		if (typeBits != 0) {
			for (int i = 0; i < numTypeMaskBits; i++) {
				std::copy((*slit)->begin(), (*slit)->end(), std::inserter(s1, s1.begin())); slit++;
			}

			s0 = &s1;
		}

		if (terrBits != 0) {
			for (int i = 0; i < numTerrainMaskBits; i++) {
				std::copy((*slit)->begin(), (*slit)->end(), std::inserter(s2, s2.begin())); slit++;
			}

			if (s0 != NULL) {
				s2 = XAIUtil::IntersectSets(*s0, s2);
			}

			s0 = &s2;
		}

		if (weapBits != 0) {
			for (int i = 0; i < numWeaponMaskBits; i++) {
				std::copy((*slit)->begin(), (*slit)->end(), std::inserter(s3, s3.begin())); slit++;
			}

			if (s0 != NULL) {
				s3 = XAIUtil::IntersectSets(*s0, s3);
			}

			s0 = &s3;
		}

		if (s0 != NULL) {
			std::copy(s0->begin(), s0->end(), std::inserter(uDefIDs, uDefIDs.begin()));
		}
	}

	/*
	for (int i = 1; i < xaiUnitDefsByID.size(); i++) {
		const XAIUnitDef* def = xaiUnitDefs[i];

		const bool b0 = (typeM != 0 && ((def->typeMask    & typeM) != 0));
		const bool b1 = (terrM != 0 && ((def->terrainMask & terrM) != 0));
		const bool b2 = (weapM != 0 && ((def->weaponMask  & weapM) != 0));

		if (b0 || b1 || b2) {
			uDefIDs.insert(i);
		}
	}
	*/

	return uDefIDs;
}

int XAICUnitDefHandler::InsertUnitDefByID(int i) {
	const XAIUnitDef* xaiDef = xaiUnitDefsByID[i];
	const UnitDef* sprDef = sprUnitDefsByID[i];

	assert(xaiDef->GetDef() == sprDef);

	int n = 0;

	std::list<std::set<int>* > sets = GetUnitDefIDSetsForMask(xaiDef->typeMask, xaiDef->terrainMask, xaiDef->weaponMask);
	std::list<std::set<int>* >::iterator setsIt;

	for (setsIt = sets.begin(); setsIt != sets.end(); setsIt++) {
		(*setsIt)->insert(sprDef->id); n++;
	}

	return n;
}

void XAICUnitDefHandler::CategorizeUnitDefByID(int id) {
	// note: canMove can be true even for buildings
	// (canVerb means "this unit can be given Verb
	// commands, but not necessarily execute them")
	// similarly, "builder = true" does not imply a
	// non-empty build-options list
	//
	// note: even if a unit has a MoveData instance,
	// it might STILL be immobile if its max. speed
	// is zero, so !isMobile does not imply movedata
	// == NULL
	//
	// const int unitCost = int(xaiUnitDef->metalCost * METAL2ENERGY + xaiUnitDef->energyCost);

	// temporarily eliminate the const
	XAIUnitDef* xaiUnitDef = const_cast<XAIUnitDef*>(xaiUnitDefsByID[id]);
	UnitDef* sprUnitDef = const_cast<UnitDef*>(sprUnitDefsByID[id]);

	xaiUnitDef->SetUnitDef(sprUnitDef);
	xaiUnitDef->SetDGunWeaponDef(xaiUnitDef->GetDGunWeaponDef());

	xaiUnitDef->isHubBuilder     = false;
	xaiUnitDef->isSpecialBuilder = false;
	xaiUnitDef->isMobile         = ((sprUnitDef->speed > 0.0f) && ((sprUnitDef->canmove && sprUnitDef->movedata != NULL) || sprUnitDef->canfly));
	xaiUnitDef->isAttacker       = (!sprUnitDef->weapons.empty());
	xaiUnitDef->isBuilder        = (!sprUnitDef->buildOptions.empty());
	xaiUnitDef->typeMask         = 0;
	xaiUnitDef->terrainMask      = 0;
	xaiUnitDef->weaponMask       = 0;
	xaiUnitDef->classMask        = 0;
	xaiUnitDef->boMoveDataMask   = 0;


	if (!xaiUnitDef->isMobile) {
		assert(!sprUnitDef->canfly);
	}

	if (!xaiUnitDef->isBuilder) {
		if (sprUnitDef->canAssist || sprUnitDef->canRepair) {
			xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_ASSISTER_MOBILE: MASK_ASSISTER_STATIC);
		}
	} else {
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_BUILDER_MOBILE: MASK_BUILDER_STATIC);
	}


	// intelligence
	if ((sprUnitDef->radarRadius > 0 || sprUnitDef->jammerRadius  > 0)) {
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_INTEL_MOBILE: MASK_INTEL_STATIC);
	}
	if ((sprUnitDef->sonarRadius  > 0 || sprUnitDef->sonarJamRadius > 0)) {
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_INTEL_MOBILE: MASK_INTEL_STATIC);
	}
	if ((sprUnitDef->seismicRadius > 0)) {
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_INTEL_MOBILE: MASK_INTEL_STATIC);
	}


	// resources (metal)
	if (xaiUnitDef->ResMakeOff('M', 0.0f, 0.0f) > 0.0f) {
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_M_PRODUCER_MOBILE: MASK_M_PRODUCER_STATIC);
	}
	if (xaiUnitDef->IsResGenerator('M')) {
		// extractor or maker (when active)
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_M_PRODUCER_MOBILE: MASK_M_PRODUCER_STATIC);
	}

	// resources (energy)
	if (xaiUnitDef->ResMakeOff('E', 0.0f, 0.0f) > 0.0f) {
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_E_PRODUCER_MOBILE: MASK_E_PRODUCER_STATIC);
	}
	if (xaiUnitDef->IsResGenerator('E')) {
		// wind or tidal
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_E_PRODUCER_MOBILE: MASK_E_PRODUCER_STATIC);
	}

	// resources ({M, E} storage)
	if (sprUnitDef->metalStorage  > 0.0f) {
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_M_STORAGE_MOBILE: MASK_M_STORAGE_STATIC);
	}
	if (sprUnitDef->energyStorage > 0.0f) {
		xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_E_STORAGE_MOBILE: MASK_E_STORAGE_STATIC);
	}


	xaiUnitDef->minWeaponRange = 1e9f;
	xaiUnitDef->maxWeaponRange = 0.0f;

	if (!sprUnitDef->weapons.empty()) {
		std::vector<UnitDef::UnitDefWeapon>::const_iterator wit;

		for (wit = sprUnitDef->weapons.begin(); wit != sprUnitDef->weapons.end(); wit++) {
			const WeaponDef* w = wit->def;

			// if true, the weapon can target
			// and shoot at underwater stuff
			// if (w->waterweapon) {}

			if (!w->stockpile && !w->noAutoTarget && !w->isShield && !w->targetable && !w->interceptor) {
				// regular weapon (note the offense/defense switch)
				xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_OFFENSE_MOBILE: MASK_DEFENSE_STATIC);
				xaiUnitDef->weaponMask |= MASK_ARMED;

				xaiUnitDef->minWeaponRange = std::min(xaiUnitDef->minWeaponRange, w->range);
				xaiUnitDef->maxWeaponRange = std::max(xaiUnitDef->maxWeaponRange, w->range);
			} else {
				// weapon of some special nature
				if (w->stockpile) {
					assert(sprUnitDef->stockpileWeaponDef != NULL);
					// weapon that uses ammunition (do we need this?)
					// xaiUnitDef->weaponMask |= MASK_STOCKPILE;
				}
				if (w->noAutoTarget) {
					// manual-target weapon (do we need this?)
					// xaiUnitDef->weaponMask |= MASK_MANUALFIRE;
				}

				if (w->isShield) {
					assert(sprUnitDef->shieldWeaponDef != NULL);
					// (possibly mobile) shield generator
					xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_DEFENSE_MOBILE: MASK_DEFENSE_STATIC);
					xaiUnitDef->weaponMask |= MASK_SHIELD;
				}

				if (!w->interceptor) {
					if (w->targetable) {
						// (possibly mobile) nuke launcher
						// (weapon that can be intercepted,
						// may or may not need stockpiling)
						xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_OFFENSE_MOBILE: MASK_OFFENSE_STATIC);
						xaiUnitDef->weaponMask |= MASK_NUKE;
					}
				} else {
					xaiUnitDef->typeMask |= (xaiUnitDef->isMobile? MASK_DEFENSE_MOBILE: MASK_DEFENSE_STATIC);
					// (possibly mobile) anti-nuke
					// there are no anti-anti weapons
					xaiUnitDef->weaponMask |= MASK_ANTINUKE;
				}
			}
		}
	}


	if (!xaiUnitDef->isMobile) {
		// for buildings, minWaterDepth tells us
		// all we need to know about the terrain
		// requirements (default {min, max} vals
		// are -10e6 and +10e6)
		if ((sprUnitDef->minWaterDepth >= 0.0f) /*&& (sprUnitDef->maxWaterDepth >= sprUnitDef->minWaterDepth)*/) {
			xaiUnitDef->terrainMask |= (sprUnitDef->floater? MASK_WATER_SURFACE: MASK_WATER_SUBMERGED);
		} else {
			xaiUnitDef->terrainMask |= MASK_LAND;
		}
	} else {
		if (sprUnitDef->canfly) {
			assert(sprUnitDef->movedata == NULL);
			xaiUnitDef->terrainMask |= MASK_AIR;
		} else {
			assert(sprUnitDef->movedata != NULL);

			switch (sprUnitDef->movedata->moveFamily) {
				case MoveData::Tank: {
					// fall-through
				}
				case MoveData::KBot: {
					assert(sprUnitDef->movedata->moveType == MoveData::Ground_Move);
					assert(sprUnitDef->movedata->followGround);

					xaiUnitDef->terrainMask |= MASK_LAND;

					// in practice, most land units can move into
					// shallow water, so this is not very useful
					// (depth here represents maxWaterDepth)
					if (sprUnitDef->movedata->depth > 0.0f) {
						xaiUnitDef->terrainMask |= MASK_WATER_SUBMERGED;
					}
				} break;
				case MoveData::Hover: {
					assert(sprUnitDef->movedata->moveType == MoveData::Hover_Move);
					assert(!sprUnitDef->movedata->followGround);
					assert(sprUnitDef->canhover);

					//! hovercraft are also floaters
					//! assert(!sprUnitDef->floater);
					//! for tanks, "minWaterDepth" should be < 0.0f
					//! for hovers, "maxWaterDepth" should be > 0.0f

					xaiUnitDef->terrainMask |= MASK_LAND;
					xaiUnitDef->terrainMask |= MASK_WATER_SURFACE;
				} break;
				case MoveData::Ship: {
					// floater is true if "waterline" key exists, which submarines ALSO have
					// followGround is true if movefamily is Tank or KBot, submarines are Ship
					assert(sprUnitDef->movedata->moveType == MoveData::Ship_Move);
					assert(!sprUnitDef->movedata->followGround);
					assert(!sprUnitDef->canhover);
					assert(sprUnitDef->floater);

					// explicit || implicit
					if (sprUnitDef->movedata->subMarine || (sprUnitDef->waterline >= sprUnitDef->movedata->depth)) {
						xaiUnitDef->terrainMask |= MASK_WATER_SUBMERGED;
					} else {
						xaiUnitDef->terrainMask |= MASK_WATER_SURFACE;
					}
				} break;
			}

			if (sprUnitDef->movedata->terrainClass == MoveData::Mixed) {
				assert(
					( xaiUnitDef->terrainMask & MASK_LAND) &&
					((xaiUnitDef->terrainMask & MASK_WATER_SURFACE) ||
					( xaiUnitDef->terrainMask & MASK_WATER_SUBMERGED))
				);
			}
		}
	}

	assert(!((xaiUnitDef->terrainMask & MASK_WATER_SURFACE) && (xaiUnitDef->terrainMask & MASK_WATER_SUBMERGED)));


	// copy the build options to a more convenient format
	std::map<int, std::string>& bldOpts = sprUnitDef->buildOptions;
	std::map<int, std::string>::const_iterator bldOptsIt = bldOpts.begin();

	for (; bldOptsIt != bldOpts.end(); bldOptsIt++) {
		const char*    bldOptName = bldOptsIt->second.c_str();
		const UnitDef* bldOptDef  = xaih->rcb->GetUnitDef(bldOptName);

		assert(bldOptDef != NULL);

		xaiUnitDef->buildOptionUDIDs.insert(bldOptDef->id);
	}
}
