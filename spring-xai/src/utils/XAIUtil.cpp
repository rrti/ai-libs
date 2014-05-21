#include <cassert>
#include <cstring>
#include <string>
#include <cmath>
#include <algorithm>

#include "./XAIUtil.hpp"
#ifdef BUILDING_AI
	#include "Sim/MoveTypes/MoveInfo.h"
	#include "System/float3.h"
	#include "LegacyCpp/IAICallback.h"
	#include "../main/XAIConstants.hpp"
#endif

namespace XAIUtil {
	#ifdef BUILDING_AI
	std::string GetAbsFileName(IAICallback* cb, const std::string& relFileName) {
		char        dst[1024] = {0};
		const char* src       = relFileName.c_str();
		const int   len       = relFileName.size();

		// last char ('\0') in dst
		// should not be overwritten
		assert(len < (1024 - 1));
		memcpy(dst, src, len);

		// get the absolute path to the file
		// (and create folders along the way)
		cb->GetValue(AIVAL_LOCATE_FILE_W, dst);

		return (std::string(dst));
	}
	#endif


	void StringToLowerInPlace(std::string& s) {
		std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))tolower);
	}
	std::string StringToLower(std::string s) {
		StringToLowerInPlace(s);
		return s;
	}

	std::string StringStripSpaces(const std::string& s1) {
		std::string s2; s2.reserve(s1.size());

		for (std::string::const_iterator it = s1.begin(); it != s1.end(); it++) {
			if (!isspace(*it)) {
				s2.push_back(*it);
			}
		}

		return s2;
	}


	unsigned int CountOneBits(unsigned int n) {
		const int S[] = {1, 2, 4, 8, 16};
		const int B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};
		int c = n;
		c = ((c >> S[0]) & B[0]) + (c & B[0]);
		c = ((c >> S[1]) & B[1]) + (c & B[1]);
		c = ((c >> S[2]) & B[2]) + (c & B[2]);
		c = ((c >> S[3]) & B[3]) + (c & B[3]);
		c = ((c >> S[4]) & B[4]) + (c & B[4]);
		return c;
	}

	#ifdef BUILDING_AI
	// one MoveData instance is more restrictive than
	// another if, all other properties being less or
	// equal, some property P is less than the other
	// instance's
	/*
	const MoveData* MostRestrictiveMoveDataIns(const MoveData* a, const MoveData* b) {
		const bool
			aLb0 = (a->maxSpeed      < b->maxSpeed     ), aLEb0 = (a->maxSpeed      <= b->maxSpeed     ),
			aLb1 = (a->depth         < b->depth        ), aLEb1 = (a->depth         <= b->depth        ),
			aLb2 = (a->maxSlope      < b->maxSlope     ), aLEb2 = (a->maxSlope      <= b->maxSlope     ),
			aLb3 = (a->crushStrength < b->crushStrength), aLEb3 = (a->crushStrength <= b->crushStrength);

		// ground units and ships are both
		// more restricted than hovercraft
		const bool
			aLb4 = (a->moveType == MoveData::Ground_Move && b->moveType == MoveData::Hover_Move),
			aLb5 = (a->moveType == MoveData::Ship_Move   && b->moveType == MoveData::Hover_Move),
			mtEq = (a->moveType == b->moveType);
		// land and water terrain classes are both
		// more restrictive than the mixed class
		const bool
			aLb6 = (a->terrainClass == MoveData::Land  && b->terrainClass == MoveData::Mixed),
			aLb7 = (a->terrainClass == MoveData::Water && b->terrainClass == MoveData::Mixed),
			tcEq = (a->terrainClass == b->terrainClass);

		// b0: <a> less than or equal to <b> wrt. all of {maxSpeed, depth, maxSlope, crushStrength}
		// b1: <a> less than <b> wrt. all of {moveType, terrainClass}
		// b2: <a> equal to <b> wrt. all of {moveType, terrainClass}
		// b3: <a>  less than or equal to <b> wrt. all of {moveType, terrainClass}
		const bool
			b0 = (aLEb0 && aLEb1 && aLEb2 && aLEb3),
			b1 = ((aLb4 || aLb5) && (aLb6 || aLb7)),
			b2 = (mtEq && tcEq),
			b3 = (b1 || b2);

		if (b0 && b3) {
			if (aLb0 || aLb1 || aLb2 || aLb3) { return a; }
			if (aLb4 || aLb5 || aLb6 || aLb7) { return a; }
		}

		return b;
	}
	*/

	int GetBuildFacing(IAICallback* cb, const float3& pos) {
		const int frame = cb->GetCurrentFrame();
		const int mapW = HEIGHT2WORLD(cb->GetMapWidth());
		const int mapH = HEIGHT2WORLD(cb->GetMapHeight());
		int sector = -1;
		int facing = -1;

		if (pos.x < (mapW >> 1)) {
			// left half of map
			if (pos.z < (mapH >> 1)) {
				sector = QUADRANT_TOP_LEFT;
			} else {
				sector = QUADRANT_BOT_LEFT;
			}
		} else {
			// right half of map
			if (pos.z < (mapH >> 1)) {
				sector = QUADRANT_TOP_RIGHT;
			} else {
				sector = QUADRANT_BOT_RIGHT;
			}
		}

		switch (sector) {
			case QUADRANT_TOP_LEFT: {
				facing = (frame & 1)? FACING_DOWN: FACING_RIGHT;
			} break;
			case QUADRANT_TOP_RIGHT: {
				facing = (frame & 1)? FACING_DOWN: FACING_LEFT;
			} break;
			case QUADRANT_BOT_RIGHT: {
				facing = (frame & 1)? FACING_UP: FACING_LEFT;
			} break;
			case QUADRANT_BOT_LEFT: {
				facing = (frame & 1)? FACING_UP: FACING_RIGHT;
			} break;
		}

		return facing;
	}
	#endif

	// assumes top-left corner is at (x0, z0), bottom-right at (x1, z1)
	bool PosInRectangle(const float3& p, int x0, int z0, int x1, int z1) {
		return ((p.x >= x0 && p.x <= x1) && (p.z >= z0 && p.z <= z1));
	}

	float GaussDens(float x, float mu, float sigma) {
		const float a = 1.0f / (sigma * std::sqrt(2.0f * M_PI));
		const float b = std::exp(-(((x - mu) * (x - mu)) / (2.0f * sigma * sigma)));
		return (a * b);
	}
}
