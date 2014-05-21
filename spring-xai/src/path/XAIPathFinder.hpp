#ifndef XAI_PATHFINDER_HDR
#define XAI_PATHFINDER_HDR

#include <map>
#include <vector>

class float3;
struct MoveData;
struct XAIHelper;
struct XAIIPathNode;
struct XAIGroup;
template<typename T> struct XAIMap;
template<typename T> struct XAIMaskMap;

class XAICPathFinder {
public:
	XAICPathFinder(XAIHelper*);
	~XAICPathFinder();

	bool IsPathPossible(const XAIGroup*, const float3&, const float3&) const;
private:
	XAIMap<float>* xaiHeightMap;
	XAIMap<float>* xaiSlopeMap;

	// for each moveType, this stores a mask-map which indicates
	// contiguous areas of the {H, S}map ("pixels" of equal mask)
	// that can be traversed by a unit using that pathType
	std::map<int, XAIMaskMap<float>* > maskMaps;
	std::vector<XAIIPathNode*> nodes;

	// maps path-types to MoveData instances
	std::map<int, const MoveData*> moveDataMap;

	XAIHelper* xaih;
};

#endif
