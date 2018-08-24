#include <json.h>
#include <memory>
#include <string>
#include <utility>
#include <tuple>
#include <mutex>
#include <algorithm>
#include <vector>
#include <cmath>
#include "Node.h"
#include "Glass.h"
#include "Web.h"
#include "Trigger.h"
#include "Site.h"
#include "Pick.h"
#include "Date.h"
#include "Logit.h"

namespace glasscore {

// site Link sorting function
// Compares site links using travel times
bool sortSiteLink(const SiteLink &lhs, const SiteLink &rhs) {
	double travelTime1 = std::get< LINK_TT1>(lhs);
	if (travelTime1 < 0) {
		travelTime1 = std::get< LINK_TT2>(lhs);
	}

	double travelTime2 = std::get< LINK_TT1>(rhs);
	if (travelTime2 < 0) {
		travelTime2 = std::get< LINK_TT2>(rhs);
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
	std::lock_guard<std::recursive_mutex> nodeGuard(nodeMutex);

	clearSiteLinks();

	sName = "Nemo";
	pWeb = NULL;
	dLat = 0;
	dLon = 0;
	dZ = 0;
	dResolution = 0;
	sPid = "";
	bEnabled = false;
}

void CNode::clearSiteLinks() {
	if (vSite.size() == 0) {
		return;
	}

	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(vSiteMutex);

	// remove any links that sites have TO this node
	for (auto &link : vSite) {
		std::shared_ptr<CSite> aSite = std::get< LINK_PTR>(link);
		aSite->remNode(sPid);
	}

	// remove all the links from this node to sites
	vSite.clear();
}

bool CNode::initialize(std::string name, double lat, double lon, double z,
						double resolution, std::string nodeID) {
	std::lock_guard<std::recursive_mutex> nodeGuard(nodeMutex);

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
	std::lock_guard<std::mutex> guard(vSiteMutex);

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
		std::shared_ptr<CSite> currentSite = std::get< LINK_PTR>(link);

		// if the new station would be before
		// the current station
		if (currentSite->getScnl() == site->getScnl()) {
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
	std::lock_guard<std::mutex> guard(vSiteMutex);

	// unlink last site from node
	vSite.pop_back();

	// enable node
	bEnabled = true;

	return (true);
}

// ---------------------------------------------------------Nucleate
std::shared_ptr<CTrigger> CNode::nucleate(double tOrigin) {
	std::lock_guard<std::recursive_mutex> nodeGuard(nodeMutex);

	// nullchecks
	// check web
	if (pWeb == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CNode::nucleate: NULL web pointer.");
		return (NULL);
	}
	// check web pGlass
	if (pWeb->getGlass() == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CNode::nucleate: NULL web glass pointer.");
		return (NULL);
	}
	// don't nucleate if this node is disabled
	if (bEnabled == false) {
		return (NULL);
	}

	// get the cut and threshold from our
	// parent web
	int nCut = pWeb->getNucleate();
	double dThresh = pWeb->getThresh();
	double dAzimuthRange = pWeb->getGlass()->getBeamMatchingAzimuthWindow();
	// commented out because slowness matching of beams is not yet implemented
	// but is scheduled to be soon
	// double dDistanceRange = pWeb->pGlass->getBeamMatchingDistanceWindow();

	// init overall significance sum and node site count
	// to 0
	double dSum = 0.0;
	int nCount = 0;

	std::vector<std::shared_ptr<CPick>> vPick;

	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(vSiteMutex);

	// the best nucleating pick
	std::shared_ptr<CPick> pickBest;

	// search through each site linked to this node
	for (const auto &link : vSite) {
		// init sigbest
		double dSigBest = -1.0;

		// get shared pointer to site
		std::shared_ptr<CSite> site = std::get< LINK_PTR>(link);

		// Ignore if station out of service
		if (!site->getUse()) {
			continue;
		}
		if (!site->getEnable()) {
			continue;
		}

		std::vector<std::shared_ptr<CPick>> vSitePicks = site->getVPick();

		// search through each pick at this site
		for (const auto &pick : vSitePicks) {
			if (pick == NULL) {
				continue;
			}

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
				double siteAzimuth = pick->getSite()->getGeo().azimuth(
						&nodeGeo);

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
			 double siteDistance = pick->pSite->getGeo().delta(&nodeGeo);

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
			vPick.push_back(pickBest);
		}
	}

	// Depth Down-weighting
	// if (dZ > 75.) {
	// dSum = dSum / (dZ / 75.);  // 75 was empirically when testing
	//							   // events in Soda Springs
	// }

	// make sure the number of significant picks
	// exceeds the nucleation threshold
	if (nCount < nCut) {
		// the node did not nucleate an event
		return (NULL);
	}

	// make sure the total node significance exceeds the
	// significance threshold
	if (dSum < dThresh) {
		// the node did not nucleate an event
		return (NULL);
	}

	// create trigger
	std::shared_ptr<CTrigger> trigger(
			new CTrigger(dLat, dLon, dZ, tOrigin, dResolution, dSum, nCount,
							vPick, pWeb));

	// the node nucleated an event
	return (trigger);
}

double CNode::getBestSig(double tObservedTT, SiteLink link) {
	// get traveltime1 to site
	double travelTime1 = std::get< LINK_TT1>(link);

	// get traveltime2 to site
	double travelTime2 = std::get< LINK_TT2>(link);

	// use observed travel time, travel times to site, and a dT/dKm of
	// 0.1 s/km to calculate distance residuals
	double tRes1 = -1;
	if (travelTime1 > 0) {
		// calculate time residual
		tRes1 = std::abs(tObservedTT - travelTime1);

		// calculate distance residual
		// NOTE:  dT/dKm is hard coded
		// dRes1 = tRes1 / 0.1;
	}
	double tRes2 = -1;
	if (travelTime2 > 0) {
		// calculate time residual
		tRes2 = std::abs(tObservedTT - travelTime2);

		// calculate distance residual
		// NOTE:  dT/dKm is hard coded
		// dRes2 = tRes2 / 0.1;
	}
	// compute significances using residuals
	// pick sigma is defined as resolution / 5.0 * 2.0
	// should trigger be a looser cutoff than location cutoff
	double dSig1 = 0;
	if (tRes1 > 0) {
		dSig1 = pWeb->getGlass()->sig(tRes1, dResolution);
	}
	double dSig2 = 0;
	if (tRes2 > 0) {
		dSig2 = pWeb->getGlass()->sig(tRes2, dResolution);
	}
	// printf("getBestSig %.2f, %.2f, %.2f, %.2f, %.2f\n", tRes1, dSig1, tRes2,
	//	   dSig2, dResolution);
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
	std::lock_guard<std::mutex> guard(vSiteMutex);

	// NOTE: could be made more efficient (faster)
	// if we had a std::map
	// for all sites
	for (const auto &link : vSite) {
		// get the site
		auto aSite = std::get< LINK_PTR>(link);

		if (aSite->getScnl() == sScnl) {
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
	std::lock_guard<std::mutex> guard(vSiteMutex);

	SiteLink lastLink = vSite[vSite.size() - 1];
	std::shared_ptr<CSite> lastSite = std::get< LINK_PTR>(lastLink);

	// found
	return (lastSite);
}

void CNode::sortSiteLinks() {
	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(vSiteMutex);

	// sort sites
	sort(vSite.begin(), vSite.end(), sortSiteLink);
}

std::string CNode::getSitesString() {
	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(vSiteMutex);
	std::string siteString = "";

	// write to station file
	for (const auto &link : vSite) {
		// get the site
		std::shared_ptr<CSite> currentSite = std::get< LINK_PTR>(link);
		double lat, lon, r;

		currentSite->getGeo().getGeographic(&lat, &lon, &r);

		siteString += sPid + "," + currentSite->getScnl() + ","
				+ std::to_string(lat) + ";"
				+ std::to_string(lon) + ";"
				+ std::to_string(r) + "\n";
	}

	return (siteString);
}

int CNode::getSiteLinksCount() const {
	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(vSiteMutex);
	return (vSite.size());
}

bool CNode::getEnabled() const {
	return (bEnabled);
}

void CNode::setEnabled(bool enabled) {
	bEnabled = enabled;
}

double CNode::getLat() const {
	return (dLat);
}

double CNode::getLon() const {
	return (dLon);
}

double CNode::getResolution() const {
	return (dResolution);
}

double CNode::getZ() const {
	return (dZ);
}

glassutil::CGeo CNode::getGeo() const {
	glassutil::CGeo geoNode;
	geoNode.setGeographic(dLat, dLon, 6371.0 - dZ);
	return(geoNode);
}

CWeb* CNode::getWeb() const {
	std::lock_guard<std::recursive_mutex> nodeGuard(nodeMutex);
	return (pWeb);
}

void CNode::setWeb(CWeb* web) {
	std::lock_guard<std::recursive_mutex> nodeGuard(nodeMutex);
	pWeb = web;
}

const std::string& CNode::getName() const {
	return (sName);
}

const std::string& CNode::getPid() const {
	return (sPid);
}
}  // namespace glasscore
