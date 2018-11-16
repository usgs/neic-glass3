#include <baseclass.h>
#include <json.h>
#include <logger.h>
#include <mutex>
#include <memory>
#include <string>

namespace glass3 {
namespace util {

// ---------------------------------------------------------BaseClass
BaseClass::BaseClass() {
	clear();
}

// ---------------------------------------------------------~BaseClass
BaseClass::~BaseClass() {
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
	setDefaultAgencyId("");
	setDefaultAuthor("");

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

// ---------------------------------------------------------getDefaultAgencyId
const std::string& BaseClass::getDefaultAgencyId() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_DefaultAgencyID);
}

// ---------------------------------------------------------setDefaultAgencyId
void BaseClass::setDefaultAgencyId(const std::string &id) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_DefaultAgencyID = id;
}

// ---------------------------------------------------------getDefaultAuthor
const std::string& BaseClass::getDefaultAuthor() {
	std::lock_guard<std::mutex> guard(getMutex());
	return (m_DefaultAuthor);
}

// ---------------------------------------------------------setDefaultAuthor
void BaseClass::setDefaultAuthor(const std::string &author) {
	std::lock_guard<std::mutex> guard(getMutex());
	m_DefaultAuthor = author;
}

}  // namespace util
}  // namespace glass3
