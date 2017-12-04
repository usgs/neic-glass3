/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef DETECTION_H
#define DETECTION_H

#include <json.h>
#include <memory>
#include <string>
#include <vector>

namespace glasscore {

// forward declarations
class CSite;
class CHypo;

/**
 * \brief glasscore pick class
 *
 * The CDetection class is the class that encapsulates everything necessary
 * to represent a waveform arrival pick, including arrival time, phase id,
 * and an unique identifier.  The CDetection class is also a node in the
 * detection graph database.
 *
 * CDetection contains functions to support nucleation of a new event based
 * on the pick.
 *
 * CDetection maintains a graph database link between it and the the site
 * (station) the pick was made at.
 *
 * CDetection also maintains a vector of CHypo objects represent the graph
 * database links between  this pick and various hypocenters.  A single pick may
 * be linked to multiple hypocenters
 *
 * CDetection uses smart pointers (std::shared_ptr).
 */
class CDetection {
 public:
	/**
	 * \brief CDetection constructor
	 */
	CDetection();

	/**
	 * \brief CDetection destructor
	 */
	~CDetection();

	/**
	 * \brief CDetection clear function
	 */
	void clear();

	/**
	 * \brief CDetection communication receiving function
	 *
	 * The function used by CDetection to receive communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports processing Detection messages
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CDetection,
	 * false otherwise
	 */
	bool dispatch(json::Object *com);

	/**
	 * \brief Process detection message
	 *
	 * Receives an incomding 'Detection' type message and
	 * does on of two things. First it checks to see if there
	 * is another hypocenter that is near enough in space and
	 * time to be considered to be the same event. If so, it
	 * adds the correlation information to the hypocenters
	 * vCorr vector and sets the bFixed to true. If the hypocenter
	 * clready has a non-zero length vCorr, then the new
	 * correlation data is added and a median location is
	 * calculated. If no existing quake fits the information in
	 * the correlation message, a new hypocenter is created with
	 * a fixed location.
	 *
	 * \param com -  A pointer to a json::object containing the incoming
	 * 'Detection' message
	 */
	bool process(json::Object *com);

	// Local Attributes
	/**
	 * \brief A pointer to the parent CGlass class, used to send output,
	 * look up site information, encode/decode time, get configuration
	 * values, call association functions, and debug flags
	 */
	CGlass *pGlass;
};
}  // namespace glasscore
#endif  // DETECTION_H
