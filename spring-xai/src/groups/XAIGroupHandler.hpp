#ifndef XAI_GROUPHANDLER_HDR
#define XAI_GROUPHANDLER_HDR

#include <list>
#include <map>

#include "../events/XAIIEventReceiver.hpp"
#include "../tasks/XAIITask.hpp"

struct XAIGroup;
struct XAIIEvent;
struct XAIHelper;

class XAICGroupHandler: public XAIIEventReceiver {
public:
	XAICGroupHandler(XAIHelper* h): numGroupIDs(0), xaih(h) {
	}

	XAIGroup* NewGroup();
	void AddGroup(XAIGroup*);
	void DelGroup(XAIGroup*);
	bool HasGroup(XAIGroup*) const;

	std::list<XAIGroup*> GetTaskedGroups() const;
	std::list<XAIGroup*> GetNonTaskedGroups() const;
	std::list<XAIGroup*> GetGroupsForTask(XAITaskType) const;

	void OnEvent(const XAIIEvent*);

private:
	void Update();

	int numGroupIDs;
	std::map<int, XAIGroup*> groupsByID;
	XAIHelper* xaih;
};

#endif
