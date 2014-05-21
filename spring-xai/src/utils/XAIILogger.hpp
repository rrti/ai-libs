#ifndef XAI_ILOGGER_HDR
#define XAI_ILOGGER_HDR

#include <fstream>

// logs to a file
class XAIILogger {
public:
	virtual ~XAIILogger() {}
	virtual void OpenLog(const std::string&);
	virtual void CloseLog();
	virtual void WriteLog() {}
	virtual void SetLogFrame(unsigned int f) { logFrame = f; }

protected:
	std::ofstream log;
	unsigned int logFrame;
};

#endif
