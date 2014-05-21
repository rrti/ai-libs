#ifndef XAI_MASKMAP_HDR
#define XAI_MASKMAP_HDR

#include <cassert>
#include <map>
#include <list>
#include <vector>

#include "Sim/MoveTypes/MoveInfo.h"
#include "./XAIMap.hpp"

template<typename T> struct XAIIMapPixelFilter {
public:
	virtual bool operator () (const XAIMap<T>*, const XAIMapPixel<T>*) {
		return false;
	}
};

template<typename T> struct XAIMapPixelLandWaterFilter: public XAIIMapPixelFilter<T> {
public:
	bool operator () (const XAIMap<T>* pxlMap, const XAIMapPixel<T>* pxl) {
		switch (pxlMap->GetType()) {
			case XAI_HEIGHT_MAP: {
				return (pxl->GetValue() > T(0));
			} break;
			default: {
			} break;
		}

		return false;
	}
};

template<typename T> struct XAIMapPixelHeightSlopeFilter: public XAIIMapPixelFilter<T> {
public:
	XAIMapPixelHeightSlopeFilter(const MoveData* mdp): md(mdp) {
	}

	// returns true iif a pixel is traversable wrt. this filter's MoveData
	bool operator () (const XAIMap<T>* pxlMap, const XAIMapPixel<T>* pxl) {
		switch (pxlMap->GetType()) {
			case XAI_HEIGHT_MAP: {
				if (md->moveType == MoveData::Ship_Move  ) { return (pxl->GetValue() < -md->depth); }
				if (md->moveType == MoveData::Ground_Move) { return (pxl->GetValue() > -md->depth); }
				if (md->moveType == MoveData::Hover_Move ) { return true; }
			} break;
			case XAI_SLOPE_MAP: {
				if (md->moveType == MoveData::Ship_Move  ) { return true; }
				if (md->moveType == MoveData::Ground_Move) { return (pxl->GetValue() < md->maxSlope); }
				if (md->moveType == MoveData::Hover_Move ) { return (pxl->GetValue() < md->maxSlope); }
			} break;
			default: {
			} break;
		}

		return false;
	}

private:
	const MoveData* md;
};

#define APPLY_FILTER(f, m, p) ((*f)(m, p))



template<typename T> struct XAIMaskMapZone {
public:
	typedef std::list<const XAIMapPixel<T>* > MapPixelList;

	XAIMaskMapZone(): xpos(-1), ypos(-1), size(-1), state(false) {
	}

	XAIMaskMapZone(int x, int y, bool b, const MapPixelList& pxls): xpos(x), ypos(y), state(b) {
		pixels = pxls; size = pixels.size();
	}

	int GetPosX() const { return xpos; }
	int GetPosY() const { return ypos; }
	int GetSize() const { return size; }
	bool GetState() const { return state; }
	const MapPixelList& GetPixels() const { return pixels; }

private:
	int  xpos;  // x-coordinate of first pixel of this zone
	int  ypos;  // y-coordinate of first pixel of this zone
	int  size;  // total number of pixels in this zone
	bool state; // true if zone "passed" filter, else false

	// the 8-connected pixels making
	// up the area of this map zone
	MapPixelList pixels;
};



template<typename T> struct XAIMaskMap {
public:
	typedef std::list<const XAIMapPixel<T>* > MapPixelList; // duplicate
	typedef std::vector<const XAIMap<T>* > MapVec;
	typedef std::vector< XAIMap<int>* > MaskVec;
	typedef std::map<int, XAIMaskMapZone<T> > ZoneMap;
	typedef std::vector< ZoneMap > ZoneMapVec;

	XAIMaskMap(): mpf(0) {
	}
	~XAIMaskMap() {
		for (unsigned int i = 0; i < maps.size(); i++) {
			delete masks[i]; zones[i].clear();
		}

		maps.clear();
		masks.clear();
		zones.clear();
	}

	void AddMap(const XAIMap<T>* map) {
		maps.push_back(map);
	}

	const MapVec& GetMaps() const { return maps; }
	const MaskVec& GetMasks() const { return masks; }

	void SetMapPixelFilter(XAIIMapPixelFilter<T>* mpFilter) {
		mpf = mpFilter;
	}

	void GenerateMasks() {
		assert(mpf != NULL);

		// for each attached map, create a mask-map
		for (unsigned int i = 0; i < maps.size(); i++) {
			const XAIMap<T>* pxlMap = maps[i];
			const int pxlArea = pxlMap->GetSizeX() * pxlMap->GetSizeY();

			int pixelsMasked =  0;
			int zoneMask     = -1;

			// initialize the mask-pixels to -1
			XAIMap<int>* mskMap = new XAIMap<int>(pxlMap->GetSizeX(), pxlMap->GetSizeY(), zoneMask, XAI_MASK_MAP);

			masks.push_back(mskMap);
			zones.push_back(ZoneMap());

			// for each map pixel, find the contiguous
			// region (aka. "zone") that it is part of
			for (int idx = pxlArea - 1; idx >= 0; idx--) {
				const XAIMapPixel<int>* pxl = mskMap->GetConstPixel(idx); 

				if (pxl->GetValue() == -1) {
					const int x = idx % pxlMap->GetSizeX();
					const int y = idx / pxlMap->GetSizeX();

					zoneMask += 1;
					pixelsMasked += FloodFillPixel(pxlMap, mskMap, x, y, zones[zones.size() - 1], zoneMask);
				}

				if (pixelsMasked == pxlArea) {
					break;
				}
			}

			assert(pixelsMasked == pxlArea);
		}
	}

protected:
	void GetNonMaskedNeighbors(const XAIMap<T>* pm, XAIMap<int>* mm, const XAIMapPixel<T>* p, MapPixelList& q) {
		// if this pixel passes the filter, so
		// must its neighbors (and vice versa)
		// to count as part of the same region
		const bool b = APPLY_FILTER(mpf, pm, p);
		const int  x = p->GetX();
		const int  y = p->GetY();

		#define IDX(ngb) (ngb->GetY() * mm->GetSizeX() + ngb->GetX())
		#define VAL(idx) ((mm->GetPixel(idx))->GetValue())
		#define ADD(ngb) ((pm->InBounds(ngb)) && (VAL(IDX(ngb)) == -1) && (APPLY_FILTER(mpf, pm, ngb) == b))

		const XAIMapPixel<T>* ngb = NULL;
			// if coordinates are out of bounds, GetPixel() will
			// return a default-value pixel with (x, y) = (-1, -1)
			ngb = pm->GetConstPixel(x + 1, y + 1);  if (ADD(ngb)) { q.push_back(ngb); }
			ngb = pm->GetConstPixel(x    , y + 1);  if (ADD(ngb)) { q.push_back(ngb); }
			ngb = pm->GetConstPixel(x - 1, y + 1);  if (ADD(ngb)) { q.push_back(ngb); }
			ngb = pm->GetConstPixel(x + 1, y    );  if (ADD(ngb)) { q.push_back(ngb); }
			ngb = pm->GetConstPixel(x - 1, y    );  if (ADD(ngb)) { q.push_back(ngb); }
			ngb = pm->GetConstPixel(x + 1, y - 1);  if (ADD(ngb)) { q.push_back(ngb); }
			ngb = pm->GetConstPixel(x    , y - 1);  if (ADD(ngb)) { q.push_back(ngb); }
			ngb = pm->GetConstPixel(x - 1, y - 1);  if (ADD(ngb)) { q.push_back(ngb); }

		#undef ADD
		#undef VAL
		#undef IDX
	}

	int FloodFillPixel(const XAIMap<T>* pxlMap, XAIMap<int>* mskMap, int x, int y, ZoneMap& zoneMap, int zoneMask) {
		MapPixelList zonePxls;
		MapPixelList pxlQueue;

		unsigned int numZonePxls = 0;

		#if (XAI_MASKMAP_DBG == 1)
		assert((pxlMap->GetConstPixel(x, y))->GetX() >= 0);
		assert((pxlMap->GetConstPixel(x, y))->GetY() >= 0);
		#endif

		// initialize the queue with the pixel at <x, y>
		pxlQueue.push_front(pxlMap->GetConstPixel(x, y));

		while (!pxlQueue.empty()) {
			const XAIMapPixel<T>* pxl = pxlQueue.front();

			const int pxlX = pxl->GetX();
			const int pxlY = pxl->GetY();

			if ((mskMap->GetPixel(pxlX, pxlY))->GetValue() == -1) {
				(mskMap->GetPixel(pxlX, pxlY))->SetValue(zoneMask);

				#if (XAI_MASKMAP_DBG == 1)
				zonePxls.push_back(pxl);
				#endif

				numZonePxls += 1;

				// grow the queue with non-masked neighbors of <pxl>
				GetNonMaskedNeighbors(pxlMap, mskMap, pxl, pxlQueue);
			}

			pxlQueue.pop_front();
		}

		#if (XAI_MASKMAP_DBG == 1)
		assert(numZonePxls == zonePxls.size());

		const XAIMapPixel<T>* zonePxl = *(zonePxls.begin());
		const XAIMaskMapZone<T> mapZone(x, y, APPLY_FILTER(mpf, pxlMap, zonePxl), zonePxls);

		zoneMap[zoneMask] = mapZone;
		#endif

		// surface area in pixels of one
		// single contiguous map region
		return (numZonePxls);
	}

	MapVec maps;
	// mask-layer per attached map
	MaskVec masks;

	// zones per mask-layer per map
	ZoneMapVec zones;

	XAIIMapPixelFilter<T>* mpf;
};

#endif
