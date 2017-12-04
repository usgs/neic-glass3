/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef INPUTINTERFACE_H
#define INPUTINTERFACE_H

#include <json.h>

namespace util {

/**
 * \interface iinput
 * \brief input data retrieval interface
 *
 * The iinput interface is implemented by concrete classes that
 * provide input data to the associator, providing a standardized 
 * interface for other classes in glass to request input data
 * via the getdata() function.
 */
class iInput {
 public:
	/**
	 * \brief Get input data
	 *
	 * This pure virtual function is implemented by a concrete class to
	 * support retrieving input data. 
	 *
	 * \return Returns a pointer to a json::object containing the input
	 * data.
	 */
	virtual json::Object* getData() = 0;

	/**
	 * \brief Get count of remaining input data
	 *
	 * This pure virtual function is implemented by a concrete class to
	 * support retrieving the current count of the remaining input data.
	 *
	 * \return Returns an integer value containing the current count of
	 * remaining input data.
	 */
	virtual int dataCount() = 0;
};
}  // namespace util
#endif  // INPUTINTERFACE_H
