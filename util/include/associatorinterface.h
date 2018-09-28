/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
/**
 * \file
 * \brief associatorinterface.h
 *
 * associatorinterface.h contains the associator interface
 */
#ifndef ASSOCINTERFACE_H
#define ASSOCINTERFACE_H

#include <json.h>
#include <memory>

/**
 * \namespace glass3
 * \brief The neic-glass3 namespace containing all the classes and functions that
 * make up neic-glass3
 *
 * The glass3 namespace contains the classes and functions that make up
 * neic-glass3, including applications (glass-app, etc.), libraries (output,
 * parse, etc.), and utilities (cache, configuration, file utils, etc.)
 */
namespace glass3 {

/**
 * \namespace glass3::util
 * \brief The neic-glass3 namespace containing utility classes and functions
 *
 * The neic-glass3 util namespace contains various base classes, class
 * interfaces, and utility classes and functions used by other 
 * components of neic-glass3.
 */
namespace util {

/**
 * \interface iAssociator
 * \brief The associator messaging interface, used in sending configuration and
 * input data to neic-glass3, specifically the glasscore libraries
 *
 * The iAssociator interface is a class interface implemented by class that
 * allocate, start, and monitor an instance of the glasscore associator library.
 * This interface provides a method for sending messages (such as configuration
 * or input data) to glasscore via the sendtoassociator() function.
 */
class iAssociator {
 public:
	/**
	 * \brief Send data to associator
	 *
	 * This pure virtual function is implemented by a class to receive messages
	 * (such as configuration or input data) for the associator library.
	 *
	 * \param message - A std::shared_ptr to a json::object containing the data
	 * to send to the associator library.
	 */
	virtual void sendToAssociator(std::shared_ptr<json::Object> &message) = 0;  // NOLINT
};
}  // namespace util
}  // namespace glass3
#endif  // ASSOCINTERFACE_H
