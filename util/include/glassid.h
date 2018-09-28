/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef GLASSID_H
#define GLASSID_H
#include <string>

namespace glass3 {
namespace util {

/**
 * \brief glassutil id generation class
 *
 * The GlassID class is the class that generates unique identifiers for data used
 * or created by glass3.
 *
 * GlassID uses CoCreateGuid (windows) or uuid_generate_random (linux) to generate
 * Practically globally/universally unique identifiers.
 */
class GlassID {
 public:
	/**
	 * \brief GlassID constructor
	 *
	 * The constructor for the GlassID class.
	 */
	GlassID();

	/**
	 * \brief GlassID destructor
	 *
	 * The destructor for the GlassID class.
	 */
	virtual ~GlassID();

	/**
	 * \brief Generate an unique string identifier
	 *
	 * Generate a string unique identifier
	 *
	 * \return Returns a std::string containing the generated
	 * identifier.
	 */
	static std::string getID();

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
}  // namespace util
}  // namespace glass3
#endif  // GLASSID_H
