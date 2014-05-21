#ifndef XAI_GROUP_HDR
#define XAI_GROUP_HDR

#include <map>

#include "System/float3.h"

struct Command;
struct XAIHelper;
class XAICUnit;
struct XAIUnitDef;
struct XAIITask;
struct XAIGroup {
public:
	XAIGroup(int gid, XAIHelper* h): id(gid), xaih(h), age(0), task(0) {
		centerPos      = ZeroVector;
		buildDist      = 1e9f;
		losDist        = 0.0f;
		minWeaponRange = 1e9f;
		maxWeaponRange = 0.0f;
		moveSpeed      = 1e9f;
		buildSpeed     = 0.0f;
		reclaimSpeed   = 0.0f;
		repairSpeed    = 0.0f;
		mtlCost        = 0.0f;
		nrgCost        = 0.0f;
		pathType       = -1;
		power          = 0.0f;
		isMobile       = true;
		isAttacker     = false;
		isBuilder      = false;
	}
	~XAIGroup();

	bool CanAddUnitMember(XAICUnit*);
	void AddUnitMember(XAICUnit*);
	void DelUnitMember(XAICUnit*);
	bool HasUnitMember(const XAICUnit*) const;
	void UnitIdle(int) const;

	// check if another group can be merged into this one
	bool CanMergeIdleGroup(XAIGroup*);
	// merge other group into this one
	void MergeIdleGroup(XAIGroup*);

	void SetTaskPtr(XAIITask* t) { task = t; }
	XAIITask* GetTaskPtr() const { return task; }

	const std::map<int, XAICUnit*>& GetUnitMembers() const { return unitsByID; }
	XAICUnit* GetLeadUnitMember() const { return ((unitsByID.begin())->second); }
	int GetUnitCount() const { return (unitsByID.size()); }
	int GetID() const { return id; }
	int GetCommandID() const;
	int GetCommandAuxID() const;
	unsigned int GetAge() const { return age; }

	float3 GetPos() const { return centerPos; }
	float GetBuildDist() const { return buildDist; }
	float GetLOSDist() const { return losDist; }
	float GetMinWeaponRange() const { return minWeaponRange; }
	float GetMaxWeaponRange() const { return maxWeaponRange; }
	float GetMaxMoveSpeed() const { return moveSpeed; }
	float GetBuildSpeed() const { return buildSpeed; }
	float GetReclaimSpeed() const { return reclaimSpeed; }
	float GetRepairSpeed() const { return repairSpeed; }
	float GetMtlCost() const { return mtlCost; }
	float GetNrgCost() const { return nrgCost; }
	int GetPathType() const { return pathType; }
	float GetPower() const { return power; }
	bool IsMobile() const { return isMobile; }
	bool IsAttacker() const { return isAttacker; }
	bool IsBuilder() const { return isBuilder; }
	float GetPositionETA(const float3&) const;

	bool Update(bool incAge = true);

	bool CanGiveCommand(int) const;
	int GiveTaskCommand(XAICUnit*) const;
	int GiveCommand(const Command&) const;
	int GiveCommand(int, XAIGroup*) const;

private:
	const int id;
	unsigned int age;

	float3 centerPos;
	float  buildDist;
	float  losDist;
	float  minWeaponRange;
	float  maxWeaponRange;
	float  moveSpeed;
	float  buildSpeed;
	float  reclaimSpeed;
	float  repairSpeed;
	float  mtlCost;
	float  nrgCost;
	int    pathType;
	float  power;
	bool   isMobile;
	bool   isAttacker; // if true, group has at least attack-capable unit
	bool   isBuilder;  // if true, group has at least build-capable unit

	// current task that this group is executing
	XAIITask* task;
	std::map<int, XAICUnit*> unitsByID;

	XAIHelper* xaih;
};

// struct XAIBuilderGroup: public XAIIGroup {};
// struct XAIAttackerGroup: public XAIIGroup {};

#endif
