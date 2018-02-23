#ifndef OUTPUTINTERFACE_H
#define OUTPUTINTERFACE_H

#include <json.h>
#include <memory>

namespace util {

/**
 * \interface ioutput
 * \brief output data interface
 *
 * The ioutput interface is implemented by concrete classes that
 * outputs data from the associator, providing a standardized
 * interface for other classes in glass to request the sending
 * of output data via the sendtooutput() function.
 */
struct iOutput {
	/**
	 * \brief Send output data
	 *
	 * This pure virtual function is implemented by a concrete class to
	 * support sending output data.
	 *
	 * \param data - A pointer to a json::object containing the output
	 * data.
	 */
	virtual void sendToOutput(std::shared_ptr<json::Object> data) = 0;
};
}  // namespace util
#endif  // OUTPUTINTERFACE_H
