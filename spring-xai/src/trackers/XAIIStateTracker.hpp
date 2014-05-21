#ifndef XAI_ISTATETRACKER_HDR
#define XAI_ISTATETRACKER_HDR

#include <set>
#include <map>

#include "../events/XAIIEventReceiver.hpp"
#include "../resources/XAIIResource.hpp"
#include "../tasks/XAITaskRequest.hpp"

struct XAIIEvent;
struct XAIUnitDef;
struct XAIHelper;
class XAIIStateTracker {
public:
	XAIIStateTracker(XAIHelper* h): xaih(h) {}
	virtual ~XAIIStateTracker() {}

	virtual void Update() {}
	virtual XAITaskRequestQueue& GetTaskRequests() { return taskRequests; }

protected:
	XAIHelper* xaih;

	XAITaskRequestQueue taskRequests;
};



class XAICEconomyTaskHandler;
class XAICEconomyStateTracker: public XAIIStateTracker {
public:
	XAICEconomyStateTracker(XAIHelper*);

	void Update();
	bool CanAffordNow(const XAIUnitDef*, float, float, bool, bool) const;

	const XAIResourceState& GetResourceState(char c) const {
		static XAIResourceState s;
		const std::map<char, XAIResourceState>::const_iterator it = resourceStates.find(c);

		if (it != resourceStates.end()) {
			return (it->second);
		}

		return s;
	}

	bool WantMobileBuilder() const { return wantMobileBuilder; }
	bool WantStaticBuilder() const { return wantStaticBuilder; }

	float GetAvgWindStrength() const { return avgWndStren; }
	float GetAvgTidalStrength() const { return avgTdlStren; }

private:
	void ToggleResourceProducers(const XAICEconomyTaskHandler*, int);
	void InsertTaskRequests(const XAICEconomyTaskHandler*, int);

	std::map<char, XAIResourceState> resourceStates;

	bool wantMobileBuilder;
	bool wantStaticBuilder;

	float avgWndStren;
	float avgTdlStren;
};

// should this maintain the threat-map?
class XAICMilitaryStateTracker: public XAIIStateTracker {
public:
	XAICMilitaryStateTracker(XAIHelper* h): XAIIStateTracker(h) {
	}

	void Update() {}
};






struct XAICStateTracker: public XAIIEventReceiver {
public:
	XAICStateTracker(XAIHelper* h): xaih(h) {
	}

	void OnEvent(const XAIIEvent*);

	XAIIStateTracker* GetEcoState() { return ecoStateTracker; }
	XAIIStateTracker* GetMilState() { return milStateTracker; }

	XAITaskRequestQueue& GetTaskRequests() { return jointTaskRequests; }

private:
	void Update();

	XAIIStateTracker* ecoStateTracker;
	XAIIStateTracker* milStateTracker;
	XAIHelper* xaih;

	XAITaskRequestQueue jointTaskRequests;
};

#endif
