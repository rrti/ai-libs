#ifndef XAI_COMMANDTRACKER_HDR
#define XAI_COMMANDTRACKER_HDR

#include <fstream>
#include <map>

#include "../utils/XAIILogger.hpp"



struct Command;
class XAICCommandTracker: public XAIILogger {
public:
	XAICCommandTracker() {
		maxCmdsByFrame = 0;
		peakCmdFrame   = 0;
		avgCmdSize     = 0.0f;
		totalCmdSize   = 0;
		totalNumCmds   = 0;
	}
	~XAICCommandTracker() {}

	void WriteLog();
	void Track(Command*, unsigned int);

private:
	std::map<int, int> cmdsByFrame;
	std::map<int, int> cmdsByID;

	int   maxCmdsByFrame;
	int   peakCmdFrame;

	float avgCmdSize;
	int   totalCmdSize;
	int   totalNumCmds;
};

#endif
