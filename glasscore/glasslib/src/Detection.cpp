#include <json.h>
#include <string>
#include <memory>
#include "Date.h"
#include "Pid.h"
#include "Web.h"
#include "Node.h"
#include "PickList.h"
#include "HypoList.h"
#include "Hypo.h"
#include "Pick.h"
#include "Detection.h"
#include "Site.h"
#include "Glass.h"
#include "Logit.h"

namespace glasscore {

// ---------------------------------------------------------CDetection
CDetection::CDetection() {
	clear();
}

// ---------------------------------------------------------~CDetection
CDetection::~CDetection() {
	clear();
}

// ---------------------------------------------------------clear
void CDetection::clear() {
	std::lock_guard<std::recursive_mutex> detectionGuard(detectionMutex);
	pGlass = NULL;
}

// ---------------------------------------------------------Dispatch
bool CDetection::dispatch(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CDetection::dispatch: NULL json communication.");
		return (false);
	}

	// check for a command
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// clear all data
		if (v == "ClearGlass") {
			// ClearGlass is also relevant to other glass
			// components, return false so they also get a
			// chance to process it
			return (false);
		}
	}

	// Input data can have Type keys
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Type"].ToString();

		// add a detection
		if (v == "Detection") {
			return (process(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------process
bool CDetection::process(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CDetection::process: NULL json communication.");
		return (false);
	}
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CDetection::process: NULL pGlass.");

		return (false);
	}

	std::lock_guard<std::recursive_mutex> detectionGuard(detectionMutex);

	// detection definition variables
	double torg = 0;
	double lat = 0;
	double lon = 0;
	double z = 0;

	// Get information from hypocenter
	if (com->HasKey("Hypocenter")
			&& ((*com)["Hypocenter"].GetType() == json::ValueType::ObjectVal)) {
		json::Object hypocenter = (*com)["Hypocenter"].ToObject();

		// get time from hypocenter
		if (hypocenter.HasKey("Time")
				&& (hypocenter["Time"].GetType() == json::ValueType::StringVal)) {
			// get time string
			std::string tiso = hypocenter["Time"].ToString();

			// convert time
			glassutil::CDate dt = glassutil::CDate();
			torg = dt.decodeISO8601Time(tiso);
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CDetection::process: Missing required Hypocenter Time Key.");

			return (false);
		}

		// get latitude from hypocenter
		if (hypocenter.HasKey("Latitude")
				&& (hypocenter["Latitude"].GetType()
						== json::ValueType::DoubleVal)) {
			lat = hypocenter["Latitude"].ToDouble();

		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CDetection::process: Missing required Hypocenter Latitude"
					" Key.");

			return (false);
		}

		// get longitude from hypocenter
		if (hypocenter.HasKey("Longitude")
				&& (hypocenter["Longitude"].GetType()
						== json::ValueType::DoubleVal)) {
			lon = hypocenter["Longitude"].ToDouble();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CDetection::process: Missing required Hypocenter Longitude"
					" Key.");

			return (false);
		}

		// get depth from hypocenter
		if (hypocenter.HasKey("Depth")
				&& (hypocenter["Depth"].GetType() == json::ValueType::DoubleVal)) {
			z = hypocenter["Depth"].ToDouble();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CDetection::process: Missing required Hypocenter Depth"
					" Key.");

			return (false);
		}
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CDetection::process: Missing required Hypocenter Key.");

		return (false);
	}

	// Check to see if hypo already exists. We could also
	// check location at this point, but it seems unlikely
	// that would add much value
	// define a three minute search window
	// NOTE: Hard coded.
	double t1 = torg - 90.0;
	double t2 = torg + 90.0;

	// search for the first hypocenter in the window
	std::shared_ptr<CHypo> hypo = pGlass->getHypoList()->findHypo(t1, t2);

	// check to see if we found a hypo
	if (hypo != NULL) {
		// found a hypo
		// calculate distance
		glassutil::CGeo geo1;
		geo1.setGeographic(lat, lon, z);
		glassutil::CGeo geo2;
		geo2.setGeographic(hypo->getLat(), hypo->getLon(), hypo->getZ());
		double delta = RAD2DEG * geo1.delta(&geo2);

		// if the detection is more than 5 degrees away, it isn't a match
		// NOTE: Hard coded.
		if (delta > 5.0) {
			// detections don't have a second travel time
			traveltime::CTravelTime* nullTrav = NULL;

			// create new hypo
			// pGlass->m_TTTMutex.lock();
			hypo = std::make_shared<CHypo>(lat, lon, z, torg,
											glassutil::CPid::pid(), "Detection",
											0.0, 0.0, 0,
											pGlass->getTrvDefault().get(),
											nullTrav, pGlass->getTTT());
			// pGlass->m_TTTMutex.unlock();

			// set hypo glass pointer and such
			hypo->setGlass(pGlass);
			hypo->setCutFactor(pGlass->getCutFactor());
			hypo->setCutPercentage(pGlass->getCutPercentage());
			hypo->setCutMin(pGlass->getCutMin());

			// process hypo using evolve
			if (pGlass->getHypoList()->evolve(hypo, 1)) {
				// add to hypo list
				pGlass->getHypoList()->addHypo(hypo);
			}
		} else {
			// existing hypo, now hwat?
			// schedule hypo for processing?
			pGlass->getHypoList()->pushFifo(hypo);
		}
	}

	// done
	return (true);
}

const CGlass* CDetection::getGlass() const {
	std::lock_guard<std::recursive_mutex> detectionGuard(detectionMutex);
	return (pGlass);
}

void CDetection::setGlass(CGlass* glass) {
	std::lock_guard<std::recursive_mutex> detectionGuard(detectionMutex);
	pGlass = glass;
}
}  // namespace glasscore
