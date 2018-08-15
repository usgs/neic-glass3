/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
/**
 * \file
 * \brief workstate.h
 *
 * workstate.h contains the WorkState enumeration
 */
#ifndef WORKSTATE_H
#define WORKSTATE_H

namespace glass3 {
namespace util {

/**
 * \brief glass3::util::WorkState enumeration
 *
 * This enumeration defines the values used in tracking the state of a work()
 * call in ThreadBaseClass.
 */
enum WorkState {
	Idle = 0, /**< There was no work to perform */
	OK = 1, /**< The work was completed successfully */
	Error = -1 /**< There was an error performing the work */
};
}  // namespace util
}  // namespace glass3
#endif  // WORKSTATE_H
