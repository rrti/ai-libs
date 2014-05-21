#include <sstream>

#include "LegacyCpp/IAICallback.h"

#include "./XAIIStateTracker.hpp"
#include "../events/XAIIEvent.hpp"
#include "../main/XAIHelper.hpp"
#include "../main/XAIDefines.hpp"
#include "../tasks/XAIITaskHandler.hpp"
#include "../tasks/XAIITask.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../units/XAIUnitHandler.hpp"
#include "../units/XAIUnit.hpp"
#include "../utils/XAILogger.hpp"

#include "../groups/XAIGroup.hpp"

XAICEconomyStateTracker::XAICEconomyStateTracker(XAIHelper* h): XAIIStateTracker(h) {
	wantMobileBuilder = false;
	wantStaticBuilder = false;

	resourceStates['M'] = XAIResourceState();
	resourceStates['E'] = XAIResourceState();

	avgWndStren = (h->rcb->GetMinWind() + h->rcb->GetMaxWind()) * 0.5f;
	avgTdlStren = (h->rcb->GetTidalStrength());
}

void XAICEconomyStateTracker::Update() {
	const XAICEconomyTaskHandler* eth = dynamic_cast<const XAICEconomyTaskHandler*>(xaih->ecoTaskHandler);

	XAIResourceState& mState = resourceStates['M'];
	XAIResourceState& eState = resourceStates['E'];

	if (xaih->GetCurrFrame() <= (TEAM_SU_INT_I << 1)) {
		return;
	} else if (xaih->GetCurrFrame() == ((TEAM_SU_INT_I << 1) + 1)) {
		mState.SetInitIncome(xaih->rcb->GetMetalIncome());
		eState.SetInitIncome(xaih->rcb->GetEnergyIncome());
	}

	mState.SetLevelDelta(xaih->rcb->GetMetal()  - mState.GetLevel());
	eState.SetLevelDelta(xaih->rcb->GetEnergy() - eState.GetLevel());
	mState.SetLevel(xaih->rcb->GetMetal());
	eState.SetLevel(xaih->rcb->GetEnergy());
	mState.SetStorage(xaih->rcb->GetMetalStorage());
	eState.SetStorage(xaih->rcb->GetEnergyStorage());

	if ((xaih->GetCurrFrame() % TEAM_SU_INT_I) == 0) {
		// engine updates these every 32 frames only
		// (values are also per 32 simulation frames)
		//
		// {m, e}Usage is equal to the total per-frame
		// {m, e}cost of all running constructions, etc
		mState.SetIncome(xaih->rcb->GetMetalIncome());
		eState.SetIncome(xaih->rcb->GetEnergyIncome());
		mState.SetUsage(xaih->rcb->GetMetalUsage());
		eState.SetUsage(xaih->rcb->GetEnergyUsage());
		mState.SlowUpdate();
		eState.SlowUpdate();
	} else {
		// in most cases, our income for a particular
		// resource does not change at all from frame
		// to frame (but...)
		//
		// positive delta's (income > usage) can be explained by
		//
		//    1. I  == C1 && U  == C2, where C1 > C2 (C1, C2 constants)
		//    2. I  == C  && U1 <  U2, where U1 < C  (U1, U2 current and last usage)
		//    3. I1 >= I2 && U1 == U2, where I1 > U1 (I1, I2 current and last income; U1 and U2 as above)
		//    4. I1 <  I2 && U1 <  U2, where I1 > U1 (I1, I2 and U1, U2 as above)
		//
		// negative delta's follow the same logic
		// 
		// delta's of zero can also mean we have
		// reached full storage, since level can
		// no longer increase at that point
	}

	mState.Update();
	eState.Update();

	// if these are true, then level == storage == 0
	// which should never actually happen unless the
	// AI is dead
	if (mState.HaveExcess() && mState.HaveStall()) {
		LOG_BASIC(
			xaih->logger,
			"[XAIEconomyTaskHandler::Update]" <<
			"[frame=" << xaih->GetCurrFrame() << "][mExcess && mStall (!)]" <<
			"[mLevel=" << mState.GetLevel() << ", mUsage=" << mState.GetUsage() << ", mStorage=" << mState.GetStorage() << "]";
		);
	}
	if (eState.HaveExcess() && eState.HaveStall()) {
		LOG_BASIC(
			xaih->logger,
			"[XAIEconomyTaskHandler::Update]" <<
			"[frame=" << xaih->GetCurrFrame() << "][eExcess && eStall (!)]" <<
			"[eLevel=" << eState.GetLevel() << ", eUsage=" << eState.GetUsage() << ", eStorage=" << eState.GetStorage() << "]"
		);
	}

	ToggleResourceProducers(eth, xaih->GetCurrFrame());
	InsertTaskRequests(eth, xaih->GetCurrFrame());
}

void XAICEconomyStateTracker::InsertTaskRequests(const XAICEconomyTaskHandler* eth, int frame) {
	const std::set<int>& mobileBuilderUIDs = eth->GetMobileBuilderUnitIDs();
	const std::set<int>& staticBuilderUIDs = eth->GetStaticBuilderUnitIDs();

	const XAIResourceState& mState = resourceStates['M'];
	const XAIResourceState& eState = resourceStates['E'];

	int numIdleMobileBuilders = 0;
	int numIdleStaticBuilders = 0;

	wantStaticBuilder = true;
	wantMobileBuilder = (!mobileBuilderUIDs.empty() /*&& (numIdleMobileBuilders < 2)*/);

	for (std::set<int>::const_iterator sit = mobileBuilderUIDs.begin(); sit != mobileBuilderUIDs.end(); sit++) {
		if (xaih->unitHandler->IsUnitIdle(*sit) && xaih->unitHandler->IsUnitFinished(*sit)) {
			numIdleMobileBuilders++;
		}
	}
	for (std::set<int>::const_iterator sit = staticBuilderUIDs.begin(); sit != staticBuilderUIDs.end(); sit++) {
		if (xaih->unitHandler->IsUnitIdle(*sit) && xaih->unitHandler->IsUnitFinished(*sit)) {
			numIdleStaticBuilders++;
		}

		const XAICUnit* u  = xaih->unitHandler->GetUnitByID(*sit);
		const XAIGroup* g  = u->GetGroupPtr();
		const XAIITask* bt = (g != 0)? g->GetTaskPtr(): 0;
		const XAIITask* at = 0;

		if ((bt != 0) && ((at = eth->GetStaticBuilderAssistTask(*sit)) != 0)) {
			const XAIBuildAssistTask* bat = (dynamic_cast<const XAIBuildAssistTask*>(at));

			if (bat != 0) {
				wantStaticBuilder = (wantStaticBuilder && bat->AssistLimitReached(u, NULL, NULL));
			}
		} else {
			wantStaticBuilder = false;
		}
	}

	// ensure continuous expansion
	//
	// we want a new mobile builder when
	//     all current ones are busy and economy allows for expansion
	//     but: only push request when there are some idle ones around
	// we want a new static builder (factory) when
	//     1. we don't have one (finished or under construction)
	//     2. the one(s) we have is/are running at peak capacity
	//     3. the one(s) we have are producing the wrong units?
	//
	// TODO: need to be smarter (demand-driven) about
	// all of this so the expansion is more directed
	// (dynamic priorities, random ranges [20 - 25]?)
	//
	// do not over-saturate our workforce; do not allow
	// requests to be pushed unless there are idle units
	//
	// we must restrict the size of the queue, because
	// otherwise it will just grow out of control with
	// requests (every frame that at least one builder
	// is alive); eg. during the first 64 frames

	/*
	if (stall) {
		// producer with high priority
	} else if (excess) {
		// builder?
	} else if (wantStorage) {
		// storage
	} else if (wantProducer) {
		// producer
	}
	*/
	if (numIdleMobileBuilders > 0 || numIdleStaticBuilders > 0) {
		if (mState.HaveStall()) {
			taskRequests.Push(XAITaskRequest(100, frame, true, false, "M_PRO_* [HaveStall]",    MASK_M_PRODUCER_MOBILE | MASK_M_PRODUCER_STATIC));
		} else if (mState.WantStorage()) {
			taskRequests.Push(XAITaskRequest( 50, frame, true, false, "M_STO_* [WantStorage]",  MASK_M_STORAGE_MOBILE  | MASK_M_STORAGE_STATIC ));
		} else if (mState.WantProducer()) {
			taskRequests.Push(XAITaskRequest( 25, frame, true, false, "M_PRO_* [WantProducer]", MASK_M_PRODUCER_MOBILE | MASK_M_PRODUCER_STATIC));
		}

		if (eState.HaveStall()) {
			taskRequests.Push(XAITaskRequest(100, frame, true, false, "E_PRO_* [HaveStall]",    MASK_E_PRODUCER_MOBILE | MASK_E_PRODUCER_STATIC));
		} else if (eState.WantStorage()) {
			taskRequests.Push(XAITaskRequest( 50, frame, true, false, "E_STO_* [WantStorage]",  MASK_E_STORAGE_MOBILE  | MASK_E_STORAGE_STATIC ));
		} else if (eState.WantProducer()) {
			taskRequests.Push(XAITaskRequest( 25, frame, true, false, "E_PRO_* [WantProducer]", MASK_E_PRODUCER_MOBILE | MASK_E_PRODUCER_STATIC));
		}

		if (mState.GetIncome() > 0.0f && eState.GetIncome() > 0.0f && !mState.HaveStall() && !eState.HaveStall()) {
			if (staticBuilderUIDs.empty() && ((mState.GetIncome() >= mState.GetInitIncome() * 2.0f) && (eState.GetIncome() > eState.GetInitIncome() * 1.5f))) {
				// note: 32-frame delay before income from latest completed
				// {M, E}-producer is counted means we don't get to this as
				// quickly as possible and over-build economy structures
				taskRequests.Push(XAITaskRequest(105, frame, true, false, "BUILDER_STATIC", MASK_BUILDER_STATIC));
			} else if (!staticBuilderUIDs.empty()) {
				if (!mState.HaveStall() && !eState.HaveStall()) {
					if (wantMobileBuilder) { taskRequests.Push(XAITaskRequest(35, frame, true, false, "BUILDER_MOBILE", MASK_BUILDER_MOBILE)); }
					if (wantStaticBuilder) { taskRequests.Push(XAITaskRequest(55, frame, true, false, "BUILDER_STATIC", MASK_BUILDER_STATIC)); }
				}
			}
		}
	}
}



bool XAICEconomyStateTracker::CanAffordNow(
	const XAIUnitDef* def,
	float buildProgress,
	float buildSpeed,
	bool started,
	bool waiting
) const {
	const XAIResourceState& mState = GetResourceState('M');
	const XAIResourceState& eState = GetResourceState('E');

	// buildProgress is a number in [0, 1]
	// BuildTasks track this value per-frame
	const float bp  = std::min(1.0f, std::max(buildProgress, 0.0f));
	const float btf = def->GetBuildTimeFrames(buildSpeed) * (1.0f - bp);
	const float mcf = def->FrameCost('M', buildSpeed);
	const float ecf = def->FrameCost('E', buildSpeed);

	// engine ensures costs are always at least 1.0f to prevent DIV0
	if ((mState.GetStallTime() < TEAM_SU_INT_F) && (mcf > 1.0f)) { return false; }
	if ((eState.GetStallTime() < TEAM_SU_INT_F) && (ecf > 1.0f)) { return false; }
	if (mState.HaveStall() && (mcf > 1.0f)) { return false; }
	if (eState.HaveStall() && (ecf > 1.0f)) { return false; }
	if (mcf > mState.GetLevel()) { return false; }
	if (ecf > eState.GetLevel()) { return false; }

	if ((mState.GetLevel() >= (mcf * btf)) && (eState.GetLevel() >= (ecf * btf))) {
		// tweak: make the anti-stall check more lenient
		return true;
	}

	const float
		mInc = (mState.GetIncome() / TEAM_SU_INT_F),
		mUse = (mState.GetUsage()  / TEAM_SU_INT_F) - ((started && !waiting)? mcf: 0.0f),
		eInc = (eState.GetIncome() / TEAM_SU_INT_F),
		eUse = (eState.GetUsage()  / TEAM_SU_INT_F) - ((started && !waiting)? ecf: 0.0f),
		mNet = (mInc - mUse) * btf,
		eNet = (eInc - eUse) * btf,
		mSum = (mcf * btf),
		eSum = (ecf * btf),
		mRes = (mState.GetLevel() + (mNet - mSum)),
		eRes = (eState.GetLevel() + (eNet - eSum));

	const bool b0 = ((mRes > 0.0f) || (mSum <= 1.0f));
	const bool b1 = ((eRes > 0.0f) || (eSum <= 1.0f));

	return (b0 && b1);
}



// enable or disable producers than we can or cannot afford to run
void XAICEconomyStateTracker::ToggleResourceProducers(const XAICEconomyTaskHandler* eth, int) {
	const XAIResourceState& mState = GetResourceState('M');
	const XAIResourceState& eState = GetResourceState('E');

	std::set<int> mobileProducerUIDs; eth->GetMobileProducerUnitIDs(&mobileProducerUIDs);
	std::set<int> staticProducerUIDs; eth->GetStaticProducerUnitIDs(&staticProducerUIDs);

	std::list<XAICUnit*> activeUnits;
	std::list<XAICUnit*> zombieUnits;
	std::list<XAICUnit*>::iterator lit;

	for (std::set<int>::iterator sit = mobileProducerUIDs.begin(); sit != mobileProducerUIDs.end(); sit++) {
		XAICUnit* u = xaih->unitHandler->GetUnitByID(*sit);

		if (xaih->unitHandler->IsUnitFinished(u->GetID())) {
			if (u->GetActiveState()) {
				activeUnits.push_back(u);
			} else {
				zombieUnits.push_back(u);
			}
		}
	}

	for (std::set<int>::iterator sit = staticProducerUIDs.begin(); sit != staticProducerUIDs.end(); sit++) {
		XAICUnit* u = xaih->unitHandler->GetUnitByID(*sit);

		if (xaih->unitHandler->IsUnitFinished(u->GetID())) {
			if (u->GetActiveState()) {
				activeUnits.push_back(u);
			} else {
				zombieUnits.push_back(u);
			}
		}
	}

	if ((mState.GetStallTime() <= TEAM_SU_INT_F) || (eState.GetStallTime() <= TEAM_SU_INT_F)) {
		// disable (all) active units
		// todo: de-activate as few as possible
		for (lit = activeUnits.begin(); lit != activeUnits.end(); lit++) {
			XAICUnit* u =  *lit;

			const XAIUnitDef* uDef = u->GetUnitDefPtr();
			const float mDefNet = uDef->ResMakeOff('M',        0.0f,        0.0f) + uDef->ResMakeOn('M', 0.0f);
			const float eDefNet = uDef->ResMakeOff('E', avgWndStren, avgTdlStren) + uDef->ResMakeOn('E', 0.0f);

			if ((mDefNet < 0.0f) && (mState.GetStallTime() < TEAM_SU_INT_F)) { u->SetActiveState(false); }
			if ((eDefNet < 0.0f) && (eState.GetStallTime() < TEAM_SU_INT_F)) { u->SetActiveState(false); }
		}
	} else {
		if ((mState.GetStallTime() > TEAM_SU_INT_F * 3.0f) && (eState.GetStallTime() > TEAM_SU_INT_F * 3.0f)) {
			// enable (all) inactive units
			// todo: re-activate as many as possible
			for (lit = zombieUnits.begin(); lit != zombieUnits.end(); lit++) {
				XAICUnit* u =  *lit;

				u->SetActiveState(true);
			}
		}
	}
}
