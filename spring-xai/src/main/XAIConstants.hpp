#ifndef XAI_CONSTANTS_HDR
#define XAI_CONSTANTS_HDR

// map coordinate-space conversion macros
//
// height-map is one-eight the size of the "world-map"
#define WORLD2HEIGHT(size) ((size) >> 3)
#define HEIGHT2WORLD(size) ((size) << 3)
// metal-map is one-half the size of the height-map
#define METAL2HEIGHT(size) ((size) << 1)
#define HEIGHT2METAL(size) ((size) >> 1)
// slope-map is one-half the size of the height-map
#define SLOPE2HEIGHT(size) ((size) << 1)
#define HEIGHT2SLOPE(size) ((size) >> 1)
// threat-map is one-eight the size of the height-map
#define THREATMAP_RESOLUTION 3
#define HEIGHT2THREAT(size) ((size) >> THREATMAP_RESOLUTION)
#define THREAT2HEIGHT(size) ((size) << THREATMAP_RESOLUTION)
#define WORLD2THREAT(size)  HEIGHT2THREAT(WORLD2HEIGHT(size))
#define THREAT2WORLD(size)  THREAT2HEIGHT(HEIGHT2WORLD(size))

// auxiliaries
#define WORLD2METAL(size) HEIGHT2METAL(WORLD2HEIGHT(size))
#define METAL2WORLD(size) HEIGHT2WORLD(METAL2HEIGHT(size))
#define WORLD2SLOPE(size) HEIGHT2SLOPE(WORLD2HEIGHT(size))
#define SLOPE2WORLD(size) HEIGHT2WORLD(SLOPE2HEIGHT(size))


// factory build-placement stuff
#define QUADRANT_TOP_LEFT   0
#define QUADRANT_TOP_RIGHT  1
#define QUADRANT_BOT_RIGHT  2
#define QUADRANT_BOT_LEFT   3
#define FACING_DOWN         0
#define FACING_RIGHT        1
#define FACING_UP           2
#define FACING_LEFT         3


#define TEAM_SU_INT_I 32
#define TEAM_SU_INT_F float(TEAM_SU_INT_I)


// defined in GlobalConstants
#define MAX_FEATURES (MAX_UNITS * 2)


#endif
