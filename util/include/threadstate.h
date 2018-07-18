/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
/**
 * \file
 * \brief threadstatus.h
 *
 * threadstatus.h contains the ThreadState enum class
 */
#ifndef THREADSTATE_H
#define THREADSTATE_H

namespace glass3 {
namespace util {

/**
 * \brief util ThreadState enumeration
 *
 * This enumeration defines the values used in tracking the state of a thread.
 */
enum ThreadState {
	Initialized = 0, /**< thread has been initialized */
	Starting = 1, /**< thread is starting up */
	Started = 2, /**< thread has been started */
	Stopping = -1, /**< thread is shutting down */
	Stopped = -2	/**< thread is shut down */
};
}  // namespace util
}  // namespace glass3
#endif  // THREADSTATE_H
