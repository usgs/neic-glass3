#include <baseclass.h>
#include <json.h>
#include <logger.h>
#include <mutex>

namespace util {

// ---------------------------------------------------------BaseClass
BaseClass::BaseClass() {
	m_bIsSetup = false;
	m_Config = NULL;
}

// ---------------------------------------------------------~BaseClass
BaseClass::~BaseClass() {
	clear();
}

// ---------------------------------------------------------setup
bool BaseClass::setup(json::Object *config) {
	std::lock_guard<std::mutex> guard(getMutex());

	// null check
	if (config == NULL) {
		return (false);
	}

	// to be overrided by child classes
	m_Config = config;
	m_bIsSetup = true;

	return (true);
}

// ---------------------------------------------------------clear
void BaseClass::clear() {
	std::lock_guard<std::mutex> guard(getMutex());

	// to be overrided by child classes
	m_Config = NULL;
	m_bIsSetup = false;
}


// ---------------------------------------------------------getConfig
const json::Object * BaseClass::getConfig() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_Config);
}

// ---------------------------------------------------------getSetup
bool BaseClass::getSetup() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_bIsSetup);
}

// ---------------------------------------------------------getMutex
std::mutex & BaseClass::getMutex() {
	return (m_Mutex);
}

}  // namespace util

