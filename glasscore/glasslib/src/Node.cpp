#include <json.h>
#include <memory>
#include <string>
#include <utility>
#include <tuple>
#include <limits>
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
#include "GlassMath.h"

#define NUCLEATION_SLOP_FACTOR_SECONDS 60.0

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
				double resolution) {
	if (!initialize(name, lat, lon, z, resolution)) {
		clear();
	}
}

// ---------------------------------------------------------~CNode
CNode::~CNode() {
	clear();
}

// ---------------------------------------------------------clear
void CNode::clear() {
	std::lock_guard<std::recursive_mutex> nodeGuard(m_NodeMutex);

	clearSiteLinks();

	m_sName = "Nemo";
	m_pWeb = NULL;
	m_dLatitude = 0;
	m_dLongitude = 0;
	m_dDepth = 0;
	m_dResolution = 0;
	m_bEnabled = false;
}

// ---------------------------------------------------------clearSiteLinks
void CNode::clearSiteLinks() {
	if (m_vSiteLinkList.size() == 0) {
		return;
	}

	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(m_SiteLinkListMutex);

	// remove any links that sites have TO this node
	for (auto &link : m_vSiteLinkList) {
		std::shared_ptr<CSite> aSite = std::get< LINK_PTR>(link);
		aSite->removeNode(getID());
	}

	// remove all the links from this node to sites
	m_vSiteLinkList.clear();
}

// ---------------------------------------------------------initialize
bool CNode::initialize(std::string name, double lat, double lon, double z,
						double resolution) {
	std::lock_guard<std::recursive_mutex> nodeGuard(m_NodeMutex);

	clear();

	m_sName = name;
	m_dLatitude = lat;
	m_dLongitude = lon;
	m_dDepth = z;
	m_dResolution = resolution;
	m_bEnabled = true;

	return (true);
}

// ---------------------------------------------------------linkSite
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
	std::lock_guard<std::mutex> guard(m_SiteLinkListMutex);

	// Link node to site using traveltime
	// NOTE: No validation on travel times
	SiteLink link = std::make_tuple(site, travelTime1, travelTime2);
	m_vSiteLinkList.push_back(link);

	// link site to node, again using the traveltime
	// NOTE: this used to be site->addNode(shared_ptr<CNode>(this), tt);
	// but that caused problems when deleting site-node links.
	site->addNode(node, travelTime1, travelTime2);

	// successfully linked site
	return (true);
}

// ---------------------------------------------------------unlinkSite
bool CNode::unlinkSite(std::shared_ptr<CSite> site) {
	// nullchecks
	// check site
	if (site == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CNode::unlinkSite: NULL site pointer.");
		return (false);
	}

	// lock while searching / modifing vSite
	m_SiteLinkListMutex.lock();

	// search through each site linked to this node
	bool found = false;
	SiteLink foundLink;
	std::shared_ptr<CSite> foundSite;
	for (const auto &link : m_vSiteLinkList) {
		// get the site
		std::shared_ptr<CSite> currentSite = std::get< LINK_PTR>(link);

		// if the new station would be before
		// the current station
		if (currentSite->getSCNL() == site->getSCNL()) {
			found = true;
			foundLink = link;
			foundSite = currentSite;

			// done
			break;
		}
	}

	if (found == true) {
		// find site iterator to remove
		auto it = std::find(m_vSiteLinkList.begin(), m_vSiteLinkList.end(),
							foundLink);
		if (it != m_vSiteLinkList.end()) {
			// remove site
			// unlink site from node
			m_vSiteLinkList.erase(it);

			// done modifying vSite
			m_SiteLinkListMutex.unlock();

			// unlink node from site
			// done after unlock to avoid node-site deadlocks
			foundSite->removeNode(getID());

			return (true);
		}
	}

	// unlock before returning
	m_SiteLinkListMutex.unlock();
	return (false);
}

// ---------------------------------------------------------unlinkLastSite
bool CNode::unlinkLastSite() {
	// get the last site in the list
	std::shared_ptr<CSite> lastSite = getLastSite();

	if (lastSite == NULL) {
		return (false);
	}

	// unlink node from last site
	// done before lock guard to prevent
	// deadlock between node and site list mutexes.
	lastSite->removeNode(getID());

	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(m_SiteLinkListMutex);

	// unlink last site from node
	m_vSiteLinkList.pop_back();

	// enable node
	m_bEnabled = true;

	return (true);
}

// ---------------------------------------------------------nucleate
std::shared_ptr<CTrigger> CNode::nucleate(double tOrigin) {
	std::lock_guard<std::recursive_mutex> nodeGuard(m_NodeMutex);

	// nullchecks
	// check web
	if (m_pWeb == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CNode::nucleate: NULL web pointer.");
		return (NULL);
	}
	// don't nucleate if this node is disabled
	if (m_bEnabled == false) {
		return (NULL);
	}

	// get the cut and threshold from our
	// parent web
	int nCut = m_pWeb->getNucleationDataThreshold();
	double dThresh = m_pWeb->getNucleationStackThreshold();
	double dAzimuthRange = CGlass::getBeamMatchingAzimuthWindow();
	// commented out because slowness matching of beams is not yet implemented
	// but is scheduled to be soon
	// double dDistanceRange = CGlass::getBeamMatchingDistanceWindow();

	// init overall significance sum and node site count
	// to 0
	double dSum = 0.0;
	int nCount = 0;

	std::vector<std::shared_ptr<CPick>> vPick;

	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(m_SiteLinkListMutex);

	// the best nucleating pick
	std::shared_ptr<CPick> pickBest;

	// search through each site linked to this node
	for (const auto &link : m_vSiteLinkList) {
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

		// get traveltime(s) to site
		double travelTime1 = std::get< LINK_TT1>(link);
		double travelTime2 = std::get< LINK_TT2>(link);

		/*		// compute pick time window from travelime(s)
		 double t1 = 0;
		 double t2 = 0;
		 if ((travelTime1 > 0) && (travelTime2 > 0)) {
		 // both travel times are valid
		 t1 = tOrigin
		 + std::min(travelTime1,
		 travelTime2) - NUCLEATION_SLOP_FACTOR_SECONDS;
		 t2 = tOrigin
		 + std::max(travelTime1,
		 travelTime2) + NUCLEATION_SLOP_FACTOR_SECONDS;
		 } else if ((travelTime1 > 0) && (travelTime2 < 0)) {
		 // only tt1 is valid
		 t1 = tOrigin + travelTime1 - NUCLEATION_SLOP_FACTOR_SECONDS;
		 t2 = tOrigin + travelTime1 + NUCLEATION_SLOP_FACTOR_SECONDS;
		 } else if ((travelTime1 < 0) && (travelTime2 > 0)) {
		 // only tt2 is valid
		 t1 = tOrigin + travelTime2 - NUCLEATION_SLOP_FACTOR_SECONDS;
		 t2 = tOrigin + travelTime2 + NUCLEATION_SLOP_FACTOR_SECONDS;
		 } else {
		 // no valid tt
		 continue;
		 }
		 */
		double min = std::numeric_limits<double>::max();
		double max = std::numeric_limits<double>::max();

		double t1Min = std::numeric_limits<double>::max();
		double t1Max = std::numeric_limits<double>::max();
		if (travelTime1 > 0) {
			t1Min = tOrigin + travelTime1 - NUCLEATION_SLOP_FACTOR_SECONDS;
			t1Max = tOrigin + travelTime1 + NUCLEATION_SLOP_FACTOR_SECONDS;
		}

		double t2Min = std::numeric_limits<double>::max();
		double t2Max = std::numeric_limits<double>::min();
		if (travelTime2 > 0) {
			t2Min = tOrigin + travelTime2 - NUCLEATION_SLOP_FACTOR_SECONDS;
			t2Max = tOrigin + travelTime2 + NUCLEATION_SLOP_FACTOR_SECONDS;
		}

		// if t1 is smaller than t2
		if (t1Min < t2Min) {
			min = t1Min;
		} else {
			min = t2Min;
		}

		// if t1 is larger than t2
		if (t1Max > t2Max) {
			max = t1Max;
		} else {
			max = t2Max;
		}

		// check range
		if ((min == std::numeric_limits<double>::max())
				|| (max == std::numeric_limits<double>::min())) {
			glassutil::CLogit::log(glassutil::log_level::error,
									"CNode::nucleate: Bad Pick SearchRange.");
			continue;
		}

		site->getPickMutex().lock();

		// compute bounds
		auto lower = site->getLower(min);
		auto upper = site->getUpper(max);

		// get the picks
		// std::vector<std::shared_ptr<CPick>> vSitePicks = site->getPicks(min,
		// max);

		// search through each pick in the window
		// for (const auto &pick : vSitePicks) {

		for (auto it = lower; ((it != upper) && (it != site->getEnd())); ++it) {
			auto pick = *it;

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

			// Ignore arrivals earlier than this potential origin and
			// past 1000 seconds (about 100 degrees)
			// NOTE: Time cutoff is hard coded
			if (tObs < 0 || tObs > 1000.0) {
				continue;
			}

			// check backazimuth if present
			if (backAzimuth > 0) {
				// set up a geo for distance calculations
				glassutil::CGeo nodeGeo;
				nodeGeo.setGeographic(m_dLatitude, m_dLongitude,
				EARTHRADIUSKM - m_dDepth);

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
			double dSig = getBestSignificance(tObs, travelTime1, travelTime2);

			// only count if this pick is significant (better than
			// previous)
			if (dSig > dSigBest) {
				// keep the new best significance
				dSigBest = dSig;

				// remember the best pick
				pickBest = pick;
			}
		}  // ---- end search through each pick at this site ----

		site->getPickMutex().unlock();

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
			new CTrigger(m_dLatitude, m_dLongitude, m_dDepth, tOrigin,
							m_dResolution, dSum, nCount, vPick, m_pWeb));

	// the node nucleated an event
	return (trigger);
}

// ---------------------------------------------------------getBestSignificance
double CNode::getBestSignificance(double tObservedTT, double travelTime1,
									double travelTime2) {
	// use observed travel time, travel times to site
	double tRes1 = -1;
	if (travelTime1 > 0) {
		// calculate time residual
		tRes1 = std::abs(tObservedTT - travelTime1);
	}
	double tRes2 = -1;
	if (travelTime2 > 0) {
		// calculate time residual
		tRes2 = std::abs(tObservedTT - travelTime2);
	}

	// compute significances using residuals and web resolution
	// should trigger be a looser cutoff than location cutoff
	double dSig1 = 0;
	if (tRes1 > 0) {
		dSig1 = glassutil::GlassMath::sig(tRes1, m_dResolution);
	}
	double dSig2 = 0;
	if (tRes2 > 0) {
		dSig2 = glassutil::GlassMath::sig(tRes2, m_dResolution);
	}

	// return the higher of the two significances
	if (dSig1 > dSig2) {
		return (dSig1);
	} else {
		return (dSig2);
	}
}

// ---------------------------------------------------------getSite
std::shared_ptr<CSite> CNode::getSite(std::string siteID) {
	if (siteID == "") {
		return (NULL);
	}

	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(m_SiteLinkListMutex);

	// NOTE: could be made more efficient (faster)
	// if we had a std::map
	// for all sites
	for (const auto &link : m_vSiteLinkList) {
		// get the site
		auto aSite = std::get< LINK_PTR>(link);

		if (aSite->getSCNL() == siteID) {
			// found
			return (aSite);
		}
	}

	// not found
	return (NULL);
}

// ---------------------------------------------------------getLastSite
std::shared_ptr<CSite> CNode::getLastSite() {
	if (getSiteLinksCount() == 0) {
		return (NULL);
	}

	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(m_SiteLinkListMutex);

	SiteLink lastLink = m_vSiteLinkList[m_vSiteLinkList.size() - 1];
	std::shared_ptr<CSite> lastSite = std::get< LINK_PTR>(lastLink);

	// found
	return (lastSite);
}

// ---------------------------------------------------------sortSiteLinks
void CNode::sortSiteLinks() {
	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(m_SiteLinkListMutex);

	// sort sites
	sort(m_vSiteLinkList.begin(), m_vSiteLinkList.end(), sortSiteLink);
}

// ---------------------------------------------------------getSitesString
std::string CNode::getSitesString() {
	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(m_SiteLinkListMutex);
	std::string siteString = "";

	// write to station file
	for (const auto &link : m_vSiteLinkList) {
		// get the site
		std::shared_ptr<CSite> currentSite = std::get< LINK_PTR>(link);
		double lat, lon, r;

		currentSite->getGeo().getGeographic(&lat, &lon, &r);

		siteString += getID() + "," + currentSite->getSCNL() + ","
				+ std::to_string(lat) + ";" + std::to_string(lon) + ";"
				+ std::to_string(r) + "\n";
	}

	return (siteString);
}

// ---------------------------------------------------------getSiteLinksCount
int CNode::getSiteLinksCount() const {
	// lock mutex for this scope
	std::lock_guard<std::mutex> guard(m_SiteLinkListMutex);
	return (m_vSiteLinkList.size());
}

// ---------------------------------------------------------getEnabled
bool CNode::getEnabled() const {
	return (m_bEnabled);
}

// ---------------------------------------------------------setEnabled
void CNode::setEnabled(bool enabled) {
	m_bEnabled = enabled;
}

// ---------------------------------------------------------getLatitude
double CNode::getLatitude() const {
	return (m_dLatitude);
}

// ---------------------------------------------------------getLongitude
double CNode::getLongitude() const {
	return (m_dLongitude);
}

// ---------------------------------------------------------getResolution
double CNode::getResolution() const {
	return (m_dResolution);
}

// ---------------------------------------------------------getDepth
double CNode::getDepth() const {
	return (m_dDepth);
}

// ---------------------------------------------------------getGeo
glassutil::CGeo CNode::getGeo() const {
	glassutil::CGeo geoNode;
	geoNode.setGeographic(m_dLatitude, m_dLongitude, EARTHRADIUSKM - m_dDepth);
	return (geoNode);
}

// ---------------------------------------------------------getWeb
CWeb* CNode::getWeb() const {
	std::lock_guard<std::recursive_mutex> nodeGuard(m_NodeMutex);
	return (m_pWeb);
}

// ---------------------------------------------------------setWeb
void CNode::setWeb(CWeb* web) {
	std::lock_guard<std::recursive_mutex> nodeGuard(m_NodeMutex);
	m_pWeb = web;
}

// ---------------------------------------------------------getName
const std::string& CNode::getName() const {
	return (m_sName);
}

// ---------------------------------------------------------getID
std::string CNode::getID() const {
	return (std::string(
			m_sName + "." + std::to_string(getLatitude()) + "."
					+ std::to_string(getLongitude()) + "."
					+ std::to_string(getDepth())));
}
}  // namespace glasscore
