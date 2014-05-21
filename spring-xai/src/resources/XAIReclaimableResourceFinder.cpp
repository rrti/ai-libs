#include <cassert>

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/IAICheats.h"
#include "LegacyCpp/UnitDef.h"
#include "Sim/Features/FeatureDef.h"

#include "./XAIIResourceFinder.hpp"
#include "../main/XAIHelper.hpp"

void XAICReclaimableResourceFinder::FindResources() {
	recResources.clear();

	const int numUIDs = xaih->ccb->GetNeutralUnits(&uIDs[0], MAX_UNITS);
	const int numFIDs = xaih->ccb->GetFeatures(&fIDs[0], MAX_FEATURES);

	for (int i = 0; i < numUIDs; i++) {
		XAIReclaimableResource r;
			r.typeMask    = XAI_RESOURCETYPE_BASE;
			r.pos         = xaih->ccb->GetUnitPos(uIDs[i]);
			r.uDef        = xaih->ccb->GetUnitDef(uIDs[i]);
			r.fDef        = NULL;

			if (r.uDef == NULL)
				continue;

			if (r.uDef->metalCost  > 0.0f) { r.typeMask |= XAI_RESOURCETYPE_METAL;  }
			if (r.uDef->energyCost > 0.0f) { r.typeMask |= XAI_RESOURCETYPE_ENERGY; }

		recResources.push_back(r);
	}
	for (int i = 0; i < numFIDs; i++) {
		XAIReclaimableResource r;
			r.typeMask    = XAI_RESOURCETYPE_BASE;
			r.pos         = xaih->rcb->GetFeaturePos(fIDs[i]);
			r.uDef        = NULL;
			r.fDef        = xaih->rcb->GetFeatureDef(fIDs[i]);

			if (r.fDef == NULL)
				continue;

			if (r.fDef->metal      > 0.0f) { r.typeMask |= XAI_RESOURCETYPE_METAL;  }
			if (r.fDef->energy     > 0.0f) { r.typeMask |= XAI_RESOURCETYPE_ENERGY; }
			if (r.fDef->geoThermal       ) { r.typeMask |= XAI_RESOURCETYPE_ENERGY; }

		recResources.push_back(r);
	}

	// the Get*Def() callbacks can return null (!), so both
	// numUIDs and numFIDs are unreliable and this can fail
	// assert(recResources.size() == (numUIDs + numFIDs));

	// make sure the next GetResources() call
	// refreshes the resources pointer list
	valid = false;
}

std::list<XAIIResource*>& XAICReclaimableResourceFinder::GetResources(bool refresh) {
	if (refresh) {
		FindResources();
	}

	if (!valid) {
		resources.clear();

		for (std::list<XAIReclaimableResource>::iterator it = recResources.begin(); it != recResources.end(); it++) {
			resources.push_back(&(*it));
		}

		valid = true;
	}

	return resources;
}
