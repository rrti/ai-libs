#ifndef XAI_HELPER_HDR
#define XAI_HELPER_HDR

class IGlobalAICallback;
class IAICallback;
class IAICheats;

class XAICLogger;
class XAICTimer;
class XAICCommandTracker;
class XAICEventHandler;
class XAICUnitDefHandler;
class XAICEventLogger;
class XAICUnitHandler;
class XAIIResourceFinder;
class XAICUnitDGunControllerHandler;
class XAIITaskHandler;
class XAICGroupHandler;
class XAICStateTracker;
class XAICPathFinder;
struct XAIThreatMap;
struct XAITaskListsParser;

class RNGint32;
class RNGflt64;

struct LuaParser;
struct lua_State;

struct XAIHelper {
public:
	XAIHelper(): haveConfig(false), initFrame(0), currFrame(0) {
		rcb = NULL;
		ccb = NULL;

		logger         = NULL;
		timer          = NULL;
		cmdTracker     = NULL;
		eventHandler   = NULL;
		unitDefHandler = NULL;
		eventLogger    = NULL;
		unitHandler    = NULL;
		extResFinder   = NULL;
		recResFinder   = NULL;
		dgunConHandler = NULL;
		ecoTaskHandler = NULL;
		milTaskHandler = NULL;
		groupHandler   = NULL;
		stateTracker   = NULL;
		pathFinder     = NULL;
		threatMap      = NULL;

		irng = NULL;
		frng = NULL;

		luaParser = NULL;
		luaState  = NULL;
	}

	void Init(IGlobalAICallback*);
	void InitLua();
	void Release();

	void SetConfig(bool b) { haveConfig = b; }
	bool GetConfig() const { return haveConfig; }
	void SetInitFrame(unsigned int f) { initFrame = f; }
	void SetCurrFrame(unsigned int f) { currFrame = f; }
	unsigned int GetInitFrame() const { return initFrame; }
	unsigned int GetCurrFrame() const { return currFrame; }

	IAICallback* rcb;   // regular callback handler
	IAICheats*   ccb;   // cheat callback handler

	XAICLogger*                    logger;
	XAICTimer*                     timer;
	XAICCommandTracker*            cmdTracker;
	XAICEventHandler*              eventHandler;
	XAICUnitDefHandler*            unitDefHandler;

	XAICEventLogger*               eventLogger;
	XAICUnitHandler*               unitHandler;
	XAIIResourceFinder*            extResFinder;
	XAIIResourceFinder*            recResFinder;
	XAICUnitDGunControllerHandler* dgunConHandler;
	XAIITaskHandler*               ecoTaskHandler;
	XAIITaskHandler*               milTaskHandler;
	XAICGroupHandler*              groupHandler;
	XAICStateTracker*              stateTracker;
	XAICPathFinder*                pathFinder;
	XAIThreatMap*                  threatMap;
	XAITaskListsParser*            taskListsParser;

	RNGint32* irng;
	RNGflt64* frng;

	LuaParser* luaParser;
	lua_State* luaState;

private:
	bool haveConfig;
	unsigned int initFrame;
	unsigned int currFrame;
};

#endif
