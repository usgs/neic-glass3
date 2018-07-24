#include <baseclass.h>
#include <json.h>
#include <logger.h>
#include <mutex>
#include <memory>

namespace glass3 {
namespace util {

// ---------------------------------------------------------BaseClass
BaseClass::BaseClass() {
	m_bIsSetup = false;
	m_Config.reset();
}

// ---------------------------------------------------------~BaseClass
BaseClass::~BaseClass() {
	clear();
}

// ---------------------------------------------------------setup
bool BaseClass::setup(std::shared_ptr<const json::Object> config) {
	std::lock_guard<std::mutex> guard(getMutex());

	// null check
	if (config == NULL) {
		return (false);
	}

	// to be overridden by child classes
	m_Config = config;
	m_bIsSetup = true;

	return (true);
}

// ---------------------------------------------------------clear
void BaseClass::clear() {
	std::lock_guard<std::mutex> guard(getMutex());

	// to be overridden by child classes
	m_Config.reset();
	m_bIsSetup = false;
}

// ---------------------------------------------------------getConfig
const std::shared_ptr<const json::Object> BaseClass::getConfig() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_Config);
}

// ---------------------------------------------------------getSetup
bool BaseClass::getSetup() {
	return (m_bIsSetup);
}

// ---------------------------------------------------------getMutex
std::mutex & BaseClass::getMutex() {
	return (m_Mutex);
}

}  // namespace util
}  // namespace glass3
