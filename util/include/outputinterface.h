#ifndef OUTPUTINTERFACE_H
#define OUTPUTINTERFACE_H

#include <json.h>
#include <memory>

namespace glass3 {
namespace util {

/**
 * \interface iOutput
 * \brief The output message interface, used in sending data to output classes in
 * neic-glass3
 *
 * The iOutput interface is implemented by classes that manage outputting
 * data from neic-glass3, providing a method for other classes in neic-glass3 to
 * send output data via the sendtooutput() function.
 */
class iOutput {
 public:
	/**
	 * \brief Send output data
	 *
	 * This pure virtual function is implemented by a class to support sending
	 * data to be output.
	 *
	 * \param data - A std::shared_ptr to a json::object containing the output
	 * data.
	 */
	virtual void sendToOutput(std::shared_ptr<json::Object> data) = 0; // NOLINT
};
}  // namespace util
}  // namespace glass3
#endif  // OUTPUTINTERFACE_H
