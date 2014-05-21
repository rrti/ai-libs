#ifndef XAI_UTIL_HDR
#define XAI_UTIL_HDR

#include <set>

struct MoveData;
class float3;
class IAICallback;
namespace XAIUtil {
	std::string GetAbsFileName(IAICallback*, const std::string&);

	void StringToLowerInPlace(std::string&);
	std::string StringToLower(std::string);
	std::string StringStripSpaces(const std::string&);

	template<typename T> std::set<T> IntersectSets(const std::set<T>& s1, const std::set<T>& s2) {
		typename std::set<T> r;
		typename std::set<T>::const_iterator sit;

		for (sit = s1.begin(); sit != s1.end(); sit++) {
			if (s2.find(*sit) != s2.end()) {
				r.insert(*sit);
			}
		}

		return r;
	}
	template<typename T> std::set<T> UnionSets(const std::set<T>& s1, const std::set<T>& s2) {
		typename std::set<T> r;
		typename std::set<T>::const_iterator sit;

		for (sit = s1.begin(); sit != s1.end(); sit++) { r.insert(*sit); }
		for (sit = s2.begin(); sit != s2.end(); sit++) { r.insert(*sit); }

		return r;
	}

	unsigned int CountOneBits(unsigned int);

	// const MoveData* MostRestrictiveMoveDataIns(const MoveData*, const MoveData*);

	int GetBuildFacing(IAICallback*, const float3&);
	bool PosInRectangle(const float3&, int x0, int z0, int x1, int z1);
	float GaussDens(float, float, float);
}

#endif
