// Seed.cpp
#include <logger.h>

#include <cstdio>
#include <string>
#include "Logit.h"

namespace glassutil {

bool CLogit::bDisable = false;
// ---------------------------------------------------------CLogit
CLogit::CLogit() {
}

// ---------------------------------------------------------~CLogit
CLogit::~CLogit() {
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

	if (logLevel == glassutil::log_level::info) {
		glass3::util::log("info", "glasscore: " + logMessage);
	} else if (logLevel == glassutil::log_level::debug) {
		glass3::util::log("debug", "glasscore: " + logMessage);
	} else if (logLevel == glassutil::log_level::warn) {
		glass3::util::log("warning", "glasscore: " + logMessage);
	} else if (logLevel == glassutil::log_level::error) {
		glass3::util::log("error", "glasscore: " + logMessage);
	}
}
}  // namespace glassutil
