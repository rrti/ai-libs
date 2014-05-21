#include <sstream>

#include "LegacyCpp/IAICallback.h"
#include "System/float3.h"

#include "./XAIPathFinder.hpp"
#include "./XAIIPathNode.hpp"
#include "../map/XAIMap.hpp"
#include "../map/XAIMaskMap.hpp"
#include "../main/XAIHelper.hpp"
#include "../main/XAIConstants.hpp"
#include "../units/XAIUnit.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../groups/XAIGroup.hpp"
#include "../utils/XAITimer.hpp"

XAICPathFinder::XAICPathFinder(XAIHelper* h): xaih(h) {
	XAICScopedTimer t("[XAICPathFinder::XAICPathFinder]", xaih->timer);

	const int hmapx = h->rcb->GetMapWidth();
	const int hmapy = h->rcb->GetMapHeight();
	const int smapx = HEIGHT2SLOPE(hmapx);
	const int smapy = HEIGHT2SLOPE(hmapy);

	const float* sprHeightMap = h->rcb->GetHeightMap();
	const float* sprSlopeMap = h->rcb->GetSlopeMap();

	xaiHeightMap = new XAIMap<float>(hmapx, hmapy, 0.0f, XAI_HEIGHT_MAP);
	xaiSlopeMap = new XAIMap<float>(smapx, smapy, 0.0f, XAI_SLOPE_MAP);

	xaiHeightMap->Copy(sprHeightMap);
	xaiSlopeMap->Copy(sprSlopeMap);


	nodes.resize(hmapx * hmapy, NULL);

	// initialize the node-map
	for (int i = 0; i < (hmapx * hmapy); i += 2) {
		nodes[i    ] = new XAIIPathNode(i    );
		nodes[i + 1] = new XAIIPathNode(i + 1);
	}

	// retrieve the unique MoveData's
	for (int id = 1; id <= xaih->rcb->GetNumUnitDefs(); id++) {
		const XAIUnitDef* ud = xaih->unitDefHandler->GetUnitDefByID(id);
		const MoveData* md = ud->GetMoveData();

		if (md != NULL) {
			if (moveDataMap.find(md->pathType) == moveDataMap.end()) {
				moveDataMap[md->pathType] = md;
			} else {
				// fails due to the interface-wrapper code
				// assert(moveDataMap[md->pathType] == md);
				assert(moveDataMap[md->pathType]->pathType      == md->pathType);
				assert(moveDataMap[md->pathType]->moveType      == md->moveType);
				assert(moveDataMap[md->pathType]->moveFamily    == md->moveFamily);
				assert(moveDataMap[md->pathType]->terrainClass  == md->terrainClass);
				assert(moveDataMap[md->pathType]->followGround  == md->followGround);
				assert(moveDataMap[md->pathType]->size          == md->size);
				assert(moveDataMap[md->pathType]->depth         == md->depth);
				assert(moveDataMap[md->pathType]->maxSlope      == md->maxSlope);
				assert(moveDataMap[md->pathType]->slopeMod      == md->slopeMod);
				assert(moveDataMap[md->pathType]->depthMod      == md->depthMod);
				assert(moveDataMap[md->pathType]->crushStrength == md->crushStrength);
			}
		}
	}

	// generate the MoveData masks (too slow)
	//    spread over multiple frames and/or
	//    change flood-fill to use scanlines and/or
	//    make threaded and/or
	//    do lazy-loading and/or
	//    cache the mask-maps (a la Spring) and/or
	//    look for MD's with equal maxSlope / maxDepth
	//    precompute pixel filter values in XAIMap<bool>'s
	// NOTE:
	//    flood-filled regions are not necessarily convex!
	//    (thus two points within the same region are not
	//    necessarily connected by a straight line)
	/*
	for (std::map<int, const MoveData*>::const_iterator it = moveDataMap.begin(); it != moveDataMap.end(); it++) {
		std::stringstream ss("");
			ss.width(2);
			ss << "[XAICPathFinder::XAICPathFinder]";
			ss << "[MaskMap(" << it->first << ")]";
		std::string s = ss.str();

		XAICScopedTimer st(s.c_str(), xaih->timer);

		XAIIMapPixelFilter<float>* mpf = new XAIMapPixelHeightSlopeFilter<float>(it->second);
		XAIMaskMap<float>* maskMap = new XAIMaskMap<float>();
			maskMap->AddMap(xaiHeightMap);
			maskMap->AddMap(xaiSlopeMap);
			maskMap->SetMapPixelFilter(mpf);
			maskMap->GenerateMasks();
		maskMaps[it->first] = maskMap;
		delete mpf;
	}
	*/
}

XAICPathFinder::~XAICPathFinder() {
	for (int i = 0; i < nodes.size(); i += 2) {
		delete nodes[i    ];
		delete nodes[i + 1];
	}

	nodes.clear();

	delete xaiHeightMap;
	delete xaiSlopeMap;

	for (std::map<int, const MoveData*>::const_iterator it = moveDataMap.begin(); it != moveDataMap.end(); it++) {
		delete maskMaps[it->first];
	}

	moveDataMap.clear();
}



bool XAICPathFinder::IsPathPossible(const XAIGroup* g, const float3& wStart, const float3& wGoal) const {
	if (g->GetUnitCount() == 0) {
		return false;
	}
	if (!g->IsMobile()) {
		return false;
	}

	// float3::IsInBounds appears to be broken AI-side
	if ((wStart.x < 0.0f) || (wStart.x >= xaih->rcb->GetMapWidth()  * SQUARE_SIZE)) { return false; }
	if ((wStart.z < 0.0f) || (wStart.z >= xaih->rcb->GetMapHeight() * SQUARE_SIZE)) { return false; }

	const XAICUnit*   u  = g->GetLeadUnitMember();
	const XAIUnitDef* ud = u->GetUnitDefPtr();
	const MoveData*   md = ud->GetMoveData();

	if (md == NULL) {
		// aircraft groups should not need to use the PF
		return true;
	}

	std::map<int, XAIMaskMap<float>* >::const_iterator it = maskMaps.find(md->pathType);

	if (it == maskMaps.end()) {
		return false; // lazy-loading?
	}

	const XAIMaskMap<float>* maskMap = it->second;
	const std::vector<const XAIMap<float>* >& maps = maskMap->GetMaps();
	const std::vector< XAIMap<int>* >& masks = maskMap->GetMasks();

	bool ret = false;

	for (int i = 0; i < maps.size(); i++) {
		const XAIMap<float>* map = maps[i];
		const XAIMap<int>* mask = masks[i];

		switch (map->GetType()) {
			case XAI_HEIGHT_MAP: {
				// convert start and goal to HM-space
				const int xS = WORLD2HEIGHT(int(wStart.x));
				const int zS = WORLD2HEIGHT(int(wStart.z));
				const int xG = WORLD2HEIGHT(int(wGoal.x));
				const int zG = WORLD2HEIGHT(int(wGoal.z));

				const int maskS = (mask->GetConstPixel(xS, zS))->GetValue();
				const int maskG = (mask->GetConstPixel(xG, zG))->GetValue();

				ret = ret && (maskS == maskG);
			} break;
			case XAI_SLOPE_MAP: {
				// convert start and goal to SM-space
				const int xS = WORLD2SLOPE(int(wStart.x));
				const int zS = WORLD2SLOPE(int(wStart.z));
				const int xG = WORLD2SLOPE(int(wGoal.x));
				const int zG = WORLD2SLOPE(int(wGoal.z));

				const int maskS = (mask->GetConstPixel(xS, zS))->GetValue();
				const int maskG = (mask->GetConstPixel(xG, zG))->GetValue();

				ret = ret && (maskS == maskG);
			} break;
			default: {
			} break;
		}
	}

	return ret;
}
