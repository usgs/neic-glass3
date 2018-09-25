#include "Detection.h"
#include <json.h>
#include <date.h>
#include <geo.h>
#include <logger.h>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include "Web.h"
#include "Node.h"
#include "PickList.h"
#include "HypoList.h"
#include "Hypo.h"
#include "Pick.h"
#include "Site.h"
#include "Glass.h"

namespace glasscore {

// ---------------------------------------------------------CDetection
CDetection::CDetection() {
}

// ---------------------------------------------------------~CDetection
CDetection::~CDetection() {
}

// -------------------------------------------------------receiveExternalMessage
bool CDetection::receiveExternalMessage(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
				"CDetection::receiveExternalMessage: NULL json communication.");
		return (false);
	}

	// Input data can have Type keys
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Type"].ToString();

		// add a detection
		if (v == "Detection") {
			return (processDetectionMessage(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------process
bool CDetection::processDetectionMessage(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error", "CDetection::process: NULL json communication.");
		return (false);
	}

	std::lock_guard<std::recursive_mutex> detectionGuard(m_DetectionMutex);

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
			glass3::util::Date dt = glass3::util::Date();
			torg = dt.decodeISO8601Time(tiso);
		} else {
			glass3::util::Logger::log(
					"error",
					"CDetection::process: Missing required Hypocenter Time Key.");

			return (false);
		}

		// get latitude from hypocenter
		if (hypocenter.HasKey("Latitude")
				&& (hypocenter["Latitude"].GetType()
						== json::ValueType::DoubleVal)) {
			lat = hypocenter["Latitude"].ToDouble();

		} else {
			glass3::util::Logger::log(
					"error",
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
			glass3::util::Logger::log(
					"error",
					"CDetection::process: Missing required Hypocenter Longitude"
					" Key.");

			return (false);
		}

		// get depth from hypocenter
		if (hypocenter.HasKey("Depth")
				&& (hypocenter["Depth"].GetType() == json::ValueType::DoubleVal)) {
			z = hypocenter["Depth"].ToDouble();
		} else {
			glass3::util::Logger::log(
					"error",
					"CDetection::process: Missing required Hypocenter Depth"
					" Key.");

			return (false);
		}
	} else {
		glass3::util::Logger::log(
				"error",
				"CDetection::process: Missing required Hypocenter Key.");

		return (false);
	}

	// Check to see if hypo already exists. We could also
	// check location at this point, but it seems unlikely
	// that would add much value
	// use the merging time window
	double t1 = torg - CGlass::getHypoMergingTimeWindow();
	double t2 = torg + CGlass::getHypoMergingTimeWindow();

	std::shared_ptr<CHypo> hypo = NULL;
	bool match = false;

	// search for the first hypocenter in the window
	std::vector<std::weak_ptr<CHypo>> hypos = CGlass::getHypoList()->getHypos(
			t1, t2);

	// check to see if we found a hypo
	if (hypos.size() > 0) {
		std::shared_ptr<CHypo> hypo = hypos[0].lock();

		if (hypo != NULL) {
			// found a hypo
			// calculate distance
			glass3::util::Geo geo1;
			geo1.setGeographic(lat, lon, z);
			glass3::util::Geo geo2;
			geo2.setGeographic(hypo->getLatitude(), hypo->getLongitude(),
								hypo->getDepth());
			double delta = RAD2DEG * geo1.delta(&geo2);

			// if the detection is beyond the merging window, it isn't a match
			if (delta < CGlass::getHypoMergingDistanceWindow()) {
				match = true;
			}
		}
	}

	if (match == true) {
		// existing hypo, now hwat?
		// schedule hypo for processing?
		CGlass::getHypoList()->appendToHypoProcessingQueue(hypo);
	} else {
		// detections don't have a second travel time
		std::shared_ptr<traveltime::CTravelTime> nullTrav;

		// create new hypo
		// Get primary nucleation TT from CGlass. Set secondary to NULL since
		// CGLASS only supports a single default nucleation travel time.
		hypo = std::make_shared<CHypo>(
				com, 0.0, 0, CGlass::getDefaultNucleationTravelTime(), nullTrav,
				CGlass::getAssociationTravelTimes());

		// set thresholds
		hypo->setNucleationDataThreshold(CGlass::getNucleationDataThreshold());
		hypo->setNucleationStackThreshold(
				CGlass::getNucleationStackThreshold());

		// process hypo using hypolist
		if (CGlass::getHypoList()->processHypo(hypo)) {
			// add to hypo list
			CGlass::getHypoList()->addHypo(hypo);
		}
	}

	// done
	return (true);
}
}  // namespace glasscore
