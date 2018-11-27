/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef BASECLASS_H
#define BASECLASS_H

#include <json.h>
#include <mutex>
#include <atomic>
#include <memory>
#include <string>

namespace glass3 {
namespace util {
/**
 * \brief glass3::util::BaseClass class - encapsulates the most basic setup and
 * configuration logic.
 *
 * Class encapsulating the setup and configuration logic, which is common to
 * most neic-glass3 classes outside of glasscore.  The BaseClass is a
 * simple, almost abstract class that provides setup and clear interfaces and
 * keeps a pointer to the current configuration. The class also provides a
 * mutex for thread safety
 *
 * This class is intended to be extended by derived classes.
 */
class BaseClass {
 public:
	/**
	 * \brief BaseClass constructor
	 *
	 * The constructor for the BaseClass class.
	 * Initializes members to default values.
	 */
	BaseClass();

	/**
	 * \brief BaseClass destructor
	 *
	 * The destructor for the BaseClass class.
	 */
	virtual ~BaseClass();

	/**
	 * \brief BaseClass configuration function
	 *
	 * The this function configures the BaseClass class
	 * \param config - A shared_ptr to a json::Object containing to the
	 * configuration to use
	 * \warning WARNING! Uses the base class mutex available via getMutex(),
	 * locking getMutex(), or any other method where the class mutex is obtained
	 * and locked, before calling setup will cause a deadlock.
	 * \return returns true if successful.
	 */
	virtual bool setup(std::shared_ptr<const json::Object> config);

	/**
	 * \brief BaseClass clear function
	 *
	 * The clear function for the BaseClass class.
	 * Clears all configuration.
	 * \warning WARNING! Uses the base class mutex available via getMutex(),
	 * locking getMutex(), or any other method where the class mutex is obtained
	 * and locked, before calling setup will cause a deadlock.
	 */
	virtual void clear();

	/* \brief Retrieves a pointer to the class member json::Object that holds
	 * the configuration.
	 *
	 * \return returns a share_ptr to a json::Object containing the configuration
	 */
	const std::shared_ptr<const json::Object> getConfig();

	/**
	 * \brief Retrieves the class member boolean flag indicating whether the
	 * class has been setup, set to true if setup was successful.
	 */
	bool getSetup();

	/**
	 * \brief Function to retrieve the name of the default agency id
	 *
	 * This function retrieves the name of default agency id, this name is used
	 * by many of the  derived classes
	 *
	 * \return A std::string containing the  agency id
	 */
	const std::string& getDefaultAgencyId();

	/**
	 * \brief Function to set the name of the default agency id
	 *
	 * This function sets the name of the default agency id, this name is used
	 * by many of the  derived classes
	 *
	 * \param id = A std::string containing the default agency id to set
	 */
	void setDefaultAgencyId(const std::string &id);

	/**
	 * \brief Function to retrieve the name of the default author
	 *
	 * This function retrieves the name of the default author, this name is used
	 * by many of the  derived classes
	 *
	 * \return A std::string containing the author
	 */
	const std::string& getDefaultAuthor();

	/**
	 * \brief Function to set the name of the default author
	 *
	 * This function sets the name of the default author, this name is used
	 * by many of the  derived classes
	 *
	 * \param author = A std::string containing the default author to set
	 */
	void setDefaultAuthor(const std::string &author);

 protected:
	/**
	 * \brief A shared pointer to the json::Object that holds the configuration
	 */
	std::shared_ptr<const json::Object> m_Config;

	/**
	 * \brief the boolean flag indicating whether the class has been
	 * setup, set to true if setup was successful.
	 */
	std::atomic<bool> m_bIsSetup;

	/**
	 * \brief A std::string containing the default agency id to
	 * use in parsing if one is not provided.
	 */
	std::string m_DefaultAgencyID;

	/**
	 * \brief A std::string containing the default author to
	 * use in parsing if one is not provided.
	 */
	std::string m_DefaultAuthor;

 private:
	/**
	 * \brief Retrieves a reference to the class member containing the mutex
	 * used to control access to class members
	 */
	std::mutex & getMutex();

	/**
	 * \brief A mutex to control access to BaseClass members
	 */
	std::mutex m_Mutex;
};
}  // namespace util
}  // namespace glass3
#endif  // BASECLASS_H
