#ifndef XAI_TASKREQUESTQUEUE_HDR
#define XAI_TASKREQUESTQUEUE_HDR

#include <string>
#include <queue>

// NOTE: make more generic, so we could have
// XAIBuildTaskRequest, XAIAttackTaskRequest,
// etc?
struct XAITaskRequest {
	XAITaskRequest() {
		isEcoReq = false;
		isMilReq = false;
		reqFrame = 0;
		reqDescr = "";
		priority = 0;
		typeMask = 0;
		terrMask = 0;
		weapMask = 0;
	}
	XAITaskRequest(const XAITaskRequest& t) {
		isEcoReq = t.isEcoReq;
		isMilReq = t.isMilReq;
		reqFrame = t.reqFrame;
		reqDescr = t.reqDescr;
		priority = t.priority;
		typeMask = t.typeMask;
		terrMask = t.terrMask;
		weapMask = t.weapMask;
	}
	XAITaskRequest(int p, int f, bool er, bool mr, std::string s = "", int tym = 0, int tem = 0, int wm = 0):
		isEcoReq(er),
		isMilReq(mr),
		reqDescr(s),
		reqFrame(f),
		priority(p),
		typeMask(tym),
		terrMask(tem),
		weapMask(wm) {
	}
	bool operator < (const XAITaskRequest& other) const {
		// sort requests in decreasing order
		return (priority < other.priority);
	}

	bool isEcoReq;
	bool isMilReq;

	std::string reqDescr;

	unsigned int reqFrame;
	unsigned int priority;
	unsigned int typeMask;
	unsigned int terrMask;
	unsigned int weapMask;
};

struct XAITaskRequestQueue {
public:
	bool Empty() const { return (q.empty()); }
	size_t Size() const { return (q.size()); }

	void Push(const XAITaskRequest& t) { q.push(t); }
	void Pop() { q.pop(); }
	const XAITaskRequest& Top() { return q.top(); }

private:
	std::priority_queue<XAITaskRequest> q;
};

#endif
