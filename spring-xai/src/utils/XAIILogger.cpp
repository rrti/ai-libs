#include "./XAIILogger.hpp"

void XAIILogger::OpenLog(const std::string& n) {
	log.open(n.c_str(), std::ios::out);
}
void XAIILogger::CloseLog() {
	log.flush();
	log.close();
}
