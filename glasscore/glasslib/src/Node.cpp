#include <json.h>
#include <memory>
#include <string>
#include <utility>
#include <tuple>
#include <mutex>
#include <algorithm>
#include "Node.h"
#include "Glass.h"
#include "Web.h"
#include "Site.h"
#include "Pick.h"
#include "Date.h"
#include "Logit.h"

namespace glasscore {

// site Link sorting function
// Compares site links using travel times
bool sortSiteLink(const SiteLink &lhs, const SiteLink &rhs) {
	double travelTime1 = std::get < LINK_TT1 > (lhs);
	if (travelTime1 < 0) {
		travelTime1 = std::get < LINK_TT2 > (lhs);
	}

	double travelTime2 = std::get < LINK_TT1 > (rhs);
	if (travelTime2 < 0) {
		travelTime2 = std::get < LINK_TT2 > (rhs);
	}

	// compare
	if (travelTime1 < travelTime2) {
		return (true);
	}

	// travelTime2 > travelTime1
	return (false);
}

// ---------------------------------------------------------CNode
CNode::CNode() {
	clear();
}

// ---------------------------------------------------------CNode
CNode::CNode(std::string name, double lat, double lon, double z,
				double resolution, std::string nodeID) {
	if (!initialize(name, lat, lon, z, resolution, nodeID)) {
		clear();
	}
}

// ---------------------------------------------------------~CNode
CNode::~CNode() {
	clear();
}

// ---------------------------------------------------------clear
void CNode::clear() {
	clearSiteLinks();

	sName = "Nemo";
	pWeb = NULL;
	dLat = 0;
	dLon = 0;
	dZ = 0;
	dResolution = 0;
	tOrg = 0;
	nCount = 0;
	dSum = 0;
	nCount = 0;
	sPid = "";
	bEnabled = false;
}

void CNode::clearSiteLinks() {
	if (vSite.size() == 0) {
		return;
	}

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(vSiteMutex);

	// remove any links that sites have TO this node
	for (auto &link : vSite) {
		std::shared_ptr<CSite> aSite = std::get < LINK_PTR > (link);
		aSite->remNode(sPid);
	}

	// remove all the links from this node to sites
	vSite.clear();
}

bool CNode::initialize(std::string name, double lat, double lon, double z,
						double resolution, std::string nodeID) {
	clear();

	sName = name;
	dLat = lat;
	dLon = lon;
	dZ = z;
	dResolution = resolution;
	sPid = nodeID;
	bEnabled = true;

	return (true);
}

bool CNode::linkSite(std::shared_ptr<CSite> site, std::shared_ptr<CNode> node,
						double travelTime1, double travelTime2) {
	// nullchecks
	// check site
	if (site == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CNode::linkSite: NULL site pointer.");
		return (false);
	}
	// check node
	if (node == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CNode::linkSite: NULL node pointer.");
		return (false);
	}

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(vSiteMutex);

	// Link node to site using traveltime
	// NOTE: No validation on travel times
	SiteLink link = std::make_tuple(site, travelTime1, travelTime2);
	vSite.push_back(link);

	// link site to node, again using the traveltime
	// NOTE: this used to be site->addNode(shared_ptr<CNode>(this), tt);
	// but that caused problems when deleting site-node links.
	site->addNode(node, travelTime1, travelTime2);

	// successfully linked site
	return (true);
}

bool CNode::unlinkSite(std::shared_ptr<CSite> site) {
	// nullchecks
	// check site
	if (site == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CNode::unlinkSite: NULL site pointer.");
		return (false);
	}

	// lock while searching / modifing vSite
	vSiteMutex.lock();

	// search through each site linked to this node
	bool found = false;
	SiteLink foundLink;
	std::shared_ptr<CSite> foundSite;
	for (const auto &link : vSite) {
		// get the site
		std::shared_ptr<CSite> currentSite = std::get < LINK_PTR > (link);

		// if the new station would be before
		// the current station
		if (currentSite->sScnl == site->sScnl) {
			found = true;
			foundLink = link;
			foundSite = currentSite;

			// done
			break;
		}
	}

	if (found == true) {
		// find site iterator to remove
		auto it = std::find(vSite.begin(), vSite.end(), foundLink);
		if (it != vSite.end()) {
			// remove site
			// unlink site from node
			vSite.erase(it);

			// done modifying vSite
			vSiteMutex.unlock();

			// unlink node from site
			// done after unlock to avoid node-site deadlocks
			foundSite->remNode(sPid);

			return (true);
		}
	}

	// unlock before returning
	vSiteMutex.unlock();
	return (false);
}

bool CNode::unlinkLastSite() {
	// get the last site in the list
	std::shared_ptr<CSite> lastSite = getLastSite();

	if (lastSite == NULL) {
		return (false);
	}

	// unlink node from last site
	// done before lock guard to prevent
	// deadlock between node and site list mutexes.
	lastSite->remNode(sPid);

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(vSiteMutex);

	// unlink last site from node
	vSite.pop_back();

	// enable node
	bEnabled = true;

	return (true);
}

// ---------------------------------------------------------Nucleate
bool CNode::nucleate(double tOrigin, bool bList) {
	// nullchecks
	// check web
	if (pWeb == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CNode::nucleate: NULL web pointer.");
		return (false);
	}
	// check web pGlass
	if (pWeb->pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CNode::nucleate: NULL web glass pointer.");
		return (false);
	}
	// don't nucleate if this node is disabled
	if (bEnabled == false) {
		return (false);
	}

	tOrg = 0.0;

	// get the cut and threshold from our
	// parent web
	int nCut = pWeb->nNucleate;
	double dThresh = pWeb->dThresh;
	double dAzimuthRange = pWeb->pGlass->beamMatchingAzimuthWindow;
	// commented out because slowness matching of beams is not yet implemented
	// but is scheduled to be soon
	// double dDistanceRange = pWeb->pGlass->beamMatchingDistanceWindow;

	// init overall significance sum and node site count
	// to 0
	dSum = 0.0;
	nCount = 0;

	// clear the pick vector
	if (bList) {
		vPick.clear();
	}

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(vSiteMutex);

	// the best nucleating pick
	std::shared_ptr<CPick> pickBest;

	// search through each site linked to this node
	for (const auto &link : vSite) {
		// init sigbest
		double dSigBest = -1.0;

		// get shared pointer to site
		std::shared_ptr<CSite> site = std::get < LINK_PTR > (link);

		// Ignore if station out of service
		if (!site->bUse) {
			continue;
		}

		site->vPickMutex.lock();
		// search through each pick at this site
		for (const auto &pick : site->vPick) {
			// get the pick's arrival time
			double tPick = pick->getTPick();

			// get the picks back azimuth
			double backAzimuth = pick->getBackAzimuth();

			// compute observed travel time from the pick time and
			// the provided origin time
			double tObs = tPick - tOrigin;

			// Ignore arrivals past earlier than this potential origin and
			// past 1000 seconds (about 100 degrees)
			// NOTE: Time cutoff is hard coded
			if (tObs < 0 || tObs > 1000.0) {
				continue;
			}

			// check backazimuth if present
			if (backAzimuth > 0) {
				// set up a geo for distance calculations
				glassutil::CGeo nodeGeo;
				nodeGeo.setGeographic(dLat, dLon, 6371.0 - dZ);

				// compute azimith from the site to the node
				double siteAzimuth = pick->getSite()->geo.azimuth(&nodeGeo);

				// check to see if pick's backazimuth is within the
				// valid range
				if ((backAzimuth < (siteAzimuth - dAzimuthRange))
						|| (backAzimuth > (siteAzimuth + dAzimuthRange))) {
					// it is not, do not nucleate
					continue;
				}
			}

			// check slowness if present
			// Need modify travel time libraries to support getting distance
			// from slowness, and it's of limited value compared to the back
			// azimuth check
			/*if (pick->dSlowness > 0) {
			 // compute distance from the site to the node
			 double siteDistance = pick->pSite->geo.delta(&nodeGeo);

			 // compute observed distance from slowness (1/velocity)
			 // and tObs (distance = velocity * time)
			 double obsDistance = (1 / pick->dSlowness) * tObs;

			 // check to see if the observed distance is within the
			 // valid range
			 if ((obsDistance < (siteDistance - dDistanceRange))
			 || (obsDistance > (siteDistance + dDistanceRange))) {
			 // it is not, do not nucleate
			 continue;
			 }
			 }*/

			// get the best significance from the observed time and the
			// link
			double dSig = getBestSig(tObs, link);

			// only count if this pick is significant (better than
			// previous)
			if (dSig > dSigBest) {
				// keep the new best significance
				dSigBest = dSig;

				// remember the best pick
				pickBest = pick;
			}
		}

		site->vPickMutex.unlock();

		// check to see if the pick with the highest significance at this site
		// should be added to the overall sum from this site
		// NOTE: This significance threshold is hard coded.

		if (dSigBest >= 0.1) {
			// count this site
			nCount++;

			// add the best pick significance to the node
			// significance sum
			dSum += dSigBest;

			// add the pick to the pick vector
			if (bList) {
				vPick.push_back(pickBest);
			}
		}
	}

	// Depth Down-weighting
	if (dZ > 75.) {
		dSum = dSum / (dZ / 75.);  // 75 was empirically when testing
								   // events in Soda Springs
	}

	// make sure the number of significant picks
	// exceeds the nucleation threshold
	if (nCount < nCut) {
		// the node did not nucleate an event
		return (false);
	}

	// make sure the total node significance exceeds the
	// significance threshold
	if (dSum < dThresh) {
		// the node did not nucleate an event
		return (false);
	}

	// Log nucleation statistics
	if (bList) {
		// convert time to human readable
		glassutil::CDate dt = glassutil::CDate(tOrigin);
		std::string sOrg = dt.dateTime();
		char sLog[1024];
		snprintf(sLog, sizeof(sLog),
					"**Nucleate %s Web:%s Cut:%d Thresh:%.2f\n", sOrg.c_str(),
					pWeb->sName.c_str(), nCut, dSum);
		glassutil::CLogit::Out(sLog);
	}

	// remember tOrigin, it will be used when a hypocenter is created
	// from this node
	tOrg = tOrigin;

	// the node nucleated an event
	return (true);
}

double CNode::getBestSig(double tObservedTT, SiteLink link) {
	// get traveltime1 to site
	double travelTime1 = std::get < LINK_TT1 > (link);

	// get traveltime2 to site
	double travelTime2 = std::get < LINK_TT2 > (link);

	// use observed travel time, travel times to site, and a dT/dKm of
	// 0.1 s/km to calculate distance residuals
	double dRes1 = -1;
	if (travelTime1 > 0) {
		// calculate time residual
		double tRes1 = tObservedTT - travelTime1;

		// calculate distance residual
		// NOTE:  dT/dKm is hard coded
		dRes1 = tRes1 / 0.1;
	}
	double dRes2 = -1;
	if (travelTime2 > 0) {
		// calculate time residual
		double tRes2 = tObservedTT - travelTime2;

		// calculate distance residual
		// NOTE:  dT/dKm is hard coded
		dRes2 = tRes2 / 0.1;
	}

	// compute significances using distance residuals
	// and detection grid resolution
	double dSig1 = 0;
	if (dRes1 > 0) {
		dSig1 = pWeb->pGlass->sig(dRes1, dResolution);
	}
	double dSig2 = 0;
	if (dRes2 > 0) {
		dSig2 = pWeb->pGlass->sig(dRes2, dResolution);
	}

	// return the higher of the two significances
	if (dSig1 > dSig2) {
		return (dSig1);
	} else {
		return (dSig2);
	}
}

std::shared_ptr<CSite> CNode::getSite(std::string sScnl) {
	if (sScnl == "") {
		return (NULL);
	}

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(vSiteMutex);

	// NOTE: could be made more efficient (faster)
	// if we had a std::map
	// for all sites
	for (const auto &link : vSite) {
		// get the site
		auto aSite = std::get < LINK_PTR > (link);

		if (aSite->sScnl == sScnl) {
			// found
			return (aSite);
		}
	}

	// not found
	return (NULL);
}

std::shared_ptr<CSite> CNode::getLastSite() {
	if (getSiteLinksCount() == 0) {
		return (NULL);
	}

	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(vSiteMutex);

	SiteLink lastLink = vSite[vSite.size() - 1];
	std::shared_ptr<CSite> lastSite = std::get < LINK_PTR > (lastLink);

	// found
	return (lastSite);
}

void CNode::sortSiteLinks() {
	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(vSiteMutex);

	// sort sites
	sort(vSite.begin(), vSite.end(), sortSiteLink);
}

std::string CNode::getSitesString() {
	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(vSiteMutex);
	std::string siteString = "";

	// write to station file
	for (const auto &link : vSite) {
		// get the site
		std::shared_ptr<CSite> currentSite = std::get < LINK_PTR > (link);

		siteString += sPid + "," + currentSite->sScnl + ","
				+ std::to_string(currentSite->geo.dLat) + ";"
				+ std::to_string(currentSite->geo.dLon) + ";"
				+ std::to_string(currentSite->geo.dRad) + "\n";
	}

	return (siteString);
}

int CNode::getSiteLinksCount() {
	// lock mutex for this scope
	std::lock_guard < std::mutex > guard(vSiteMutex);
	int size = vSite.size();

	return (size);
}
}  // namespace glasscore
