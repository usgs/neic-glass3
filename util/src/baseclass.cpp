#include <baseclass.h>
#include <json.h>
#include <logger.h>

namespace util {
// construction / destruction
BaseClass::BaseClass() {
	logger::log("debug", "BaseClass::BaseClass(): Construction.");

	m_bIsSetup = false;
	m_Config = NULL;
}

BaseClass::~BaseClass() {
	logger::log("debug", "BaseClass::~BaseClass(): Destruction.");
}

// configuration
bool BaseClass::setup(json::Object *config) {
	// to be overrided by child classes
	m_Config = config;
	m_bIsSetup = true;

	return (true);
}

void BaseClass::clear() {
	// to be overrided by child classes
	m_Config = NULL;
	m_bIsSetup = false;
}
}  // namespace util

