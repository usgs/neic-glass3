/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef CACHE_H
#define CACHE_H

#include <baseclass.h>
#include <json.h>
#include <logger.h>

#include <mutex>
#include <string>
#include <map>
#include <memory>

namespace util {
/**
 * \brief util cache class - a thread safe in-memory cache
 *
 * The util cache class is a class implementing an in-memory cache of
 * shared_ptr's to json::Objects.  The cache is thread safe.
 *
 * cache inherits from the baseclass class.
 */
class Cache : public util::BaseClass {
 public:
	/**
	 * \brief cache constructor
	 *
	 * The cache for the lookup class.
	 * Initializes members to default values.
	 */
	Cache();

	/**
	 * \brief cache destructor
	 *
	 * The destructor for the cache class.
	 */
	virtual ~Cache();

	/**
	 * \brief cache clear function
	 *
	 * The clear function for the cache class.
	 * Clear all data currently contained in the cache
	 */
	void clear() override;

	/**
	 *\brief add data to cache
	 *
	 * Add the provided data identified by the provided id to the cache
	 * \param data - A pointer to a json::Object to add to the cache
	 * \param id - A std::string that contains the key that identifies the data
	 * to add
	 * \return returns true if successful, false otherwise.
	 */
	virtual bool addToCache(std::shared_ptr<json::Object> data, std::string id);

	/**
	 *\brief remove data from cache
	 *
	 * Remove the data identified by the provided id from the cache
	 * \param id - A std::string that contains the key that identifies the data
	 * to remove
	 * \return returns true if successful, false otherwise.
	 */
	virtual bool removeFromCache(std::string id);

	/**
	 *\brief check for data in cache
	 *
	 * Check to see if the data identified by the provided id is in the cache
	 * \param id - A std::string that contains the key that identifies the data
	 * to remove
	 * \return returns true if successful, false otherwise.
	 */
	virtual bool isInCache(std::string id);

	/**
	 *\brief get data from cache
	 *
	 * Get the data identified by the provided id from the cache
	 * \param id - A std::string that contains the key that identifies the data
	 * to get
	 * \return returns a pointer to the json::Object containing the data, NULL
	 * if the data
	 * was not found
	 */
	virtual std::shared_ptr<json::Object> getFromCache(std::string id);

	/**
	 * \brief get next data from cache
	 *
	 * Gets the next data available from the cache
	 * \param restart - A boolean flag indicating whether to start at the
	 * beginning of the cache
	 * \return returns a pointer to the json::Object containing the next data,
	 * NULL if there is no
	 * more data available.
	 */
	virtual std::shared_ptr<json::Object> getNextFromCache(bool restart = false);

	/**
	 *\brief check if cache empty
	 *
	 * Check to see if the cache is empty
	 * \return returns true if empty, false otherwise.
	 */
	virtual bool isEmpty();

	/**
	 *\brief Retrieves a boolean flag indicating whether the cache has
	 * been modified
	 */
	bool getCacheModified();

	/**
	 *\brief Retrieves the current size of the cache
	 */
	int size();

 private:
	/**
	 * \brief the std::map used to store the cache
	 */
	std::map<std::string, std::shared_ptr<json::Object>> m_Cache;

	/**
	 * \brief a boolean flag indicating whether the cache has
	 * been modified
	 */
	bool m_bCacheModified;

	/**
	 * \brief a std::map iterator used to move through the cache via
	 * getNextFromCache()
	 */
	std::map<std::string, std::shared_ptr<json::Object>>::iterator m_CacheDumpItr;
};
}  // namespace util
#endif  // CACHE_H

