/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef IGLASSSEND_H
#define IGLASSSEND_H

#include <json.h>
#include <memory>

namespace glasscore {

/**
 * \interface IGlassSend
 * \brief glasscore messaging interface
 *
 * The IGlassSend interface is implemented by concrete classes
 * providing a standardized interface for other classes to send
 * information to the glasscore library via the Send()
 * function.
 */
struct IGlassSend {
	/**
	 * \brief Send data to glasscore
	 *
	 * This pure virtual function is implemented by a concrete class to
	 * receive data for glasscore.
	 *
	 * \param com - A pointer to a json::object containing the data
	 * to send to glasscore.
	 */
	virtual void Send(std::shared_ptr<json::Object> com) = 0;
};
}  // namespace glasscore
#endif  // IGLASSSEND_H
