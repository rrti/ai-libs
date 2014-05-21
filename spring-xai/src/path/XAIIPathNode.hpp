#ifndef XAI_IPATHNODE_HDR
#define XAI_IPATHNODE_HDR

enum XAIPathNodeState {
	XAI_NODE_STATE_OPEN    = (1 << 0),
	XAI_NODE_STATE_CLOSED  = (1 << 1),
	XAI_NODE_STATE_DIRTY   = (1 << 2),
	XAI_NODE_STATE_BLOCKED = (1 << 3),
	XAI_NODE_STATE_START   = (1 << 4),
	XAI_NODE_STATE_GOAL    = (1 << 5),
};

struct XAIIPathNode {
public:
	XAIIPathNode(int nid): id(nid) {
		Reset();
	}
	virtual ~XAIIPathNode() {
	}

	bool operator < (const XAIIPathNode* n) const { return ((g + h) < (n->g + n->h)); }
	bool operator > (const XAIIPathNode* n) const { return ((g + h) > (n->g + n->h)); }

	/*
	bool operator () (const XAIIPathNode* a, const XAIIPathNode* b) const {
		return ((a->g + a->h) > (b->g + b->h));
	}
	*/

	bool operator == (const XAIIPathNode* n) const {
		return (id == n->id);
	}
	bool operator != (const XAIIPathNode* n) const {
		return (id != n->id);
	}

	void Reset() {
		state = XAI_NODE_STATE_OPEN;
		g     = 0.0f;
		h     = 0.0f;
		w     = 0.0f;
	}

	bool IsOpen()    const { return (state & XAI_NODE_STATE_OPEN   ); }
	bool IsClosed()  const { return (state & XAI_NODE_STATE_CLOSED ); }
	bool IsDirty()   const { return (state & XAI_NODE_STATE_DIRTY  ); }
	bool IsBlocked() const { return (state & XAI_NODE_STATE_BLOCKED); }
	bool IsStart()   const { return (state & XAI_NODE_STATE_START  ); }
	bool IsGoal()    const { return (state & XAI_NODE_STATE_GOAL   ); }

	float GetG() const { return g; }
	float GetH() const { return h; }
	float GetW() const { return w; }

protected:
	int id;

	XAIPathNodeState state;

	float g; // G-component of f(x) = g(x) + h(x) [act. cost from start to current node]
	float h; // H-component of f(x) = g(x) + h(x) [est. cost from current node to goal]
	float w; // weight (ex. due to threat)
};

#endif
