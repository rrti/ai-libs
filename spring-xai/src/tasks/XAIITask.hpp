#ifndef XAI_ITASK_HDR
#define XAI_ITASK_HDR

#include <set>

#include "Sim/Units/CommandAI/Command.h"
#include "System/float3.h"

enum XAITaskType {
	XAI_TASK_BASE        = -3,

	// NOTE: how to represent tasks that can be
	// "shared" in some sense by both the economy
	// and military handlers (such as build-tasks)?
	//
	// should assist tasks be more abstract and get
	// other TASKS as "assistees" instead of units?
	XAI_TASK_BUILD_UNIT  =  0,
	XAI_TASK_ASSIST_UNIT =  1,
	XAI_TASK_RECLAIM     =  2,

	XAI_TASK_DEFEND_UNIT =  3,
	XAI_TASK_DEFEND_AREA =  4,
	XAI_TASK_ATTACK_UNIT =  5,
	XAI_TASK_ATTACK_AREA =  6,
};


struct XAIHelper;
struct XAIGroup;
class XAICUnit;
struct XAIITask {
public:
	XAIITask(int tid, XAIHelper* h):
		type(XAI_TASK_BASE),
		id(tid),
		age(0),
		maxAge(1U << 31U),
		maxUnits(1U << 31U),
		numUnits(0U),
		started(false),
		waiting(false),
		waitTime(0),
		startFrame(0),
		xaih(h) {
	}
	virtual ~XAIITask();

	virtual void AddGroupMember(XAIGroup*);
	virtual void DelGroupMember(XAIGroup*);
	virtual bool HasGroupMember(XAIGroup*) const;
	virtual bool CanAddGroupMember(XAIGroup*) {
		return false;
	}
	virtual const std::set<XAIGroup*>& GetGroupMembers() const {
		return groups;
	}

	virtual bool Update();
	virtual void Wait(bool);
	virtual bool IsWaiting() const { return waiting; }

	virtual void SetCommand(const Command&, const Command&) {}
	virtual void SetAgeLimit(unsigned int n) { maxAge = n; }
	virtual void SetUnitLimit(unsigned int n) { maxUnits = n; }
	virtual int GetGroupCount() const { return (groups.size()); }
	virtual int GetID() const { return id; }
	virtual int GetObjectID() const { return 0; }
	virtual float GetProgress() const { return 0.0f; }
	virtual unsigned int GetAge() const { return age; }
	virtual unsigned int GetMaxAge() const { return maxAge; }
	virtual const Command& GetCommand() const { return cmd; }
	virtual const Command& GetCommandAux() const { return cmdAux; }

	virtual void UnitCreated(int, int, int) {}
	virtual void UnitDestroyed(int) {}
	virtual void UnitDamaged(int, int) {}

	XAITaskType type;

protected:
	const int id;

	// current lifetime in frames
	unsigned int age;
	unsigned int maxAge;

	// maximum number of units allowed to work
	// at once on this task across all groups
	unsigned int maxUnits;
	// current number of units across all groups that are
	// assigned to this task; this may not exceed maxUnits
	unsigned int numUnits;

	// true iif the task has started executing
	// (eg. a building nanoframe being created
	// signifies the start of a build-task)
	bool started;
	bool waiting;

	// number of frames this task has been waiting
	unsigned int waitTime;
	// frame this task was started on
	unsigned int startFrame;

	// the command that all groups attached to
	// this task should (periodically) execute 
	Command cmd;
	Command cmdAux;

	// groups of units involved in this task
	// if this ever becomes empty (or there is
	// no non-empty group left), then the task
	// aborted abnormally
	std::set<XAIGroup*> groups;

	XAIHelper* xaih;
};

/*
struct XAIIEconomyTask: public XAIITask { type = XAI_ECO_TASK_BASE; };
struct XAIIMilitaryTask: public XAIITask { type = XAI_MIL_TASK_BASE; };
*/



struct XAIAttackTask: public XAIITask {
public:
	XAIAttackTask(int tid, XAIHelper* h): XAIITask(tid, h), tAttackeeUnitID(-1) {
		type = XAI_TASK_ATTACK_UNIT;

		tAttackProgress = 0.0f;
	}

	void SetCommand(const Command& c, const Command& cAux) {
		cmd    = c;
		cmdAux = cAux;

		assert(!c.params.empty());
		tAttackeeUnitID = int(c.params[0]);
	}

	void AddGroupMember(XAIGroup*);
	bool CanAddGroupMember(XAIGroup*);

	bool Update();
	float GetProgress() const { return tAttackProgress; }
	int GetObjectID() const { return tAttackeeUnitID; }

private:
	int tAttackeeUnitID;
	float tAttackProgress;
};

struct XAIDefendTask: public XAIITask {
public:
	XAIDefendTask(int tid, XAIHelper* h): XAIITask(tid, h), tDefendeeUnitID(-1) {
		type = XAI_TASK_DEFEND_UNIT;
	}

	void SetCommand(const Command& c, const Command& cAux) {
		cmd    = c;
		cmdAux = cAux;

		assert(!c.params.empty());
		tDefendeeUnitID = int(c.params[0]);
	}

	void AddGroupMember(XAIGroup*);
	bool CanAddGroupMember(XAIGroup*);

	bool Update();
	int GetObjectID() const { return tDefendeeUnitID; }

	void UnitDestroyed(int);

private:
	int tDefendeeUnitID;
};




struct XAIBuildTask: public XAIITask {
public:
	XAIBuildTask(int tid, XAIHelper* h): XAIITask(tid, h), tBuildeeUnitID(-1), tBuilderUnitID(-1) {
		type = XAI_TASK_BUILD_UNIT;

		tBuildProgress = 0.0f;
		tBuildSpeed    = 0.0f;
		tBuildTime     = 0.0f;
	}

	void SetCommand(const Command& c, const Command& cAux) {
		cmd    = c;
		cmdAux = cAux;
	}

	void AddGroupMember(XAIGroup*);
	bool CanAddGroupMember(XAIGroup*);

	bool Update();
	float GetProgress() const { return tBuildProgress; }
	int GetObjectID() const { return tBuildeeUnitID; }

	void UnitCreated(int, int, int);
	void UnitDestroyed(int);
	void UnitDamaged(int, int);

private:
	// the ID of the unit we are building
	// this is initially unknown, the task
	// handler notifies us of it
	int tBuildeeUnitID;
	// ID of the unit initiating construction
	int tBuilderUnitID;

	// progress percentage in [0, 1]
	float tBuildProgress;
	float tBuildSpeed;
	float tBuildTime;
};

struct XAIReclaimTask: public XAIITask {
public:
	XAIReclaimTask(int tid, XAIHelper* h): XAIITask(tid, h), tReclaimeeID(-1) {
		type = XAI_TASK_RECLAIM;

		tReclaimProgress = 0.0f;
		tReclaimSpeed    = 0.0f;
	}

	void SetCommand(const Command& c, const Command& cAux) {
		cmd    = c;
		cmdAux = cAux;

		if (c.params.size() == 1) {
			// reclaim by unit- or feature-ID
			tReclaimeeID = int(c.params[0]);
		} else {
			// area reclaim
			tReclaimPos.x = c.params[0];
			tReclaimPos.y = c.params[1];
			tReclaimPos.z = c.params[2];
			tReclaimRad   = c.params[3];
		}
	}

	void AddGroupMember(XAIGroup*);
	bool CanAddGroupMember(XAIGroup*);

	bool Update();
	float GetProgress() const { return tReclaimProgress; }
	int GetObjectID() const { return tReclaimeeID; }

private:
	int tReclaimeeID;

	float3 tReclaimPos;
	float  tReclaimRad;

	float tReclaimProgress;
	float tReclaimSpeed;
};



struct XAIIAssistTask: public XAIITask {
public:
	XAIIAssistTask(int tid, XAIHelper* h): XAIITask(tid, h), tAssisteeUnitID(-1) {
		type = XAI_TASK_ASSIST_UNIT;
	}

	virtual bool CanAssistNow(const XAIGroup*, bool) { return false; }
	virtual int GetObjectID() const { return tAssisteeUnitID; }

protected:
	int tAssisteeUnitID;
};

struct XAIBuildAssistTask: public XAIIAssistTask {
public:
	XAIBuildAssistTask(int tid, XAIHelper* h): XAIIAssistTask(tid, h) {
		tAssistSpeed = 0.0f;
	}

	void SetCommand(const Command& c, const Command& cAux) {
		cmd    = c;
		cmdAux = cAux;

		assert(!c.params.empty());
		tAssisteeUnitID = int(c.params[0]);
	}

	bool CanAddGroupMember(XAIGroup*);
	void AddGroupMember(XAIGroup*);
	bool Update();

	bool CanAssistNow(const XAIGroup*, bool) const;
	bool AssistLimitReached(const XAICUnit*, const XAIGroup*, float*) const;

	void UnitDestroyed(int);

private:
	float tAssistSpeed;
};

/*
struct XAIAttackAssistTask: public XAIIAssistTask {
public:
	XAIAttackAssistTask(int tid, XAIHelper* h): XAIIAssistTask(tid, h) {
	}
};
*/

#endif
