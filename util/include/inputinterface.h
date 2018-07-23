/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef INPUTINTERFACE_H
#define INPUTINTERFACE_H

#include <json.h>
#include <memory>

namespace glass3 {
namespace util {

/**
 * \interface iInput
 * \brief The input data retrieval interface, used to provide input data to other
 * parts of neic-glass3
 *
 * The iInput interface is a class interface implemented by class that
 * manages an input source for neic-glass three (such as an input file directory
 * or a HazDevBroker topic). This interface provides a method for other classes
 * in neic-glass3 to retrieve input messages via the getdata() function, and a
 * method for querying the pending data via the dataCount() function.
 */
class iInput {
 public:
	/**
	 * \brief Get input data
	 *
	 * This pure virtual function is implemented by a class to support retrieving
	 * input data managed by the class.
	 *
	 * \return Returns a std::shared_ptr to a json::object containing the input
	 * data.
	 */
	virtual std::shared_ptr<json::Object> getInputData() = 0;

	/**
	 * \brief Get count of remaining input data
	 *
	 * This pure virtual function is implemented by a class to support retrieving
	 * the current count of the remaining input data managed by the class.
	 *
	 * \return Returns an integer value containing the current count of
	 * remaining input data.
	 */
	virtual int getInputDataCount() = 0;
};
}  // namespace util
}  // namespace glass3
#endif  // INPUTINTERFACE_H
