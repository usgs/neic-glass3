/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef BASECLASS_H
#define BASECLASS_H

#include <json.h>

/**
 * @namespace util
 * The namespace containing a collection of utility classes and functions.
 */
namespace util {
/**
 * \brief util baseclass class
 *
 * The util baseclass class is a class encapsulating the setup and configuration
 * logic.  The baseclass class supports setup, clearing, and keeping
 * a pointer to the current configuration
 *
 * This class is intended to be extended by derived classes.
 */
class BaseClass {
 public:
	/**
	 * \brief baseclass constructor
	 *
	 * The constructor for the baseclass class.
	 * Initializes members to default values.
	 */
	BaseClass();

	/**
	 * \brief baseclass destructor
	 *
	 * The destructor for the baseclass class.
	 */
	virtual ~BaseClass();

	/**
	 * \brief baseclass configuration function
	 *
	 * The this function configures the baseclass class
	 * \param config - A pointer to a json::Object containing to the
	 * configuration to use
	 * \return returns true if successful.
	 */
	virtual bool setup(json::Object *config);

	/**
	 * \brief baseclass clear function
	 *
	 * The clear function for the baseclass class.
	 * Clears all configuration
	 */
	virtual void clear();

	/**
	 * \brief A pointer to the json::Object that holds the configuration
	 */
	json::Object *m_Config;

	/**
	 * \brief the boolean flag indicating whether the class has been
	 * setup.
	 */
	bool m_bIsSetup;
};
}  // namespace util
#endif  // BASECLASS_H
