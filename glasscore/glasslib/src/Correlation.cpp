#include <json.h>
#include <memory>
#include <string>
#include "Date.h"
#include "Pid.h"
#include "Web.h"
#include "Node.h"
#include "PickList.h"
#include "HypoList.h"
#include "Hypo.h"
#include "Pick.h"
#include "Correlation.h"
#include "Site.h"
#include "SiteList.h"
#include "Glass.h"
#include "Logit.h"

namespace glasscore {

// ---------------------------------------------------------CCorrelation
CCorrelation::CCorrelation() {
	clear();
}

// ---------------------------------------------------------CCorrelation
CCorrelation::CCorrelation(std::shared_ptr<CSite> correlationSite,
							double correlationTime, int correlationId,
							std::string correlationIdString, std::string phase,
							double orgTime, double orgLat, double orgLon,
							double orgZ, double corrVal) {
	clear();

	// nullcheck
	if (correlationSite == NULL) {
		return;
	}

	initialize(correlationSite, correlationTime, correlationId,
				correlationIdString, phase, orgTime, orgLat, orgLon, orgZ,
				corrVal);
}

// ---------------------------------------------------------CCorrelation
CCorrelation::CCorrelation(json::Object *correlation, int correlationId,
							CSiteList *pSiteList) {
	clear();

	// null check json
	if (correlation == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelation::CCorrelation: NULL json communication.");
		return;
	}

	// check cmd
	if (correlation->HasKey("Cmd")
			&& ((*correlation)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string cmd = (*correlation)["Cmd"].ToString();

		if (cmd != "Correlation") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CCorrelation::CCorrelation: Non-Correlation message passed"
					" in.");
			return;
		}
	} else if (correlation->HasKey("Type")
			&& ((*correlation)["Type"].GetType() == json::ValueType::StringVal)) {
		std::string type = (*correlation)["Type"].ToString();

		if (type != "Correlation") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CCorrelation::CCorrelation: Non-Correlation message passed"
					" in.");
			return;
		}
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelation::CCorrelation: Missing required Cmd or Type Key.");
		return;
	}

	// Correlation definition variables
	std::string sta = "";
	std::string comp = "";
	std::string net = "";
	std::string loc = "";
	std::shared_ptr<CSite> site = NULL;
	std::string ttt = "";
	double tcorr = 0;
	std::string pid = "";
	std::string phs = "";
	double tori = 0;
	double lat = 0;
	double lon = 0;
	double z = 0;
	double corr = 0;

	// site
	if (correlation->HasKey("Site")
			&& ((*correlation)["Site"].GetType() == json::ValueType::ObjectVal)) {
		// site is an object, create scnl string from it
		// get object
		json::Object siteobj = (*correlation)["Site"].ToObject();

		// site
		if (siteobj.HasKey("Station")
				&& (siteobj["Station"].GetType() == json::ValueType::StringVal)) {
			sta = siteobj["Station"].ToString();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CCorrelation::CCorrelation: Missing required Station Key.");
			return;
		}

		// comp (optional)
		if (siteobj.HasKey("Channel")
				&& (siteobj["Channel"].GetType() == json::ValueType::StringVal)) {
			comp = siteobj["Channel"].ToString();
		} else {
			comp = "";
		}

		// net
		if (siteobj.HasKey("Network")
				&& (siteobj["Network"].GetType() == json::ValueType::StringVal)) {
			net = siteobj["Network"].ToString();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CCorrelation::CCorrelation: Missing required Network Key.");
			return;
		}

		// loc (optional)
		if (siteobj.HasKey("Location")
				&& (siteobj["Location"].GetType() == json::ValueType::StringVal)) {
			loc = siteobj["Location"].ToString();
		} else {
			loc = "";
		}
	} else {
		// no site key
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelation::CCorrelation: Missing required Site Key.");
		return;
	}

	// lookup the site, if we have a sitelist available
	if (pSiteList) {
		site = pSiteList->getSite(sta, comp, net, loc);
	}

	// check to see if we got a site
	if (site == NULL) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CCorrelation::CCorrelation: site is null.");
		return;
	}

	// check to see if we're using this site
	if (!site->bUse) {
		return;
	}

	// time
	if (correlation->HasKey("Time")
			&& ((*correlation)["Time"].GetType() == json::ValueType::StringVal)) {
		// Time is formatted in iso8601, convert to julian seconds
		ttt = (*correlation)["Time"].ToString();
		glassutil::CDate dt = glassutil::CDate();
		tcorr = dt.decodeISO8601Time(ttt);
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelation::CCorrelation: Missing required Time Key.");
		return;
	}

	// pid
	// get the correlation id
	if (correlation->HasKey("ID")
			&& ((*correlation)["ID"].GetType() == json::ValueType::StringVal)) {
		pid = (*correlation)["ID"].ToString();
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CCorrelation::CCorrelation: Missing required ID Key.");
		return;
	}

	// phase
	if (correlation->HasKey("Phase")
			&& ((*correlation)["Phase"].GetType() == json::ValueType::StringVal)) {
		phs = (*correlation)["Phase"].ToString();
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelation::CCorrelation: Missing required Phase Key.");
		return;
	}

	// hypocenter
	if (correlation->HasKey("Hypocenter")
			&& ((*correlation)["Hypocenter"].GetType()
					== json::ValueType::ObjectVal)) {
		// site is an object, create scnl string from it
		// get object
		json::Object hypoobj = (*correlation)["Hypocenter"].ToObject();

		// latitude
		if (hypoobj.HasKey("Latitude")
				&& (hypoobj["Latitude"].GetType() == json::ValueType::DoubleVal)) {
			lat = hypoobj["Latitude"].ToDouble();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CCorrelation::CCorrelation: Missing required Hypocenter"
					" Latitude Key.");
			return;
		}

		// longitude
		if (hypoobj.HasKey("Longitude")
				&& (hypoobj["Longitude"].GetType() == json::ValueType::DoubleVal)) {
			lon = hypoobj["Longitude"].ToDouble();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CCorrelation::CCorrelation: Missing required Hypocenter"
					" Longitude Key.");
			return;
		}

		// z
		if (hypoobj.HasKey("Depth")
				&& (hypoobj["Depth"].GetType() == json::ValueType::DoubleVal)) {
			z = hypoobj["Depth"].ToDouble();
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CCorrelation::CCorrelation: Missing required Hypocenter"
					" Depth Key.");
			return;
		}

		// time
		// get the correlation time based on which key we find
		if (hypoobj.HasKey("Time")
				&& (hypoobj["Time"].GetType() == json::ValueType::StringVal)) {
			// Time is formatted in iso8601, convert to julian seconds
			ttt = hypoobj["Time"].ToString();
			glassutil::CDate dt = glassutil::CDate();
			tori = dt.decodeISO8601Time(ttt);
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CCorrelation::CCorrelation: Missing required Hypocenter"
					" Time Key.");
			return;
		}

	} else {
		// no hypocenter key
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelation::CCorrelation: Missing required Hypocenter Key.");
		return;
	}

	// correlation
	if (correlation->HasKey("Correlation")
			&& ((*correlation)["Correlation"].GetType()
					== json::ValueType::DoubleVal)) {
		corr = (*correlation)["Correlation"].ToDouble();
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CCorrelation::CCorrelation: Missing required Correlation Key.");
		return;
	}

	// pass to initialization function
	if (!initialize(site, tcorr, correlationId, pid, phs, tori, lat, lon, z,
					corr)) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelation::CCorrelation: Failed to initialize correlation.");
		return;
	}

	std::lock_guard<std::mutex> guard(correlationMutex);

	// remember input json for hypo message generation
	// note move to init?
	std::shared_ptr<json::Object> jcorr(new json::Object(*correlation));
	jCorrelation = jcorr;
}

// ---------------------------------------------------------~CCorrelation
CCorrelation::~CCorrelation() {
	clear();
}

// ---------------------------------------------------------clear
void CCorrelation::clear() {
	std::lock_guard<std::mutex> guard(correlationMutex);

	pSite = NULL;
	pHypo = NULL;
	jCorrelation = NULL;
	sAss = "";
	sPhs = "";
	sPid = "";
	idCorrelation = 0;
	tCorrelation = 0;
	tOrg = 0;
	dLat = 0;
	dLon = 0;
	dZ = 0;
	dCorrelation = 0;

	tGlassCreate = glassutil::CDate::now();
}

bool CCorrelation::initialize(std::shared_ptr<CSite> correlationSite,
								double correlationTime, int correlationId,
								std::string correlationIdString,
								std::string phase, double orgTime,
								double orgLat, double orgLon, double orgZ,
								double corrVal) {
	clear();

	// nullcheck
	if (correlationSite == NULL) {
		return (false);
	}

	std::lock_guard<std::mutex> guard(correlationMutex);

	pSite = correlationSite;
	sPhs = phase;
	sPid = correlationIdString;
	idCorrelation = correlationId;
	tCorrelation = correlationTime;
	tOrg = orgTime;

	// set geographic location
	dLat = orgLat;
	dLon = orgLon;
	dZ = orgZ;

	dCorrelation = corrVal;

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CCorrelation::initialize: pSite:" + pSite->sScnl
					+ "; tCorrelation:" + std::to_string(tCorrelation)
					+ "; idCorrelation:" + std::to_string(idCorrelation)
					+ "; sPid:" + sPid + "; sPhs:" + sPhs + "; tOrg:"
					+ std::to_string(tOrg) + "; dLat:" + std::to_string(dLat)
					+ "; dLon:" + std::to_string(dLon) + "; dZ:"
					+ std::to_string(dZ) + "; dCorrelation:"
					+ std::to_string(dCorrelation));

	return (true);
}

// ---------------------------------------------------------addHypo
void CCorrelation::addHypo(std::shared_ptr<CHypo> hyp, std::string ass,
							bool force) {
	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CCorrelation::addHypo: NULL hypo " "provided.");
		return;
	}

	std::lock_guard<std::mutex> guard(correlationMutex);

	// Add hypo data reference to this correlation
	if (force == true) {
		pHypo = hyp;
		sAss = ass;
	} else	if (!pHypo) {
		pHypo = hyp;
		sAss = ass;
	}
}

// ---------------------------------------------------------remHypo
void CCorrelation::remHypo(std::shared_ptr<CHypo> hyp) {
	// nullcheck
	if (hyp == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CCorrelation::remHypo: NULL hypo provided.");
		return;
	}

	std::lock_guard<std::mutex> guard(correlationMutex);

	// Remove hypo reference from this corrleation
	if (pHypo->sPid == hyp->sPid) {
		pHypo = NULL;
	}
}

void CCorrelation::clearHypo() {
	std::lock_guard<std::mutex> guard(correlationMutex);
	pHypo = NULL;
}

void CCorrelation::setAss(std::string ass) {
	std::lock_guard<std::mutex> guard(correlationMutex);

	sAss = ass;
}

}  // namespace glasscore
/*

 // ---------------------------------------------------------Process
 bool CCorrelation::process(json::Object *com) {

 // null check json
 if (com == NULL) {
 glassutil::CLogit::log(glassutil::log_level::error,
 "CCorrelation::process: NULL json communication.");
 return (false);
 }
 if (pGlass == NULL) {
 glassutil::CLogit::log(glassutil::log_level::error, "CCorrelation::process: NULL pGlass.");
 return (false);
 }

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
 string tiso = hypocenter["Time"].ToString();

 // convert time
 glassutil::CDate dt = glassutil::CDate();
 torg = dt.decodeISO8601Time(tiso);

 } else {
 glassutil::CLogit::log(glassutil::log_level::error,
 "CCorrelation::process: Missing required Hypocenter Time Key.");
 return (false);
 }

 // get latitude from hypocenter
 if (hypocenter.HasKey("Latitude")
 && (hypocenter["Latitude"].GetType()
 == json::ValueType::DoubleVal)) {

 lat = hypocenter["Latitude"].ToDouble();

 } else {
 glassutil::CLogit::log(glassutil::log_level::error,
 "CCorrelation::process: Missing required Hypocenter Latitude Key.");
 return (false);
 }

 // get longitude from hypocenter
 if (hypocenter.HasKey("Longitude")
 && (hypocenter["Longitude"].GetType()
 == json::ValueType::DoubleVal)) {

 lon = hypocenter["Longitude"].ToDouble();

 } else {
 glassutil::CLogit::log(glassutil::log_level::error,
 "CCorrelation::process: Missing required Hypocenter Longitude Key.");
 return (false);
 }

 // get depth from hypocenter
 if (hypocenter.HasKey("Depth")
 && (hypocenter["Depth"].GetType() == json::ValueType::DoubleVal)) {

 z = hypocenter["Depth"].ToDouble();

 } else {
 glassutil::CLogit::log(glassutil::log_level::error,
 "CCorrelation::process: Missing required Hypocenter Depth Key.");
 return (false);
 }

 } else {
 glassutil::CLogit::log(glassutil::log_level::error,
 "CCorrelation::process: Missing required Hypocenter Key.");
 return (false);
 }

 // create a correlation object
 shared_ptr<CORR> corr;
 corr->dLat = lat;
 corr->dLon = lon;
 corr->dz = z;
 corr->dTorg = torg;

 // save a pointer to the original message
 shared_ptr<json::Object> jcorr(new json::Object(*com));
 corr->jCorr = jcorr;

 // Check to see if hypo already exists. We could also
 // check location at this point, but it seems unlikely
 // that would add much value
 auto hypolist = pGlass->pHypoList;

 // define a two second search window
 // NOTE: Hard coded
 double t1 = torg - 1.0;
 double t2 = torg + 1.0;

 // search for the first hypocenter in the window
 shared_ptr<CHypo> hypo = hypolist->findHypo(t1, t2);

 // check to see if we found a hypo
 bool bgen = false;
 if (hypo == NULL) {
 // no matching hypo found
 bgen = true;
 } else {
 // found a hypo

 // calculate distance
 // NOTE: BUG! This is checking the distance to ITSELF?!
 // Shouldn't it be between the HYPO and the correlation?
 CGeo geo1;
 geo1.setGeographic(lat, lon, 6371.0);
 CGeo geo2;
 geo2.setGeographic(lat, lon, 6371.0);
 double delta = RAD2DEG * geo1.delta(&geo2);

 // if the correlation? is more than 0.1 degrees away, it isn't a match
 // NOTE: Hard coded.
 if (delta > 0.1) {
 bgen = true;
 }
 }

 // If no match or none exist, create and add Hypo
 if (bgen) {

 // create new hypo
 hypo = make_shared<CHypo>(lat, lon, z, torg, CPid::pid(), "Correlation",
 0.0, 0.0, 0);

 // set hypo glass pointer and such
 hypo->pGlass = pGlass;
 hypo->dCutFactor = pGlass->dCutFactor;
 hypo->dCutPercentage = pGlass->dCutPercentage;
 hypo->dCutMin = pGlass->dCutMin;

 // set this hypo as fixed since it came from a detection
 // NOTE: this seems problematic for association and relocation
 hypo->bFixed = true;

 // add correlation to hypo
 hypo->vCorr.push_back(corr);

 // add hypo to hypo list and schedule it for processing
 pGlass->pHypoList->addHypo(hypo);
 pGlass->pHypoList->pushFifo(hypo);
 return (true);
 }

 // Calculate new location and set as fixed to
 // allow for previously associated events.
 hypo->bFixed = true;

 // add correlation to hypo
 hypo->vCorr.push_back(corr);

 // Calculate new location
 // compute the average of all correlation's lat/lon/depth/time
 int nsum = 0;
 double latsum = lat;
 double lonsum = lon;
 double zsum = z;
 double tsum = torg;
 for (auto q : hypo->vCorr) {
 nsum++;
 latsum += q->dLat;
 lonsum += q->dLon;
 zsum += q->dz;
 tsum += q->dTorg;
 }

 // set the hypo to the average lat/lon/depth/time
 hypo->dLat = latsum / nsum;
 hypo->dLon = lonsum / nsum;
 hypo->dZ = zsum / nsum;
 hypo->tOrg = torg / nsum;

 // schedule hypo for processing
 pGlass->pHypoList->pushFifo(hypo);

 // NOTE: Shouldn't we call darwin? otherwise we're waiting for the next hypo
 // to be nucleated before we process.

 // done
 return (true);
 }*/

