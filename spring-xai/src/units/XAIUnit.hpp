#ifndef XAI_UNIT_HDR
#define XAI_UNIT_HDR

#include "System/float3.h"

struct Command;
struct XAIUnitDef;
struct XAIITask;
struct XAIGroup;
struct XAIHelper;

class XAICUnit {
public:
	XAICUnit(int iid, XAIHelper* h):
		id(iid),
		currCmdID(0),
		active(true),
		waiting(false),
		unitDef(0),
		group(0),
		xaih(h) {
	}

	void Init();
	void Update();

	bool GetActiveState() const { return active; }
	void SetActiveState(bool);

	// called by UnitHandler; the UnitDef pointer does NOT
	// change during the lifetime of this unit (ie. between
	// UnitCreated and UnitDestroyed)
	void SetUnitDefPtr(const XAIUnitDef* def) { unitDef = def; }
	const XAIUnitDef* GetUnitDefPtr() const { return unitDef; }

	void SetGroupPtr(XAIGroup* g) { group = g; }
	XAIGroup* GetGroupPtr() const { return group; }

	const float3& GetPos() const { return pos; }
	const float3& GetVel() const { return vel; }
	const float3& GetDir() const { return dir; }

	int GetID() const { return id; }
	int GetCurrCmdID() const { return currCmdID; }
	unsigned int GetAge() const { return age; }

	bool HasCommand() const;
	bool CanGiveCommand(int) const;
	int GiveCommand(const Command&);
	int TryGiveCommand(const Command&);

	float GetPositionETA(const float3&);
	void Move(const float3&);
	void Stop();

private:
	void UpdatePosition();
	void UpdateCommand();
	void UpdateWait();

	void Wait(bool);

	const int id;
	int currCmdID;

	float3 pos;
	float3 vel;
	float3 dir;
	float spd; // length of vel

	bool active;
	bool waiting;

	unsigned int age;
	unsigned int limboTime;

	const XAIUnitDef* unitDef;

	// each unit can only ever be part of one active
	// group at the same time; the group in turn is
	// registered with some task
	XAIGroup* group;

	XAIHelper* xaih;
};

#endif
