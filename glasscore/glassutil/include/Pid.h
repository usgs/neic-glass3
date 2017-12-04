/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef PID_H
#define PID_H
#include <string>

namespace glassutil {

/**
 * \brief glassutil id generation class
 *
 * The CPid class is the class that generates unique identifiers for data used
 * or created by glassutil.
 *
 * CPid uses CoCreateGuid (windows) or uuid_generate_random (linux) to generate
 * Practically globally/universally unique identifiers.
 */
class CPid {
 public:
	/**
	 * \brief CPid constructor
	 *
	 * The constructor for the CPid class.
	 */
	CPid();

	/**
	 * \brief CPid destructor
	 *
	 * The destructor for the CPid class.
	 */
	virtual ~CPid();

	/**
	 * \brief Generate an unique string identifier
	 *
	 * Generate a string unique identifier
	 *
	 * \return Returns a std::string containing the generated
	 * identifier.
	 */
	static std::string pid();

	/**
	 * \brief Generate an unique integer identifier
	 *
	 * Generate an unique integer identifier (random number)
	 *
	 * \return Returns an integer variable containing the generated
	 * identifier.
	 */
	static unsigned int random();
};
}  // namespace glassutil
#endif  // PID_H
