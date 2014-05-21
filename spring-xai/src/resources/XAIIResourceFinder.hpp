#ifndef XAI_IRESOURCEFINDER_HDR
#define XAI_IRESOURCEFINDER_HDR

#include <list>
#include <set>
#include <vector>

#include "Sim/Misc/GlobalConstants.h"

#include "./XAIIResource.hpp"
#include "../main/XAIConstants.hpp"
#include "../events/XAIIEventReceiver.hpp"
#include "../events/XAIIEvent.hpp"

// the finders need to act on the INIT event
struct XAIHelper;
class XAIIResourceFinder: public XAIIEventReceiver {
public:
	XAIIResourceFinder(XAIHelper* h): xaih(h), valid(false), autoInit(false) {
	}
	virtual std::list<XAIIResource*>& GetResources(bool) {
		return resources;
	}

protected:
	std::list<XAIIResource*> resources;
	XAIHelper* xaih;
	bool valid;
	bool autoInit;
};



class XAICExtractableResourceFinder: public XAIIResourceFinder {
public:
	XAICExtractableResourceFinder(XAIHelper* h, bool b): XAIIResourceFinder(h) {
		autoInit = b;
	}

	void OnEvent(const XAIIEvent* e) {
		// note: request a removal from event
		// handler's receiver list after this?
		if (e->type == XAI_EVENT_INIT && autoInit) {
			FindResources();
		}
	}

	std::list<XAIIResource*>& GetResources(bool);

private:
	// this only needs to be executed once
	void FindResources();

	std::list<XAIExtractableResource> extResources;


	struct Rectangle {
	public:
		Rectangle(): xmin(0), zmin(0), xmax(0), zmax(0) {
		}

		int xmin, xmax;
		int zmin, zmax;
	};

	struct ExtractableResourceData {
	public:
		ExtractableResourceData() {
			metalMapW = 0;
			metalMapH = 0;
			metalMapA = 0;
			extRad    = 0.0f;
			extRadSq  = 0.0f;
			extScale  = 0.0f;
			avgExtVal = 0.0f;
		}

		int metalMapW;
		int metalMapH;
		int metalMapA; // area

		int   extRad;
		int   extRadSq;
		float extScale;
		float avgExtVal;

		std::vector<unsigned char> rawMetalMap;
		std::vector<float        > intMetalMap;

		std::set<int> extPosIndices;
		std::list<float> extIntValues;
	};
	ExtractableResourceData extResData;

	void FindExtractorPositions();
	bool FindExtractorPosition(XAIExtractableResource*);
	void IntegrateResourceMap(Rectangle&);
	std::string GetExtractorCacheName();
	bool ReadExtractorCache();
	void WriteExtractorCache();
};


class XAICReclaimableResourceFinder: public XAIIResourceFinder {
public:
	XAICReclaimableResourceFinder(XAIHelper* h, bool b): XAIIResourceFinder(h) {
		autoInit = b;

		uIDs.resize(MAX_UNITS);
		fIDs.resize(MAX_FEATURES);
	}

	void OnEvent(const XAIIEvent* e) {
		// note: listen to UNIT_DESTROYED
		// events to update wreckage data?
		if (e->type == XAI_EVENT_INIT && autoInit) {
			FindResources();
		}
	}

	std::list<XAIIResource*>& GetResources(bool);

private:
	// this needs to periodically re-check
	// the map due to wrecks being created
	void FindResources();

	std::list<XAIReclaimableResource> recResources;


	std::vector<int> uIDs;
	std::vector<int> fIDs;
};

#endif
