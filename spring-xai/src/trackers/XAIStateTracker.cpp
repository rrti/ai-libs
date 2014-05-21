#include "./XAIIStateTracker.hpp"
#include "../events/XAIIEvent.hpp"

void XAICStateTracker::OnEvent(const XAIIEvent* e) {
	switch (e->type) {
		case XAI_EVENT_INIT: {
			ecoStateTracker = new XAICEconomyStateTracker(xaih);
			milStateTracker = new XAICMilitaryStateTracker(xaih);
		} break;

		case XAI_EVENT_UPDATE: {
			Update();
		} break;

		case XAI_EVENT_RELEASE: {
			delete ecoStateTracker; ecoStateTracker = 0;
			delete milStateTracker; milStateTracker = 0;
		} break;

		default: {
		} break;
	}
}

void XAICStateTracker::Update() {
	ecoStateTracker->Update();
	milStateTracker->Update();

	XAITaskRequestQueue& ecoTaskReqs = ecoStateTracker->GetTaskRequests();
	XAITaskRequestQueue& milTaskReqs = milStateTracker->GetTaskRequests();
	XAITaskRequestQueue& jntTaskReqs = jointTaskRequests;

	// merge the individual priority queues
	if (jntTaskReqs.Size() < (ecoTaskReqs.Size() + milTaskReqs.Size())) {
		while (!ecoTaskReqs.Empty() || !milTaskReqs.Empty()) {
			if (!ecoTaskReqs.Empty()) {
				jntTaskReqs.Push(ecoTaskReqs.Top());
				ecoTaskReqs.Pop();
			}
			if (!milTaskReqs.Empty()) {
				jntTaskReqs.Push(milTaskReqs.Top());
				milTaskReqs.Pop();
			}
		}
	} else {
		while (!ecoTaskReqs.Empty()) { ecoTaskReqs.Pop(); }
		while (!milTaskReqs.Empty()) { milTaskReqs.Pop(); }
	}
}
