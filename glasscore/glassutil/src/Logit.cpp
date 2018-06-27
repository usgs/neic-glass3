// Seed.cpp
#include <cstdio>
#include <string>
#include "Logit.h"

namespace glassutil {

#ifndef _WIN32
std::function<void(logMessageStruct)> CLogit::m_logCallback = NULL;
#endif

bool CLogit::bDisable = false;
// ---------------------------------------------------------CLogit
CLogit::CLogit() {
}

// ---------------------------------------------------------~CLogit
CLogit::~CLogit() {
}

// ---------------------------------------------------------setLogCallback
void CLogit::setLogCallback(std::function<void(logMessageStruct)> callback) {
#ifndef _WIN32
	m_logCallback = callback;
#endif
}

// ---------------------------------------------------------disable
void CLogit::disable() {
	bDisable = true;
}

// ---------------------------------------------------------enable
void CLogit::enable() {
	bDisable = false;
}

// ---------------------------------------------------------Out
void CLogit::Out(const char *s) {
	CLogit::log(s);
}

// ---------------------------------------------------------log
void CLogit::log(const char * logMessage) {
	log(log_level::debug, std::string(logMessage));
}

// ---------------------------------------------------------log
void CLogit::log(log_level logLevel, const char * logMessage) {
	log(logLevel, std::string(logMessage));
}

// ---------------------------------------------------------log
void CLogit::log(std::string logMessage) {
	log(log_level::debug, logMessage);
}

// ---------------------------------------------------------log
void CLogit::log(log_level logLevel, std::string logMessage) {
	// don't bother if logging is disabled
	if (bDisable == true) {
		return;
	}

#ifndef _WIN32
	// use the callback if it's available
	if (m_logCallback) {
		logMessageStruct newMessage;
		newMessage.level = logLevel;
		newMessage.message = logMessage;

		m_logCallback(newMessage);
	} else {
		printf("%s\n", logMessage.c_str());
	}
#else
	printf("%s\n", logMessage.c_str());
#endif
}
}  // namespace glassutil
