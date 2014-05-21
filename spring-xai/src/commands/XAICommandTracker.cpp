#include <fstream>
#include <sstream>

#include "Sim/Units/CommandAI/Command.h"

#include "./XAICommandTracker.hpp"
#include "../utils/XAILogger.hpp"

void XAICCommandTracker::WriteLog() {
	std::stringstream ss;
	std::map<int, int>::const_iterator it;

	ss << "[frame number, commands per frame]\n";
	ss << "\n";

	for (it = cmdsByFrame.begin(); it != cmdsByFrame.end(); it++) {
		ss << (it->first) << "\t" << (it->second) << "\n";
	}

	ss << "\n";
	ss << "[command ID, commands per ID]\n";
	ss << "\n";

	for (it = cmdsByID.begin(); it != cmdsByID.end(); it++) {
		ss << (it->first) << "\t" << (it->second) << "\n";
	}

	log << ss.str();
	log.flush();
}



void XAICCommandTracker::Track(Command* c, unsigned int f) {
	if (cmdsByFrame.find(f) == cmdsByFrame.end()) {
		cmdsByFrame[f] = 1;
	} else {
		cmdsByFrame[f] += 1;
	}

	if (cmdsByID.find(c->id) == cmdsByID.end()) {
		cmdsByID[c->id] = 1;
	} else {
		cmdsByID[c->id] += 1;
	}

	if (cmdsByFrame[f] > maxCmdsByFrame) {
		maxCmdsByFrame = cmdsByFrame[f];
		peakCmdFrame   = f;
	}

	totalNumCmds += 1;
	totalCmdSize += c->params.size();
}

/*
void XAICCommandTracker::Update() {
	if ((logFrame % 1800) == 0 && !cmdsByFrame.empty()) {
		const int   numRegFrames     = cmdsByFrame.size();
		const float avgCmdsRegFrames = totalNumCmds / float(numRegFrames);
		const float avgCmdsAllFrames = totalNumCmds / float(f);
		const float avgCmdSize       = totalCmdSize / float(totalNumCmds);

		std::stringstream msg;
			msg << "[CCommandTracker::Update()]\n";
			msg << "\tcurrent frame:                                  " << logFrame         << "\n";
			msg << "\tnumber of frames registered:                    " << numRegFrames     << "\n";
			msg << "\t(avg.) number of commands (registered frames):  " << avgCmdsRegFrames << "\n";
			msg << "\t(avg.) number of commands (all elapsed frames): " << avgCmdsAllFrames << "\n";
			msg << "\t(avg.) number of parameters per command:        " << avgCmdSize       << "\n";
			msg << "\t(max.) number of commands, peak frame:          "
				<< maxCmdsByFrame << ", "
				<< peakCmdFrame   << "\n";

		log << msg;
	}
}
*/
