#include <map>

// AI interface headers
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "../Wrappers/LegacyCpp/AIAI.h"

// XAI headers
#include "./XAIExports.hpp"
#include "../main/XAI.hpp"

static std::map<int, CAIAI*> aiInstances;
static std::map<int, const SSkirmishAICallback*> aiCallbacks;



EXPORT(int) init(int skirmishAIId, const SSkirmishAICallback* callback) {
	if (aiInstances.find(skirmishAIId) != aiInstances.end()) {
		return -1;
	}

	aiCallbacks[skirmishAIId] = callback;

	// CAIGlobalAI is the Legacy C++ wrapper
	aiInstances[skirmishAIId] = new CAIAI(new XAI());
	return 0;
}

// optionally called
EXPORT(int) release(int skirmishAIId) {
	if (aiInstances.count(skirmishAIId) == 0) {
		return -1;
	}

	delete aiInstances[skirmishAIId];
	aiInstances.erase(skirmishAIId);

	return 0;
}

EXPORT(int) handleEvent(int skirmishAIId, int topic, const void* data) {
	if (aiInstances.find(skirmishAIId) != aiInstances.end()) {
		return aiInstances[skirmishAIId]->handleEvent(topic, data);
	}

	return -1;
}



///////////////////////////////////////////////////////
// methods from here on are for AI internal use only //
///////////////////////////////////////////////////////
const char* aiexport_getVersion() {
	const int skirmishAIId = aiCallbacks.begin()->first;
	const SSkirmishAICallback* cb = aiCallbacks[skirmishAIId];
	const char* version = cb->SkirmishAI_Info_getValueByKey(skirmishAIId, SKIRMISH_AI_PROPERTY_VERSION);

	return version;
}
