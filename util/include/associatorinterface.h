/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef ASSOCINTERFACE_H
#define ASSOCINTERFACE_H

#include <json.h>

/**
 * \namespace util
 * \brief glass namespace containing utility classes and functions
 *
 * The glass util namespace contains various base classes, class 
 * interfaces, and utility classes and functions used by other 
 * components of glass.
 */
namespace util {

/**
 * \interface iassociator
 * \brief associator messaging interface
 *
 * The iassociator interface is implemented by concrete classes that 
 * instantiate and manage the associator library, providing a 
 * standardized interface for other classes in glass to send 
 * information to the associator library via the sendtoassociator() 
 * function.
 */
class iAssociator {
 public:
	/**
	 * \brief Send data to associator
	 *
	 * This pure virtual function is implemented by a concrete class to 
	 * receive data for the associator library.
	 *
	 * \param message - A pointer to a json::object containing the data
	 * to send to the associator library.
	 */
	virtual void sendToAssociator(json::Object* message) = 0;
};
}  // namespace util
#endif  // ASSOCINTERFACE_H
