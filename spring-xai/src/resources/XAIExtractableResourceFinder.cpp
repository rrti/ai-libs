#include <cassert>
#include <fstream>
#include <sstream>

#include "LegacyCpp/IAICallback.h"
#include "System/float3.h"

#include "./XAIIResourceFinder.hpp"
#include "../main/XAIHelper.hpp"
#include "../main/XAIConstants.hpp"
#include "../main/XAIFolders.hpp"
#include "../utils/XAIUtil.hpp"
#include "../utils/XAITimer.hpp"

typedef unsigned char uint8;


std::string XAICExtractableResourceFinder::GetExtractorCacheName() {
	std::string relName = XAI_MTL_DIR + XAIUtil::StringStripSpaces(xaih->rcb->GetMapName()) + ".mtl";
	std::string absName = XAIUtil::GetAbsFileName(xaih->rcb, relName);
	return absName;
}

bool XAICExtractableResourceFinder::ReadExtractorCache() {
	std::string fn = GetExtractorCacheName();
	std::ifstream fs(fn.c_str(), std::ios::in);

	if (!fs.good()) {
		return false;
	}

	const int    hmw = xaih->rcb->GetMapWidth();
	const float* hm  = xaih->rcb->GetHeightMap();

	int linesRead = 0;

	while (!fs.eof()) {
		std::stringstream ss;
		std::string s;
		std::getline(fs, s);

		if (s.empty()) {
			break;
		}

		ss.str("");
		ss << s;

		if (linesRead > 0) {
			int   epx  = 0;    ss >> epx;
			int   epz  = 0;    ss >> epz;
			float reiv = 0.0f; ss >> reiv;
			float neiv = 0.0f; ss >> neiv;

			// restore coordinate index in metal-map space
			const int epi = (epz * HEIGHT2METAL(hmw)) + epx;

			XAIExtractableResource res;
				res.typeMask         = XAI_RESOURCETYPE_METAL;
				res.pos              = float3(METAL2WORLD(epx), hm[METAL2HEIGHT(epi)], METAL2WORLD(epz));
				res.rExtractionValue = reiv;
				res.nExtractionValue = neiv;
			extResources.push_back(res);
		} else {
			ss >> extResData.avgExtVal;
		}

		linesRead += 1;
	}

	fs.close();
	return true;
}

void XAICExtractableResourceFinder::WriteExtractorCache() {
	std::string fn = GetExtractorCacheName();
	std::ofstream fs(fn.c_str(), std::ios::out);

	fs << extResData.avgExtVal << "\n";

	// save coordinates in metal-map space
	for (std::list<XAIExtractableResource>::const_iterator it = extResources.begin(); it != extResources.end(); it++) {
		const int   epx  = WORLD2METAL(int((*it).pos.x));
		const int   epz  = WORLD2METAL(int((*it).pos.z));
		const float reiv = (*it).rExtractionValue;
		const float neiv = (*it).nExtractionValue;

		fs << epx << " " << epz << " " << reiv << " " << neiv << "\n";
	}

	fs.close();
}



void XAICExtractableResourceFinder::FindExtractorPositions() {
	// GetMap{Width, Height} callbacks return gs->map{x, y},
	// which are the dimensions of the height-map texture
	const int    heightMapW = xaih->rcb->GetMapWidth();
	const int    heightMapH = xaih->rcb->GetMapHeight();
	const uint8* metalMap   = xaih->rcb->GetMetalMap();

	extResData.metalMapW = HEIGHT2METAL(heightMapW);
	extResData.metalMapH = HEIGHT2METAL(heightMapH);
	extResData.metalMapA = extResData.metalMapW * extResData.metalMapH;

	// extractor radius is in world-space by default
	extResData.extRad    = WORLD2METAL(int(xaih->rcb->GetExtractorRadius()));
	extResData.extRadSq  = extResData.extRad * extResData.extRad;
	extResData.extScale  = xaih->rcb->GetMaxMetal();
	extResData.avgExtVal = 0.0f;

	assert(extResData.extRad > 0);

	extResData.rawMetalMap.clear();
	extResData.intMetalMap.clear();
	extResData.rawMetalMap.resize(extResData.metalMapA, 0   );
	extResData.intMetalMap.resize(extResData.metalMapA, 0.0f);
	extResData.extPosIndices.clear();
	extResData.extIntValues.clear();
 
	// (re-)initialize the maps on every call
	for (int i = 0; i < extResData.metalMapA; i++) {
		extResData.rawMetalMap[i] = metalMap[i];
		extResData.intMetalMap[i] = 0.0f;
	}


	XAIExtractableResource res;
	Rectangle rect;
		rect.xmin =                    0;
		rect.zmin =                    0;
		rect.xmax = extResData.metalMapW;
		rect.zmax = extResData.metalMapH;

	IntegrateResourceMap(rect);

	while (FindExtractorPosition(&res)) {
		extResources.push_back(res);

		#ifdef XAI_EXTRACTABLE_RESOURCE_FINDER_DEBUG
		xaih->rcb->CreateLineFigure(res.pos, res.pos + float3(0.0f, 200.0f, 0.0f), 8.0f, 1, 3600, 0);
		#endif
	}

	if (!extResources.empty()) {
		extResData.avgExtVal /= extResources.size();
	}
}

bool XAICExtractableResourceFinder::FindExtractorPosition(XAIExtractableResource* res) {
	float maxExtIntVal =  0.0f;
	int   maxExtPosIdx = -1;

	// find the best non-zero integration position
	// note: if the integration values are ~~equal
	// for all (x, z) (like on Speed*** maps), we
	// do not even need to do this
	for (std::set<int>::const_iterator it = extResData.extPosIndices.begin(); it != extResData.extPosIndices.end(); it++) {
		if (extResData.intMetalMap[*it] > maxExtIntVal) {
			maxExtIntVal = extResData.intMetalMap[*it];
			maxExtPosIdx = *it;
		}
	}

	// guard against maps without any metal
	if (maxExtPosIdx == -1 || maxExtIntVal <= 0.0f) {
		return false;
	}

	if (!extResData.extIntValues.empty() && (maxExtIntVal < ((*extResData.extIntValues.begin()) * 0.1f))) {
		return false;
	}

	extResData.extIntValues.push_back(maxExtIntVal);
	extResData.avgExtVal += maxExtIntVal;

	const int extPosX = maxExtPosIdx % extResData.metalMapW;
	const int extPosZ = maxExtPosIdx / extResData.metalMapW;
	const float* hm = xaih->rcb->GetHeightMap();

	res->typeMask         = XAI_RESOURCETYPE_METAL;
	res->pos              = float3(METAL2WORLD(extPosX), hm[METAL2HEIGHT(maxExtPosIdx)], METAL2WORLD(extPosZ));
	res->rExtractionValue = maxExtIntVal;
	res->nExtractionValue = maxExtIntVal / (*extResData.extIntValues.begin());


	// set the squares covered by the extractor
	// to zero for the next integration round
	const int xminOff = std::min(                       extPosX,     extResData.extRad);
	const int zminOff = std::min(                       extPosZ,     extResData.extRad);
	const int xmaxOff = std::min(extResData.metalMapW - extPosX - 1, extResData.extRad);
	const int zmaxOff = std::min(extResData.metalMapH - extPosZ - 1, extResData.extRad);

	for (int i = -xminOff; i <= xmaxOff; i++) {
		for (int j = -zminOff; j <= zmaxOff; j++) {
			const int iSq = i * i;
			const int jSq = j * j;
			const int idx = (extPosZ + j) * extResData.metalMapW + (extPosX + i);

			if ((iSq + jSq) <= extResData.extRadSq) {
				extResData.rawMetalMap[idx] = 0;
				extResData.extPosIndices.erase(idx);
			}
		}
	}

	Rectangle rect;
		rect.xmin = extPosX - (extResData.extRad << 1);
		rect.zmin = extPosZ - (extResData.extRad << 1);
		rect.xmax = extPosX + (extResData.extRad << 1);
		rect.zmax = extPosZ + (extResData.extRad << 1);
	// re-integrate metal map in affected area
	// based on the changed rawMetalMap values
	IntegrateResourceMap(rect);

	return true;
}

void XAICExtractableResourceFinder::IntegrateResourceMap(Rectangle& r) {
	r.xmin = std::max(0, r.xmin); r.xmax = std::min(r.xmax, extResData.metalMapW);
	r.zmin = std::max(0, r.zmin); r.zmax = std::min(r.zmax, extResData.metalMapH);

	// set to 1 for full accuracy
	// (but longer loading times)
	const int xstep = 2;
	const int zstep = 2;

	for (int x = r.xmin; x < r.xmax; x += xstep) {
		for (int z = r.zmin; z < r.zmax; z += zstep) {
			const int xminOff = std::min(                       x,     extResData.extRad);
			const int zminOff = std::min(                       z,     extResData.extRad);
			const int xmaxOff = std::min(extResData.metalMapW - x - 1, extResData.extRad);
			const int zmaxOff = std::min(extResData.metalMapH - z - 1, extResData.extRad);

			// integrated value of an extractor
			// placed at (x, z) on the metal map
			float extIntVal = 0.0f;

			for (int i = -xminOff; i <= xmaxOff; i++) {
				for (int j = -zminOff; j <= zmaxOff; j++) {
					assert((x + i >= 0) && (x + i < extResData.metalMapW));
					assert((z + j >= 0) && (z + j < extResData.metalMapH));

					const int iSq = i * i;
					const int jSq = j * j;

					// note: extractor UnitDefs might
					// have the extractSquare tag set
					if ((iSq + jSq) > extResData.extRadSq) {
						continue;
					}

					// for the metal-map, a pixel value <v> means "this square
					// produces <v> * the map's metal-scale amount of metal" if
					// claimed by an extractor
					const int   metalMapIdx = (z + j) * extResData.metalMapW + (x + i);
					const float metalPxlVal = extResData.rawMetalMap[metalMapIdx];

					extIntVal += (extResData.extScale * metalPxlVal);
				}
			}

			// this number multiplied by an extractor's depth
			// ("extractsMetal") yields its in-game income per
			// team SlowUpdate()
			// all extractors have the same radius as returned
			// by AICallback::GetExtractorRadius() (per-unitdef
			// "extractRange" gets set to map.extractorRadius)
			extResData.intMetalMap[z * extResData.metalMapW + x] = extIntVal;

			// save the indices of any positions
			// with non-zero integration values
			if (extIntVal > 0.0f) {
				extResData.extPosIndices.insert(z * extResData.metalMapW + x);
			}
		}
	}
}



void XAICExtractableResourceFinder::FindResources() {
	XAICScopedTimer t("[XAICExtractableResourceFinder::FindResources]", xaih->timer);

	extResources.clear();

	if (!ReadExtractorCache()) {
		FindExtractorPositions();
		WriteExtractorCache();
	}

	// make sure the next GetResources() call
	// refreshes the resources pointer list
	valid = false;
}

std::list<XAIIResource*>& XAICExtractableResourceFinder::GetResources(bool refresh) {
	if (refresh) {
		FindResources();
	}

	if (!valid) {
		resources.clear();

		for (std::list<XAIExtractableResource>::iterator it = extResources.begin(); it != extResources.end(); it++) {
			resources.push_back(&(*it));
		}

		valid = true;
	}

	return resources;
}
