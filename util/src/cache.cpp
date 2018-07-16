#include <cache.h>
#include <json.h>
#include <logger.h>
#include <stringutil.h>
#include <timeutil.h>
#include <mutex>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

namespace glass3 {
namespace util {

// ---------------------------------------------------------Cache
Cache::Cache() {
	clear();
}

// ---------------------------------------------------------~Cache
Cache::~Cache() {
	clear();
}

// ---------------------------------------------------------clear
void Cache::clear() {
	// lock in case someone else is using the cache
	getMutex().lock();

	// erase the entire cache
	m_Cache.erase(m_Cache.begin(), m_Cache.end());

	// unlock before baseclass clear (uses the same mutex)
	getMutex().unlock();

	// finally do baseclass clear
	util::BaseClass::clear();
}

// ---------------------------------------------------------addToCache
bool Cache::addToCache(std::shared_ptr<json::Object> data, std::string id) {
	if (data == NULL) {
		logger::log("error", "cache::addtocache(): Bad json object passed in.");
		return (false);
	}

	// don't do anything if we didn't get an ID
	if (id == "") {
		logger::log("error", "cache::addstationtocache(): Bad id passed in.");
		return (false);
	}

	// lock in case someone else is using the cache
	std::lock_guard<std::mutex> guard(getMutex());

	// see if we have it already
	if (m_Cache.find(id) != m_Cache.end()) {
		// we do, replace what we have
		// update the cache
		m_Cache[id] = data;

		logger::log(
				"trace",
				"cache::addtocache(): Updated Data " + json::Serialize(*data)
						+ " was in the Cache.");
		return (true);
	}

	// we don't, add it
	m_Cache[id] = data;

	logger::log(
			"trace",
			"cache::addtocache(): Added Data " + json::Serialize(*data)
					+ " to Cache.");

	return (true);
}

// ---------------------------------------------------------removeFromCache
bool Cache::removeFromCache(std::string id) {
	// don't do anything if we didn't get an id
	if (id == "") {
		logger::log("error", "cache::removefromcache(): Bad ID passed in.");
		return (false);
	}

	std::lock_guard<std::mutex> guard(getMutex());

	// see if we have it
	if (m_Cache.find(id) == m_Cache.end()) {
		// we don't, do nothing
		logger::log(
				"warning",
				"cache::removefromcache(): Did not erase data " + id
						+ "; not found in Cache.");

		return (false);
	}

	// erase the element from the map
	m_Cache.erase(m_Cache.find(id));

	logger::log(
			"trace",
			"cache::removefromcache(): Removed data " + id + " from Cache.");

	return (true);
}

// ---------------------------------------------------------isInCache
bool Cache::isInCache(std::string id) {
	// don't do anything if we didn't get an id
	if (id == "") {
		logger::log("error", "cache::isincache(): Bad ID passed in.");
		return (false);
	}

	std::lock_guard<std::mutex> guard(getMutex());

	// see if we have it
	if (m_Cache.find(id) == m_Cache.end()) {
		// we don't, do nothing
		return (false);
	}

	return (true);
}

// ---------------------------------------------------------getFromCache
std::shared_ptr<json::Object> Cache::getFromCache(std::string id) {
	// don't do anything if we didn't get an id
	if (id == "") {
		logger::log("error", "cache::getfromcache(): Bad ID passed in.");
		return (NULL);
	}

	logger::log("trace",
				"cache::getfromcache(): Looking for ID " + id + " in Cache.");

	// lock in case someone else is using the cache
	std::lock_guard<std::mutex> guard(getMutex());

	// see if we even have it
	if (m_Cache.find(id) == m_Cache.end()) {
		// we don't
		logger::log(
				"trace",
				"cache::getfromcache(): ID " + id + " was not found in Cache.");
		return (NULL);
	}

	// get result from cache
	std::shared_ptr<json::Object> data = m_Cache[id];

	logger::log(
			"trace",
			"cache::getstationfromcache(): Got ID " + json::Serialize(*data)
					+ " from Cache.");

	// return our result
	return (data);
}

// ---------------------------------------------------------getNextFromCache
std::shared_ptr<json::Object> Cache::getNextFromCache(bool restart) {
	// lock in case someone else is using the cache
	std::lock_guard<std::mutex> guard(getMutex());

	if (m_Cache.empty() == true) {
		return (NULL);
	}

	// start at the beginning if requested
	if (restart == true) {
		m_CacheDumpItr = m_Cache.begin();
	} else if (m_CacheDumpItr == m_Cache.end()) {
		// make sure we're not at the end
		return (NULL);
	}

	// get current data from the iterator
	std::shared_ptr<json::Object> data =
			(std::shared_ptr<json::Object>) m_CacheDumpItr->second;

	// advance the iterator
	++m_CacheDumpItr;

	return (data);
}

// ---------------------------------------------------------isEmpty
bool Cache::isEmpty() {
	std::lock_guard<std::mutex> guard(getMutex());

	bool cacheempty = m_Cache.empty();
	return (cacheempty);
}

// ---------------------------------------------------------size
int Cache::size() {
	int cachesize = 0;

	std::lock_guard<std::mutex> guard(getMutex());
	if (m_Cache.empty() != true) {
		cachesize = static_cast<int>(m_Cache.size());
	}

	return (cachesize);
}
}  // namespace util
}  // namespace glass3
