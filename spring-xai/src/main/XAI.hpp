#ifndef XAI_HDR
#define XAI_HDR

#include "LegacyCpp/IGlobalAI.h"

struct XAIHelper;
class IGlobalAICallback;
class XAI: public IGlobalAI {
public:
	XAI();
	~XAI();

	void InitAI(IGlobalAICallback*, int);
	void ReleaseAI();

	void UnitCreated(int, int);
	void UnitFinished(int);
	void UnitDestroyed(int, int);
	void UnitIdle(int);
	void UnitDamaged(int, int, float, float3);
	void EnemyDamaged(int, int, float, float3);
	void UnitMoveFailed(int);

	void EnemyEnterLOS(int);
	void EnemyLeaveLOS(int);
	void EnemyEnterRadar(int);
	void EnemyLeaveRadar(int);
	void EnemyDestroyed(int, int);

	void GotChatMessage(const char*, int);
	void GotLuaMessage(const char*, const char**);
	int HandleEvent(int, const void*);

	void Update();

private:
	unsigned int xaiInstance;
	static unsigned int xaiInstances;

	XAIHelper* xaiHelper;
};

#endif
