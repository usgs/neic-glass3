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

namespace util {

Cache::Cache() {
	logger::log("debug", "cache::cache(): Construction.");

	clear();
}

Cache::Cache(json::Object *config) {
	logger::log("debug", "cache::cache(...): Advanced Construction.");

	clear();

	// now call setup
	setup(config);
}

Cache::~Cache() {
	logger::log("debug", "cache::~cache(): Destruction.");

	writeCacheToDisk();

	clear();
}

// configuration
bool Cache::setup(json::Object *config) {
	if (config == NULL) {
		logger::log(
				"info",
				"cache::setup(): NULL configuration passed in, disk caching will"
				" be disabled.");
		return (true);
	}

	logger::log("debug", "cache::setup(): Setting Up.");

	// Cmd
	if (!(config->HasKey("Cmd"))) {
		logger::log("error", "cache::setup(): BAD configuration passed in.");
		return (false);
	} else {
		std::string configtype = (*config)["Cmd"].ToString();
		if (configtype != "Cache") {
			logger::log(
					"error",
					"cache::setup(): Wrong configuration provided, configuration"
							" is for: " + configtype + ".");
			return (false);
		}
	}

	// lock our configuration while we're updating it
	// this mutex may be pointless
	m_ConfigMutex.lock();

	// see if a disk cache file is specified
	if (!(config->HasKey("DiskFile"))) {
		logger::log("error", "cache::setup(): No disk file provided.");
		return (false);
	} else {
		// we're using a disk cache
		m_sDiskCacheFile = (*config)["DiskFile"].ToString();
		logger::log(
				"info",
				"cache::setup(): Using file: " + m_sDiskCacheFile
						+ " for disk cache.");
	}

	// unlock our configuration
	m_ConfigMutex.unlock();

	loadCacheFromDisk();

	logger::log("debug", "cache::setup(): Done Setting Up.");

	// do baseclass setup;
	// mostly remembering our config object
	util::BaseClass::setup(config);

	// we're done
	return (true);
}

void Cache::clear() {
	logger::log("debug", "lookup::clear(): clearing configuration.");

	// disk cache
	m_sDiskCacheFile = "";

	clearCache();

	// finally do baseclass clear
	util::BaseClass::clear();
}

// cache managment
bool Cache::addToCache(json::Object * data, std::string id, bool lock) {
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
	if (lock) {
		m_CacheMutex.lock();
	}

	// see if we have it already
	if (m_Cache.find(id) != m_Cache.end()) {
		// we do, replace what we have
		// grab the old
		json::Object * olddata = m_Cache[id];

		// update the cache
		m_Cache[id] = data;

		// cleanup
		if (olddata != NULL)
			delete (olddata);

		if (lock) {
			m_CacheMutex.unlock();
		}

		// we've modified the cache
		m_bCacheModified = true;

		logger::log(
				"trace",
				"cache::addtocache(): Updated Data " + json::Serialize(*data)
						+ " was in the Cache.");
		return (true);
	}

	// we don't, add it
	m_Cache[id] = data;

	// unlock
	if (lock) {
		m_CacheMutex.unlock();
	}

	// we've modified the cache
	m_bCacheModified = true;

	logger::log(
			"trace",
			"cache::addtocache(): Added Data " + json::Serialize(*data)
					+ " to Cache.");

	return (true);
}

bool Cache::removeFromCache(std::string id, bool lock) {
	// don't do anything if we didn't get an id
	if (id == "") {
		logger::log("error", "cache::removefromcache(): Bad ID passed in.");
		return (false);
	}
	if (lock) {
		m_CacheMutex.lock();
	}

	// see if we have it
	if (m_Cache.find(id) == m_Cache.end()) {
		// we don't, do nothing
		if (lock) {
			m_CacheMutex.unlock();
		}

		logger::log(
				"warning",
				"cache::removefromcache(): Did not erase data " + id
						+ "; not found in Cache.");

		return (false);
	}

	// grab the old
	json::Object * olddata = m_Cache[id];

	// erase the element from the map
	m_Cache.erase(m_Cache.find(id));

	// cleanup
	if (olddata != NULL)
		delete (olddata);

	if (lock) {
		m_CacheMutex.unlock();
	}

	logger::log(
			"trace",
			"cache::removefromcache(): Removed data " + id + " from Cache.");

	// we've modified the cache
	m_bCacheModified = true;

	return (true);
}

bool Cache::isInCache(std::string id, bool lock) {
	// don't do anything if we didn't get an id
	if (id == "") {
		logger::log("error", "cache::isincache(): Bad ID passed in.");
		return (false);
	}

	if (lock) {
		m_CacheMutex.lock();
	}

	// see if we have it
	if (m_Cache.find(id) == m_Cache.end()) {
		// we don't, do nothing
		if (lock) {
			m_CacheMutex.unlock();
		}

		return (false);
	}

	if (lock) {
		m_CacheMutex.unlock();
	}

	return (true);
}

json::Object * Cache::getFromCache(std::string id, bool lock, bool copy) {
	// don't do anything if we didn't get an id
	if (id == "") {
		logger::log("error", "cache::getfromcache(): Bad ID passed in.");
		return (NULL);
	}

	logger::log("trace",
				"cache::getfromcache(): Looking for ID " + id + " in Cache.");

	// lock in case someone else is using the cache
	if (lock) {
		m_CacheMutex.lock();
	}

	// see if we even have it
	if (m_Cache.find(id) == m_Cache.end()) {
		// we don't
		if (lock) {
			m_CacheMutex.unlock();
		}

		logger::log(
				"trace",
				"cache::getfromcache(): ID " + id + " was not found in Cache.");
		return (NULL);
	}

	// get result from cache
	json::Object *data;
	if (copy)
		data = new json::Object(*m_Cache[id]);
	else
		data = m_Cache[id];

	if (lock) {
		m_CacheMutex.unlock();
	}

	logger::log(
			"trace",
			"cache::getstationfromcache(): Got ID " + json::Serialize(*data)
					+ " from Cache.");

	// return our result
	return (data);
}

json::Object * Cache::getNextFromCache(bool restart, bool lock) {
	// lock in case someone else is using the cache
	if (lock) {
		m_CacheMutex.lock();
	}

	if (m_Cache.empty() == true) {
		if (lock) {
			m_CacheMutex.unlock();
		}

		return (NULL);
	}

	// start at the beginning if requested
	if (restart == true) {
		m_CacheDumpItr = m_Cache.begin();
	} else if (m_CacheDumpItr == m_Cache.end()) {
		// make sure we're not at the end
		if (lock) {
			m_CacheMutex.unlock();
		}

		return (NULL);
	}

	// get current data from the iterator
	json::Object * data = (json::Object *) m_CacheDumpItr->second;

	// advance the iterator
	++m_CacheDumpItr;

	// unlock
	if (lock) {
		m_CacheMutex.unlock();
	}

	return (data);
}

void Cache::clearCache(bool lock) {
	// lock in case someone else is using the cache
	if (lock) {
		m_CacheMutex.lock();
	}

	// erase the entire cache
	m_Cache.erase(m_Cache.begin(), m_Cache.end());
	m_bCacheModified = true;

	// unlock
	if (lock) {
		m_CacheMutex.unlock();
	}

	logger::log("info", "cache::clearcache(): Cleared Cache.");
}

bool Cache::loadCacheFromDisk(bool lock) {
	// pull data from our config
	m_ConfigMutex.lock();
	std::string cachefile = m_sDiskCacheFile;
	m_ConfigMutex.unlock();

	if (cachefile == "") {
		logger::log(
				"debug",
				"cache::loadcachefromdisk(): Use of disk cache is not enabled, "
				"returning.");
		return (false);
	}

	std::chrono::high_resolution_clock::time_point tFileStartTime =
			std::chrono::high_resolution_clock::now();

	// check to see if file exists
	if (std::ifstream(cachefile).good()) {
		// lock incase someone else is using the disk file
		m_DiskFileMutex.lock();

		// open the file
		std::ifstream infile;
		std::string line;
		infile.open(cachefile, std::ios::in);

		int datacount = 0;

		// while there is stuff to read
		while (infile) {
			// get a line
			std::getline(infile, line);

			// make sure we've not got the empty line at the end of the file
			if (line.length() > 0) {
				// try to convert the line to a json object
				json::Object *datatoadd = NULL;
				try {
					json::Value deserializeddata = json::Deserialize(line);

					// make sure we got valid json
					if (deserializeddata.GetType()
							== json::ValueType::NULLVal) {
						logger::log(
								"warning",
								"cache::loadcachefromdisk: json::Deserialize "
								"returned null, skipping to next line.");
						continue;
					}

					// convert our resulting value to a json object
					datatoadd = new json::Object(deserializeddata.ToObject());
				} catch (const std::runtime_error &e) {
					// oopse
					std::string exceptionstring = e.what();
					logger::log(
							"error",
							"cache::loadcachefromdisk: json::Deserialize "
									"encountered error " + exceptionstring
									+ ", returning.");
					return (false);
				}

				// make sure Deserialize gave us something
				if (datatoadd == NULL) {
					logger::log("warning",
								"cache::loadcachefromdisk: json::Deserialize "
								"returned null, skipping to next line.");
					continue;
				}

				if (!(datatoadd->HasKey("cacheid"))) {
					logger::log("error",
								"cache::addtocache(): Json object is missing "
								"required cacheid key.");
					continue;
				}

				// use the site in the message as the ID
				std::string id = (*datatoadd)["cacheid"];

				// add to cache
				addToCache(datatoadd, id, lock);

				datacount++;
			}
		}

		m_DiskFileMutex.unlock();

		// we just loaded it from disk
		m_bCacheModified = false;

		std::chrono::high_resolution_clock::time_point tFileEndTime =
				std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> tFileProcDuration =
				std::chrono::duration_cast<std::chrono::duration<double>>(
						tFileEndTime - tFileStartTime);

		logger::log(
				"info",
				"cache::loadcachefromdisk: Loaded " + std::to_string(datacount)
						+ " data into cache from file: " + cachefile + " in "
						+ std::to_string(tFileProcDuration.count())
						+ " seconds.");
	} else {
		// found no file, we're starting from scratch
		logger::log(
				"warning",
				"cache::loadcachefromdisk: No Data Cache file " + cachefile
						+ " found on disk.");
		return (false);
	}

	return (true);
}

bool Cache::isEmpty(bool lock) {
	if (lock) {
		m_CacheMutex.lock();
	}

	bool cacheempty = m_Cache.empty();

	if (lock) {
		m_CacheMutex.unlock();
	}

	return (cacheempty);
}

bool Cache::writeCacheToDisk(bool lock) {
	// pull data from our config
	m_ConfigMutex.lock();
	std::string cachefile = m_sDiskCacheFile;
	m_ConfigMutex.unlock();

	// are we supposed to be writing the cache to disk?
	if (cachefile == "") {
		logger::log(
				"debug",
				"cache::writecachetodisk: Use of disk cache is not enabled.");
		return (true);
	}

	// is there anything to write?
	if (isEmpty(lock) == true) {
		logger::log(
				"debug",
				"cache::writecachetodisk: Cache is empty, skipping writing to "
				"disk.");
		return (true);
	}

	// check to see if anything has changed
	if (m_bCacheModified == false) {
		logger::log(
				"debug",
				"cache::writecachetodisk: No change in cache, skipping writing "
				"to disk.");
		return (true);
	}

	// set up timing code
	std::chrono::high_resolution_clock::time_point tFileStartTime =
			std::chrono::high_resolution_clock::now();
	int datacount = 0;

	// lock incase someone else is using the disk file
	m_DiskFileMutex.lock();

	try {
		// check to see if file exists
		if (std::ifstream(cachefile).good()) {
			// we have a file, delete it
			if (std::remove(cachefile.c_str()) != 0) {
				// sleep a little while
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

				// try again
				if (std::remove(cachefile.c_str()) != 0) {
					logger::log(
							"error",
							"cache::writecachetodisk: Failed to delete old cache "
									+ cachefile + " from disk; Second try.");
					return (false);
				} else {
					logger::log(
							"info",
							"cache::writecachetodisk: Deleted old cache "
									+ cachefile + " from disk; Second Try.");
				}
			} else {
				logger::log(
						"info",
						"cache::writecachetodisk: Deleted old cache "
								+ cachefile + " from disk.");
			}
		}

		// open file for writing
		std::ofstream outfile;
		outfile.open(cachefile, std::ios::out | std::ios::app);

		// lock incase someone else is using the cache
		if (lock) {
			m_CacheMutex.lock();
		}

		// now go through the whole station cache
		std::map<std::string, json::Object *>::iterator CacheItr;
		for (CacheItr = m_Cache.begin(); CacheItr != m_Cache.end();
				++CacheItr) {
			// get the current station
			json::Object * data = (json::Object *) CacheItr->second;
			std::string id = CacheItr->first;

			// make sure we have an id
			if (!(data->HasKey("cacheid"))) {
				// tag the id onto the data
				(*data)["cacheid"] = id;
			}

			// write current station to file, followed by newline.
			outfile << json::Serialize(*data).c_str() << "\n";

			datacount++;
		}

		// unlock
		if (lock) {
			m_CacheMutex.unlock();
		}

		outfile.close();
	} catch (const std::exception &e) {
		printf("%s", e.what());
		logger::log(
				"warning",
				"cache::writecachetodisk: Writing cache to disk: "
						+ std::string(e.what()));
	}

	m_DiskFileMutex.unlock();

	std::chrono::high_resolution_clock::time_point tFileEndTime =
			std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> tFileProcDuration =
			std::chrono::duration_cast<std::chrono::duration<double>>(
					tFileEndTime - tFileStartTime);

	// we're done
	logger::log(
			"info",
			"cache::writecachetodisk: Wrote cache to disk " + cachefile + " ("
					+ std::to_string(datacount) + " data) " + " in "
					+ std::to_string(tFileProcDuration.count()) + " seconds.");

	// mark cache unmodified, we've just written it to disk
	m_bCacheModified = false;

	return (true);
}
}  // namespace util
