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

namespace glass3 {
namespace util {
/**
 * \brief glass3::util::Cache class - a thread safe in-memory cache
 *
 * The glass3::util::Cache class is a class implementing an in-memory cache of
 * shared_ptr's to json::Objects as a std::map.  The cache supports iteration,
 * and is thread safe.
 *
 * Cache inherits from the baseclass class.
 */
class Cache : public util::BaseClass {
 public:
	/**
	 * \brief Cache constructor
	 *
	 * The Cache for the lookup class.
	 * Initializes members to default values.
	 */
	Cache();

	/**
	 * \brief Cache destructor
	 *
	 * The destructor for the Cache class.
	 */
	virtual ~Cache();

	/**
	 * \brief Cache clear function
	 *
	 * The clear function for the Cache class.
	 * Clear all data currently contained in the Cache
	 */
	void clear() override;

	/**
	 *\brief add data to Cache
	 *
	 * Add the provided data identified by the provided id to the Cache
	 * \param data - A pointer to a json::Object to add to the Cache
	 * \param id - A std::string that contains the key that identifies the data
	 * to add
	 * \return returns true if successful, false otherwise.
	 */
	virtual bool addToCache(std::shared_ptr<json::Object> data, std::string id);

	/**
	 *\brief remove data from Cache
	 *
	 * Remove the data identified by the provided id from the Cache
	 * \param id - A std::string that contains the key that identifies the data
	 * to remove
	 * \return returns true if successful, false otherwise.
	 */
	virtual bool removeFromCache(std::string id);

	/**
	 *\brief check for data in Cache
	 *
	 * Check to see if the data identified by the provided id is in the Cache
	 * \param id - A std::string that contains the key that identifies the data
	 * to remove
	 * \return returns true if successful, false otherwise.
	 */
	virtual bool isInCache(std::string id);

	/**
	 *\brief get data from Cache
	 *
	 * Get the data identified by the provided id from the Cache
	 * \param id - A std::string that contains the key that identifies the data
	 * to get
	 * \return returns a pointer to the json::Object containing the data, NULL
	 * if the data
	 * was not found
	 */
	virtual std::shared_ptr<const json::Object> getFromCache(std::string id);

	/**
	 * \brief get next data from Cache
	 *
	 * Gets the next data available from the Cache
	 * \param restart - A boolean flag indicating whether to start at the
	 * beginning of the Cache
	 * \return returns a pointer to the json::Object containing the next data,
	 * NULL if there is no
	 * more data available.
	 */
	virtual std::shared_ptr<const json::Object> getNextFromCache(
			bool restart = false);

	/**
	 *\brief check if Cache empty
	 *
	 * Check to see if the Cache is empty
	 * \return returns true if empty, false otherwise.
	 */
	virtual bool isEmpty();

	/**
	 *\brief Retrieves the current size of the Cache
	 */
	int size();

 private:
	/**
	 * \brief the std::map used to store the Cache
	 */
	std::map<std::string, std::shared_ptr<json::Object>> m_Cache;

	/**
	 * \brief a std::map iterator used to move through the Cache via
	 * getNextFromCache()
	 */
	std::map<std::string, std::shared_ptr<json::Object>>::iterator m_CacheDumpItr;
};
}  // namespace util
}  // namespace glass3
#endif  // CACHE_H

