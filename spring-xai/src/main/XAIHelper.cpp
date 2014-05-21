#include <ctime>

#include "LegacyCpp/IGlobalAICallback.h"
#include "LegacyCpp/IAICallback.h"

#include "./XAIHelper.hpp"
#include "./XAIDefines.hpp"
#include "./XAIFolders.hpp"
#include "./XAILua.hpp"
#include "../utils/XAILogger.hpp"
#include "../utils/XAITimer.hpp"
#include "../utils/XAIRNG.hpp"
#include "../utils/XAIUtil.hpp"
#include "../utils/XAILuaParser.hpp"
#include "../commands/XAICommandTracker.hpp"
#include "../events/XAIEventHandler.hpp"
#include "../events/XAIEventLogger.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../units/XAIUnitHandler.hpp"
#include "../units/XAIUnitDGunControllerHandler.hpp"
#include "../resources/XAIIResourceFinder.hpp"
#include "../tasks/XAIITaskHandler.hpp"
#include "../tasks/XAITaskListsParser.hpp"
#include "../groups/XAIGroupHandler.hpp"
#include "../trackers/XAIIStateTracker.hpp"
#include "../path/XAIPathFinder.hpp"
#include "../map/XAIThreatMap.hpp"

void XAIHelper::Init(IGlobalAICallback* gcb) {
	rcb = gcb->GetAICallback();
	ccb = gcb->GetCheatInterface();

	initFrame = rcb->GetCurrentFrame();
	currFrame = initFrame;

	logger          = new XAICLogger(rcb);
	timer           = new XAICTimer();
	cmdTracker      = new XAICCommandTracker();
	eventHandler    = new XAICEventHandler();
	unitDefHandler  = new XAICUnitDefHandler(this);

	eventLogger     = new XAICEventLogger();
	unitHandler     = new XAICUnitHandler(this);
	extResFinder    = new XAICExtractableResourceFinder(this, false);
	recResFinder    = new XAICReclaimableResourceFinder(this, false);
	dgunConHandler  = new XAICUnitDGunControllerHandler(this);
	ecoTaskHandler  = new XAICEconomyTaskHandler(this);
	milTaskHandler  = new XAICMilitaryTaskHandler(this);
	groupHandler    = new XAICGroupHandler(this);
	stateTracker    = new XAICStateTracker(this);
	pathFinder      = new XAICPathFinder(this);
	threatMap       = new XAIThreatMap(this);
	taskListsParser = new XAITaskListsParser();

	irng = new RNGint32(); irng->seedGen(time(NULL));
	frng = new RNGflt64(); frng->seedGen((*irng)());

	eventHandler->AddReceiver(threatMap,        0);
	eventHandler->AddReceiver(unitHandler,      4);
	eventHandler->AddReceiver(groupHandler,     5);
	eventHandler->AddReceiver(stateTracker,    10);
	eventHandler->AddReceiver(ecoTaskHandler,  14);
	eventHandler->AddReceiver(milTaskHandler,  15);
	eventHandler->AddReceiver(dgunConHandler,  20);
	eventHandler->AddReceiver(eventLogger,     25);
	eventHandler->AddReceiver(extResFinder,    30);
	eventHandler->AddReceiver(recResFinder,    31);

	logger->OpenLog(logger->GetLogName(NULL) + "[log].txt");
	timer->OpenLog(logger->GetLogName(NULL) + "[timings].txt");
	cmdTracker->OpenLog(logger->GetLogName(NULL) + "[trackedCmds].txt");
	eventLogger->OpenLog(logger->GetLogName(NULL) + "[trackedEvts].txt");

	unitDefHandler->OpenLog(logger->GetLogName(NULL) + "[unitDefs].txt");
	unitDefHandler->WriteLog();
	unitDefHandler->CloseLog();

	#if (XAI_USE_LUA == 1)
	InitLua();
	#endif
}

void XAIHelper::InitLua() {
	luaState = lua_open();
	luaL_openlibs(luaState);

	const std::string modName = XAIUtil::StringStripSpaces(rcb->GetModName());
	const std::string mapName = XAIUtil::StringStripSpaces(rcb->GetMapName());
	const std::string cfgName = XAI_CFG_DIR + modName + "-" + mapName + ".lua";
	const std::string cfgFile = XAIUtil::GetAbsFileName(rcb, cfgName);

	luaParser = new LuaParser(luaState);
	haveConfig = luaParser->Execute(cfgFile, "cfg");

	if (haveConfig) {
		logger->Log("[XAIHelper::InitLua] successfully parsed \"" + cfgFile + "\"");
		taskListsParser->ParseTaskLists(this);
	} else {
		logger->Log("[XAIHelper::InitLua] failed to parse \"" + cfgFile + "\": " + luaParser->GetError());
		taskListsParser->WriteDefaultTaskListsFile(this, cfgFile + ".txt");
	}
}

void XAIHelper::Release() {
	logger->CloseLog();

	timer->SetLogFrame(GetCurrFrame());
	timer->WriteLog();
	timer->CloseLog();
	cmdTracker->SetLogFrame(GetCurrFrame());
	cmdTracker->WriteLog();
	cmdTracker->CloseLog();
	eventLogger->SetLogFrame(GetCurrFrame());
	eventLogger->WriteLog();
	eventLogger->CloseLog();

	eventHandler->DelReceivers();

	delete logger;          logger          = NULL;
	delete timer;           timer           = NULL;
	delete cmdTracker;      cmdTracker      = NULL;
	delete eventHandler;    eventHandler    = NULL;
	delete unitDefHandler;  unitDefHandler  = NULL;
	delete eventLogger;     eventLogger     = NULL;
	delete unitHandler;     unitHandler     = NULL;

	delete extResFinder;    extResFinder    = NULL;
	delete recResFinder;    recResFinder    = NULL;
	delete dgunConHandler;  dgunConHandler  = NULL;
	delete ecoTaskHandler;  ecoTaskHandler  = NULL;
	delete milTaskHandler;  milTaskHandler  = NULL;
	delete groupHandler;    groupHandler    = NULL;
	delete stateTracker;    stateTracker    = NULL;
	delete pathFinder;      pathFinder      = NULL;
	delete threatMap;       threatMap       = NULL;
	delete taskListsParser; taskListsParser = NULL;

	delete irng; irng = NULL;
	delete frng; frng = NULL;

	#if (XAI_USE_LUA == 1)
	delete luaParser;    luaParser = NULL;
	lua_close(luaState); luaState  = NULL;
	#endif
}
