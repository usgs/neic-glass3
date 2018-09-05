#include <json.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <tuple>
#include <memory>
#include <utility>
#include <vector>
#include <fstream>
#include <thread>
#include <mutex>
#include <ctime>
#include <limits>
#include <map>
#include <sstream>
#include "Web.h"
#include "IGlassSend.h"
#include "Glass.h"
#include "Terra.h"
#include "Date.h"
#include "Pick.h"
#include "Node.h"
#include "SiteList.h"
#include "Site.h"
#include "Logit.h"
#include "Pid.h"

#define _USE_MATH_DEFINES

namespace glasscore {

// site sorting function
// Compares nodal distance for nearest site assignment
bool sortSite(const std::pair<double, std::shared_ptr<CSite>> &lhs,
				const std::pair<double, std::shared_ptr<CSite>> &rhs) {
	// compare
	if (lhs.first < rhs.first) {
		return (true);
	}

	// lhs > rhs
	return (false);
}

// ---------------------------------------------------------CWeb
CWeb::CWeb(int numThreads, int sleepTime, int checkInterval)
		: glass3::util::ThreadBaseClass("Web", sleepTime, numThreads,
										checkInterval) {
	clear();

	// start up the threads
	start();
}

// ---------------------------------------------------------CWeb
CWeb::CWeb(std::string name, double thresh, int numDetect, int numNucleate,
			int resolution, int numRows, int numCols, int numZ, bool update,
			std::shared_ptr<traveltime::CTravelTime> firstTrav,
			std::shared_ptr<traveltime::CTravelTime> secondTrav, int numThreads,
			int sleepTime, int checkInterval, double aziTap, double maxDep)
		: glass3::util::ThreadBaseClass("Web", sleepTime, numThreads,
										checkInterval) {
	clear();

	initialize(name, thresh, numDetect, numNucleate, resolution, numRows,
				numCols, numZ, update, firstTrav, secondTrav, aziTap, maxDep);

	// start up the threads
	start();
}

// ---------------------------------------------------------~CWeb
CWeb::~CWeb() {
	clear();
}

// ---------------------------------------------------------clear
void CWeb::clear() {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
	m_iNumRows = 0;
	m_iNumColumns = 0;
	m_iNumDepths = 0;
	m_iNumStationsPerNode = 10;
	m_iNucleationDataThreshold = 5;
	m_dNucleationStackThreshold = 2.5;
	m_dResolution = 100;
	m_sName = "Nemo";
	m_pGlass = NULL;
	m_pSiteList = NULL;
	m_bUpdate = false;
	m_dAzimuthTaper = 360.0;
	m_dMaxDepth = 800.0;

	// clear out all the nodes in the web
	try {
		m_vNodeMutex.lock();
		for (auto &node : m_vNode) {
			node->clear();
		}
		m_vNode.clear();
	} catch (...) {
		// ensure the vNode mutex is unlocked
		m_vNodeMutex.unlock();

		throw;
	}
	m_vNodeMutex.unlock();

	// clear the network filter
	m_vNetworksFilter.clear();
	m_vSitesFilter.clear();
	m_bUseOnlyTeleseismicStations = false;

	// clear sites
	try {
		m_vSiteMutex.lock();
		m_vNodeSites.clear();
	} catch (...) {
		// ensure the vSite mutex is unlocked
		m_vSiteMutex.unlock();

		throw;
	}
	m_vSiteMutex.unlock();

	m_pNucleationTravelTime1 = NULL;
	m_pNucleationTravelTime2 = NULL;
}

// ---------------------------------------------------------initialize
bool CWeb::initialize(std::string name, double thresh, int numDetect,
						int numNucleate, int resolution, int numRows,
						int numCols, int numZ, bool update,
						std::shared_ptr<traveltime::CTravelTime> firstTrav,
						std::shared_ptr<traveltime::CTravelTime> secondTrav,
						double aziTap, double maxDep) {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);

	m_sName = name;
	m_dNucleationStackThreshold = thresh;
	m_iNumStationsPerNode = numDetect;
	m_iNucleationDataThreshold = numNucleate;
	m_dResolution = resolution;
	m_iNumRows = numRows;
	m_iNumColumns = numCols;
	m_iNumDepths = numZ;
	m_bUpdate = update;
	m_pNucleationTravelTime1 = firstTrav;
	m_pNucleationTravelTime2 = secondTrav;
	m_dAzimuthTaper = aziTap;
	m_dMaxDepth = maxDep;
	// done
	return (true);
}

// ---------------------------------------------------------dispatch
bool CWeb::dispatch(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::dispatch: NULL json communication.");
		return (false);
	}

	// we only care about messages with a string Cmd key.
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// generate a global grid
		if (v == "Global") {
			return (global(com));
		}

		// generate a regional or local grid
		if (v == "Grid") {
			return (grid(com));
		}

		// generate an explicitly defined grid
		if (v == "Grid_Explicit") {
			return (gridExplicit(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------global
bool CWeb::global(std::shared_ptr<json::Object> com) {
	glassutil::CLogit::log(glassutil::log_level::debug, "CWeb::global");

	// nullchecks
	// check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::global: NULL json configuration.");
		return (false);
	}

	char sLog[1024];

	// global grid definition variables and defaults
	std::string name = "Nemo";
	int detect = 20;
	int nucleate = 7;
	double thresh = 2.0;

	// check pGlass
	if (m_pGlass != NULL) {
		detect = m_pGlass->getNumStationsPerNode();
		nucleate = m_pGlass->getNucleationDataThreshold();
		thresh = m_pGlass->getNucleationStackThreshold();
	}

	double resol = 100;
	std::vector<double> zzz;
	int zs = 0;
	bool saveGrid = false;
	bool update = false;
	double aziTaper = 360.;
	double maxDepth = 800.;
	// get grid configuration from json
	// name
	if (((*com).HasKey("Name"))
			&& ((*com)["Name"].GetType() == json::ValueType::StringVal)) {
		name = (*com)["Name"].ToString();
	}

	// Nucleation Phases to be used for this Global grid
	if (((*com).HasKey("NucleationPhases"))
			&& ((*com)["NucleationPhases"].GetType()
					== json::ValueType::ObjectVal)) {
		json::Object nucleationPhases = (*com)["NucleationPhases"].ToObject();

		if (loadTravelTimes(&nucleationPhases) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::global: Error Loading NucleationPhases");
			return (false);
		}
	} else {
		if (loadTravelTimes((json::Object *) NULL) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::global: Error Loading default NucleationPhases");
			return (false);
		}
	}

	// the number of stations that are assigned to each node
	if (((*com).HasKey("Detect"))
			&& ((*com)["Detect"].GetType() == json::ValueType::IntVal)) {
		detect = (*com)["Detect"].ToInt();
	}

	// number of picks that need to associate to start an event
	if (((*com).HasKey("Nucleate"))
			&& ((*com)["Nucleate"].GetType() == json::ValueType::IntVal)) {
		nucleate = (*com)["Nucleate"].ToInt();
	}

	// viability threshold needed to exceed for a nucleation to be successful.
	if (((*com).HasKey("Thresh"))
			&& ((*com)["Thresh"].GetType() == json::ValueType::DoubleVal)) {
		thresh = (*com)["Thresh"].ToDouble();
	}

	// sets the aziTaper value
	if ((*com).HasKey("AzimuthGapTaper")
			&& ((*com)["AzimuthGapTaper"].GetType()
					== json::ValueType::DoubleVal)) {
		aziTaper = (*com)["AzimuthGapTaper"].ToDouble();
	}

	// sets the maxDepth value
	if ((*com).HasKey("MaximumDepth")
			&& ((*com)["MaximumDepth"].GetType() == json::ValueType::DoubleVal)) {
		maxDepth = (*com)["MaximumDepth"].ToDouble();
	}

	// Node resolution for this Global grid
	if (((*com).HasKey("Resolution"))
			&& ((*com)["Resolution"].GetType() == json::ValueType::DoubleVal)) {
		resol = (*com)["Resolution"].ToDouble();
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CWeb::global: Missing required Resolution Key.");
		return (false);
	}

	// list of depth layers to generate for this Global grid
	if (((*com).HasKey("Z"))
			&& ((*com)["Z"].GetType() == json::ValueType::ArrayVal)) {
		json::Array zarray = (*com)["Z"].ToArray();
		for (auto v : zarray) {
			if (v.GetType() == json::ValueType::DoubleVal) {
				zzz.push_back(v.ToDouble());
			}
		}
		zs = static_cast<int>(zzz.size());
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::global: Missing required Z Array.");
		return (false);
	}

	// whether to create a file detailing the node configuration for
	// this Global grid
	if ((*com).HasKey("SaveGrid")) {
		saveGrid = (*com)["SaveGrid"].ToBool();
	}

	// set whether to update weblists
	if ((com->HasKey("Update"))
			&& ((*com)["Update"].GetType() == json::ValueType::BoolVal)) {
		update = (*com)["Update"].ToBool();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::global: Using Update: " + std::to_string(update));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::global: Using default Update: "
						+ std::to_string(update));
	}

	// init, note global doesn't use nRow or nCol
	initialize(name, thresh, detect, nucleate, resol, 0, 0, zs, update,
				m_pNucleationTravelTime1, m_pNucleationTravelTime2, aziTaper,
				maxDepth);

	// generate site and network filter lists
	generateSiteFilters(com);

	// Generate eligible station list
	generateNodeSiteList();

	// calculate the number of nodes from the desired resolution
	// This function was empirically determined via using different
	// numNode values and computing the average resolution from a node to
	// the nearest other 6 nodes
	int numNodes = 5.0E8 * std::pow(m_dResolution, -1.965);

	// should have an odd number of nodes (see paper named below)
	if ((numNodes % 2) == 0) {
		numNodes += 1;
	}

	snprintf(sLog, sizeof(sLog), "CWeb::global: Calculated numNodes:%d;",
				numNodes);
	glassutil::CLogit::log(sLog);

	// create / open gridfile for saving
	std::ofstream outfile;
	std::ofstream outstafile;
	if (saveGrid) {
		std::string filename = m_sName + "_gridfile.txt";
		outfile.open(filename, std::ios::out);
		outfile << "Grid,NodeID,NodeLat,NodeLon,NodeDepth" << "\n";

		filename = m_sName + "_gridstafile.txt";
		outstafile.open(filename, std::ios::out);
		outstafile << "NodeID,[StationSCNL;StationLat;StationLon;StationRad],"
					<< "\n";
	}

	// Generate equally spaced grid of nodes over the globe (more or less)
	// Follows Paper (Gonzolez, 2010) Measurement of Areas on a Sphere Using
	// Fibonacci and Latitude Longitude Lattices
	// std::vector<std::pair<double, double>> vVert;
	int iNodeCount = 0;
	int numSamples = (numNodes - 1) / 2;
	double fibRatio = (1 + std::sqrt(5.0)) / 2.0;  // AKA golden ratio

	for (int i = (-1 * numSamples); i <= numSamples; i++) {
		double aLat = std::asin((2 * i) / ((2.0 * numSamples) + 1))
				* (180.0 / PI);
		double aLon = fmod(i, fibRatio) * (360.0 / fibRatio);

		// longitude bounds check
		if (aLon < -180.0) {
			aLon += 360.0;
		}
		if (aLon > 180.0) {
			aLon -= 360.0;
		}

		// lock the site list while adding a node
		std::lock_guard<std::mutex> guard(m_vSiteMutex);

		// sort site list for this vertex
		sortNodeSiteList(aLat, aLon);

		// for each depth
		for (auto z : zzz) {
			// create node
			std::shared_ptr<CNode> node = generateNode(aLat, aLon, z,
														m_dResolution);

			// if we got a valid node, add it
			if (addNode(node) == true) {
				iNodeCount++;

				// write node to grid file
				if (saveGrid) {
					outfile << m_sName << "," << node->getID() << ","
							<< std::to_string(aLat) << ","
							<< std::to_string(aLon) << "," << std::to_string(z)
							<< "\n";

					// write to station file
					outstafile << node->getSitesString();
				}
			}
		}
	}

	// close grid file
	if (saveGrid) {
		outfile.close();
		outstafile.close();
	}

	std::string phases = "";
	if (m_pNucleationTravelTime1 != NULL) {
		phases += m_pNucleationTravelTime1->sPhase;
	}
	if (m_pNucleationTravelTime2 != NULL) {
		phases += ", " + m_pNucleationTravelTime2->sPhase;
	}

	// log global grid info
	snprintf(
			sLog, sizeof(sLog),
			"CWeb::global sName:%s Phase(s):%s; nZ:%d; resol:%.2f; nDetect:%d;"
			" nNucleate:%d; dThresh:%.2f; vNetFilter:%d;"
			" vSitesFilter:%d; bUseOnlyTeleseismicStations:%d; iNodeCount:%d;",
			m_sName.c_str(), phases.c_str(), getNumDepths(), getResolution(),
			getNumStationsPerNode(), getNucleationDataThreshold(),
			getNucleationStackThreshold(),
			static_cast<int>(m_vNetworksFilter.size()),
			static_cast<int>(m_vSitesFilter.size()),
			static_cast<int>(m_bUseOnlyTeleseismicStations), iNodeCount);
	glassutil::CLogit::log(glassutil::log_level::info, sLog);

	// success
	return (true);
}

// ---------------------------------------------------------grid
bool CWeb::grid(std::shared_ptr<json::Object> com) {
	glassutil::CLogit::log(glassutil::log_level::debug, "CWeb::grid");
	// nullchecks
	// check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::grid: NULL json configuration.");
		return (false);
	}

	char sLog[1024];

	// grid definition variables and defaults
	std::string name = "Nemo";
	int detect = 20;
	int nucleate = 7;
	double thresh = 2.0;

	// check pGlass
	if (m_pGlass != NULL) {
		detect = m_pGlass->getNumStationsPerNode();
		nucleate = m_pGlass->getNucleationDataThreshold();
		thresh = m_pGlass->getNucleationStackThreshold();
	}

	double resol = 0;
	double lat = 0;
	double lon = 0;
	int rows = 0;
	int cols = 0;
	double aziTaper = 360.;
	double maxDepth = 800.;
	std::vector<double> zzz;
	int zs = 0;
	bool saveGrid = false;
	bool update = false;

	// get grid configuration from json
	// name
	if (((*com).HasKey("Name"))
			&& ((*com)["Name"].GetType() == json::ValueType::StringVal)) {
		name = (*com)["Name"].ToString();
	}

	// Nucleation Phases to be used for this grid
	if (((*com).HasKey("NucleationPhases"))
			&& ((*com)["NucleationPhases"].GetType()
					== json::ValueType::ObjectVal)) {
		json::Object nucleationPhases = (*com)["NucleationPhases"].ToObject();

		if (loadTravelTimes(&nucleationPhases) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::grid: Error Loading NucleationPhases");
			return (false);
		}
	} else {
		if (loadTravelTimes((json::Object *) NULL) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::grid: Error Loading default NucleationPhases");
			return (false);
		}
	}

	// the number of stations that are assigned to each node
	if (((*com).HasKey("Detect"))
			&& ((*com)["Detect"].GetType() == json::ValueType::IntVal)) {
		detect = (*com)["Detect"].ToInt();
	}

	// number of picks that need to associate to start an event
	if (((*com).HasKey("Nucleate"))
			&& ((*com)["Nucleate"].GetType() == json::ValueType::IntVal)) {
		nucleate = (*com)["Nucleate"].ToInt();
	}

	// viability threshold needed to exceed for a nucleation to be successful.
	if (((*com).HasKey("Thresh"))
			&& ((*com)["Thresh"].GetType() == json::ValueType::DoubleVal)) {
		thresh = (*com)["Thresh"].ToDouble();
	}

	// Node resolution for this grid
	if (((*com).HasKey("Resolution"))
			&& ((*com)["Resolution"].GetType() == json::ValueType::DoubleVal)) {
		resol = (*com)["Resolution"].ToDouble();
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::grid: Missing required Resolution Key.");
		return (false);
	}

	// Latitude of the center point of this grid
	if (((*com).HasKey("Lat"))
			&& ((*com)["Lat"].GetType() == json::ValueType::DoubleVal)) {
		lat = (*com)["Lat"].ToDouble();
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::grid: Missing required Lat Key.");
		return (false);
	}

	// Longitude of the center point of this grid
	if (((*com).HasKey("Lon"))
			&& ((*com)["Lon"].GetType() == json::ValueType::DoubleVal)) {
		lon = (*com)["Lon"].ToDouble();
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::grid: Missing required Lon Key.");
		return (false);
	}

	// list of depth layers to generate for this grid
	if (((*com).HasKey("Z"))
			&& ((*com)["Z"].GetType() == json::ValueType::ArrayVal)) {
		json::Array zarray = (*com)["Z"].ToArray();
		for (auto v : zarray) {
			if (v.GetType() == json::ValueType::DoubleVal) {
				zzz.push_back(v.ToDouble());
			}
		}
		zs = static_cast<int>(zzz.size());
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::grid: Missing required Z Array.");
		return (false);
	}

	// the number of rows in this grid
	if (((*com).HasKey("Rows"))
			&& ((*com)["Rows"].GetType() == json::ValueType::IntVal)) {
		rows = (*com)["Rows"].ToInt();
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::grid: Missing required Rows Key.");
		return (false);
	}

	// the number of columns in this grid
	if (((*com).HasKey("Cols"))
			&& ((*com)["Cols"].GetType() == json::ValueType::IntVal)) {
		cols = (*com)["Cols"].ToInt();
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::grid: Missing required Cols Key.");
		return (false);
	}

	// sets the aziTaper value
	if ((*com).HasKey("AzimuthGapTaper")
			&& ((*com)["AzimuthGapTaper"].GetType()
					== json::ValueType::DoubleVal)) {
		aziTaper = (*com)["AzimuthGapTaper"].ToDouble();
	}

	// sets the maxDepth value
	if ((*com).HasKey("MaximumDepth")
			&& ((*com)["MaximumDepth"].GetType() == json::ValueType::DoubleVal)) {
		maxDepth = (*com)["MaximumDepth"].ToDouble();
	}

	// whether to create a file detailing the node configuration for
	// this grid
	if ((*com).HasKey("SaveGrid")) {
		saveGrid = (*com)["SaveGrid"].ToBool();
	}

	// set whether to update weblists
	if ((com->HasKey("Update"))
			&& ((*com)["Update"].GetType() == json::ValueType::BoolVal)) {
		update = (*com)["Update"].ToBool();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::grid: Using Update: " + std::to_string(update));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::grid: Using default Update: "
						+ std::to_string(update));
	}

	// initialize
	initialize(name, thresh, detect, nucleate, resol, rows, cols, zs, update,
				m_pNucleationTravelTime1, m_pNucleationTravelTime2, aziTaper,
				maxDepth);

	// generate site and network filter lists
	generateSiteFilters(com);

	// Generate eligible station list
	generateNodeSiteList();

	// Generate initial node values
	// compute latitude distance in geographic degrees by converting
	// the provided resolution in kilometers to degrees
	// NOTE: Hard coded conversion factor
	double latDistance = resol / DEG2KM;

	// compute the longitude distance in geographic degrees by
	// dividing the latitude distance by the cosine of the center latitude
	double lonDistance = latDistance / cos(DEG2RAD * lat);

	// compute the middle row index
	int irow0 = m_iNumRows / 2;

	// compute the middle column index
	int icol0 = m_iNumColumns / 2;

	// compute the maximum latitude using the provided center latitude,
	// middle row index, and the latitude distance
	double lat0 = lat + (irow0 * latDistance);

	// compute the minimum longitude using the provided center longitude,
	// middle column index, and the longitude distance
	double lon0 = lon - (icol0 * lonDistance);

	// create / open gridfile for saving
	std::ofstream outfile;
	std::ofstream outstafile;
	if (saveGrid) {
		std::string filename = m_sName + "_gridfile.txt";
		outfile.open(filename, std::ios::out);
		outfile << "Grid,NodeID,NodeLat,NodeLon,NodeDepth" << "\n";

		filename = m_sName + "_gridstafile.txt";
		outstafile.open(filename, std::ios::out);
		outstafile << "NodeID,[StationSCNL;StationLat;StationLon;StationRad],"
					<< "\n";
	}

	// init node count
	int iNodeCount = 0;

	// generate grid
	// for each row
	for (int irow = 0; irow < m_iNumRows; irow++) {
		// compute the current row latitude by subtracting
		// the row index multiplied by the latitude distance from the
		// maximum latitude
		double latrow = lat0 - (irow * latDistance);

		// log the latitude
		snprintf(sLog, sizeof(sLog), "LatRow:%.2f", latrow);
		glassutil::CLogit::log(glassutil::log_level::debug, sLog);
		// for each col
		for (int icol = 0; icol < m_iNumColumns; icol++) {
			// compute the current column longitude by adding the
			// column index multiplied by the longitude distance to the
			// minimum longitude
			double loncol = lon0 + (icol * lonDistance);

			std::lock_guard<std::mutex> guard(m_vSiteMutex);

			// sort site list for this grid point
			sortNodeSiteList(latrow, loncol);

			// for each depth at this grid point
			for (auto z : zzz) {
				// generate this node
				std::shared_ptr<CNode> node = generateNode(latrow, loncol, z,
															m_dResolution);

				// if we got a valid node, add it
				if (addNode(node) == true) {
					iNodeCount++;
				}

				// write node to grid file
				if (saveGrid) {
					outfile << m_sName << "," << node->getID() << ","
							<< std::to_string(latrow) << ","
							<< std::to_string(loncol) << ","
							<< std::to_string(z) << "\n";

					// write to station file
					outstafile << node->getSitesString();
				}
			}
		}
	}

	// close grid file
	if (saveGrid) {
		outfile.close();
		outstafile.close();
	}

	std::string phases = "";
	if (m_pNucleationTravelTime1 != NULL) {
		phases += m_pNucleationTravelTime1->sPhase;
	}
	if (m_pNucleationTravelTime2 != NULL) {
		phases += ", " + m_pNucleationTravelTime2->sPhase;
	}

	// log grid info
	snprintf(sLog, sizeof(sLog),
				"CWeb::grid sName:%s Phase(s):%s; Ranges:Lat(%.2f,%.2f),"
				"Lon:(%.2f,%.2f); nRow:%d; nCol:%d; nZ:%d; resol:%.2f;"
				" nDetect:%d; nNucleate:%d; dThresh:%.2f; vNetFilter:%d;"
				" vSitesFilter:%d;  bUseOnlyTeleseismicStations:%d;"
				" iNodeCount:%d;",
				m_sName.c_str(), phases.c_str(), lat0,
				lat0 - (m_iNumRows - 1) * latDistance, lon0,
				lon0 + (m_iNumColumns - 1) * lonDistance, getNumRows(),
				getNumColumns(), getNumDepths(), getResolution(),
				getNumStationsPerNode(), getNucleationDataThreshold(),
				getNucleationStackThreshold(),
				static_cast<int>(m_vNetworksFilter.size()),
				static_cast<int>(m_vSitesFilter.size()),
				static_cast<int>(m_bUseOnlyTeleseismicStations), iNodeCount);
	glassutil::CLogit::log(glassutil::log_level::info, sLog);

	// success
	return (true);
}

// ---------------------------------------------------------gridExplicit
bool CWeb::gridExplicit(std::shared_ptr<json::Object> com) {
	glassutil::CLogit::log(glassutil::log_level::debug, "CWeb::grid_explicit");

	// nullchecks
	// check json
	if (com == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CWeb::grid_explicit: NULL json configuration.");
		return (false);
	}

	char sLog[1024];

	// grid definition variables and defaults
	std::string name = "Nemo";
	int detect = 20;
	int nucleate = 7;
	double thresh = 2.0;

	// check pGlass
	if (m_pGlass != NULL) {
		detect = m_pGlass->getNumStationsPerNode();
		nucleate = m_pGlass->getNucleationDataThreshold();
		thresh = m_pGlass->getNucleationStackThreshold();
	}

	int nN = 0;
	bool saveGrid = false;
	bool update = false;
	double aziTaper = 360.;
	double maxDepth = 800.;
	std::vector<std::vector<double>> nodes;
	double resol = 0;

	// get grid configuration from json
	// name
	if (((*com).HasKey("Name"))
			&& ((*com)["Name"].GetType() == json::ValueType::StringVal)) {
		name = (*com)["Name"].ToString();
	}

	// Nucleation Phases to be used for this grid
	if (((*com).HasKey("NucleationPhases"))
			&& ((*com)["NucleationPhases"].GetType()
					== json::ValueType::ObjectVal)) {
		json::Object nucleationPhases = (*com)["NucleationPhases"].ToObject();

		if (loadTravelTimes(&nucleationPhases) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::grid_explicit: Error Loading NucleationPhases");
			return (false);
		}
	} else {
		if (loadTravelTimes((json::Object *) NULL) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::grid_explicit: Error Loading default NucleationPhases");
			return (false);
		}
	}

	// the number of stations that are assigned to each node
	if (((*com).HasKey("Detect"))
			&& ((*com)["Detect"].GetType() == json::ValueType::IntVal)) {
		detect = (*com)["Detect"].ToInt();
	}

	// number of picks that need to associate to start an event
	if (((*com).HasKey("Nucleate"))
			&& ((*com)["Nucleate"].GetType() == json::ValueType::IntVal)) {
		nucleate = (*com)["Nucleate"].ToInt();
	}

	// viability threshold needed to exceed for a nucleation to be successful.
	if (((*com).HasKey("Thresh"))
			&& ((*com)["Thresh"].GetType() == json::ValueType::DoubleVal)) {
		thresh = (*com)["Thresh"].ToDouble();
	}

	// sets the aziTaper value
	if ((*com).HasKey("AzimuthGapTaper")
			&& ((*com)["AzimuthGapTaper"].GetType()
					== json::ValueType::DoubleVal)) {
		aziTaper = (*com)["AzimuthGapTaper"].ToDouble();
	}

	// sets the maxDepth value
	if ((*com).HasKey("MaximumDepth")
			&& ((*com)["MaximumDepth"].GetType() == json::ValueType::DoubleVal)) {
		maxDepth = (*com)["MaximumDepth"].ToDouble();
	}

	// Node resolution for this grid
	if (((*com).HasKey("Resolution"))
			&& ((*com)["Resolution"].GetType() == json::ValueType::DoubleVal)) {
		resol = (*com)["Resolution"].ToDouble();
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CWeb::grid_explicit: Missing required Resolution Key.");
		return (false);
	}

	// the number of columns in this grid
	if ((com->HasKey("NodeList"))
			&& ((*com)["NodeList"].GetType() == json::ValueType::ArrayVal)) {
		json::Array nodeList = (*com)["NodeList"].ToArray();

		for (const auto &val : nodeList) {
			nN++;
		}
		nodes.resize(nN);

		int i = 0;
		for (const auto &val : nodeList) {
			if (val.GetType() != json::ValueType::ObjectVal) {
				glassutil::CLogit::log(
						glassutil::log_level::error,
						"CWeb::grid_explicit: Bad Node Definition found in Node"
						" List.");
				continue;
			}
			json::Object obj = val.ToObject();
			if (obj["Latitude"].GetType() != json::ValueType::DoubleVal
					|| obj["Longitude"].GetType() != json::ValueType::DoubleVal
					|| obj["Depth"].GetType() != json::ValueType::DoubleVal) {
				glassutil::CLogit::log(
						glassutil::log_level::error,
						"CWeb::grid_explicit: Node Lat, Lon, or Depth not a "
						" double in param file.");
				return (false);
			}
			double lat = obj["Latitude"].ToDouble();
			double lon = obj["Longitude"].ToDouble();
			double depth = obj["Depth"].ToDouble();
			nodes[i].resize(3);
			nodes[i][0] = lat;
			nodes[i][1] = lon;
			nodes[i][2] = depth;
			i++;
		}
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CWeb::grid_explicit: Missing required NodeList Key.");
		return (false);
	}
	// whether to create a file detailing the node configuration for
	// this grid
	if ((*com).HasKey("SaveGrid")) {
		saveGrid = (*com)["SaveGrid"].ToBool();
	}
	// set whether to update weblists
	if ((com->HasKey("Update"))
			&& ((*com)["Update"].GetType() == json::ValueType::BoolVal)) {
		update = (*com)["Update"].ToBool();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::grid_explicit: Using Update: "
						+ std::to_string(update));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::grid_explicit: Using default Update: "
						+ std::to_string(update));
	}

	// initialize
	initialize(name, thresh, detect, nucleate, resol, 0., 0., 0., update,
				m_pNucleationTravelTime1, m_pNucleationTravelTime2, aziTaper,
				maxDepth);

	// generate site and network filter lists
	generateSiteFilters(com);

	// Generate eligible station list
	generateNodeSiteList();

	// create / open gridfile for saving
	std::ofstream outfile;
	std::ofstream outstafile;
	if (saveGrid) {
		std::string filename = m_sName + "_gridfile.txt";
		outfile.open(filename, std::ios::out);
		outfile << "Grid,NodeID,NodeLat,NodeLon,NodeDepth" << "\n";

		filename = m_sName + "_gridstafile.txt";
		outstafile.open(filename, std::ios::out);
		outstafile << "NodeID,StationSCNL,StationLat,StationLon,StationRad"
					<< "\n";
	}

	// init node count
	int iNodeCount = 0;

	// loop through node vector
	for (int i = 0; i < nN; i++) {
		// get lat,lon,depth
		double lat = nodes[i][0];
		double lon = nodes[i][1];
		double Z = nodes[i][2];

		std::lock_guard<std::mutex> guard(m_vSiteMutex);

		// sort site list
		sortNodeSiteList(lat, lon);

		// create node, note resolution set to 0
		std::shared_ptr<CNode> node = generateNode(lat, lon, Z, resol);
		if (addNode(node) == true) {
			iNodeCount++;
		}

		// write to station file
		outstafile << node->getSitesString();
	}

	// close grid file
	if (saveGrid) {
		outfile.close();
		outstafile.close();
	}

	std::string phases = "";
	if (m_pNucleationTravelTime1 != NULL) {
		phases += m_pNucleationTravelTime1->sPhase;
	}
	if (m_pNucleationTravelTime2 != NULL) {
		phases += ", " + m_pNucleationTravelTime2->sPhase;
	}

	// log grid info
	snprintf(
			sLog,
			sizeof(sLog),
			"CWeb::grid_explicit sName:%s Phase(s):%s; nDetect:%d; nNucleate:%d;"
			" dThresh:%.2f; vNetFilter:%d; bUseOnlyTeleseismicStations:%d;"
			" vSitesFilter:%d; iNodeCount:%d;",
			m_sName.c_str(), phases.c_str(), getNumStationsPerNode(),
			getNucleationDataThreshold(), getNucleationStackThreshold(),
			static_cast<int>(m_vNetworksFilter.size()),
			static_cast<int>(m_vSitesFilter.size()),
			static_cast<int>(m_bUseOnlyTeleseismicStations), iNodeCount);
	glassutil::CLogit::log(glassutil::log_level::info, sLog);

	// success
	return (true);
}

// ---------------------------------------------------------loadTravelTimes
bool CWeb::loadTravelTimes(json::Object *com) {
	// check json
	if (com == NULL) {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CWeb::loadTravelTimes: NULL json configuration "
				"Using default first phase and no second phase");

		// if no json object, default to P
		// clean out old phase if any
		m_pNucleationTravelTime1.reset();

		// use overall glass default if available
		if ((m_pGlass != NULL)
				&& (m_pGlass->getDefaultNucleationTravelTime() != NULL)) {
			m_pNucleationTravelTime1 =
					m_pGlass->getDefaultNucleationTravelTime();
		} else {
			// create new traveltime
			m_pNucleationTravelTime1 =
					std::make_shared<traveltime::CTravelTime>();

			// set up the nucleation traveltime
			m_pNucleationTravelTime1->setup("P");
		}

		// no second phase
		// clean out old phase if any
		m_pNucleationTravelTime2.reset();

		return (true);
	}

	// load the first travel time
	if ((com->HasKey("Phase1"))
			&& ((*com)["Phase1"].GetType() == json::ValueType::ObjectVal)) {
		// get the phase object
		json::Object phsObj = (*com)["Phase1"].ToObject();

		// clean out old phase if any
		m_pNucleationTravelTime1.reset();

		// create new traveltime
		m_pNucleationTravelTime1 = std::make_shared<traveltime::CTravelTime>();

		// get the phase name
		// default to P
		std::string phs = "P";
		if (phsObj.HasKey("PhaseName")) {
			phs = phsObj["PhaseName"].ToString();
		}

		// get the file if present
		std::string file = "";
		if (phsObj.HasKey("TravFile")) {
			file = phsObj["TravFile"].ToString();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CWeb::loadTravelTimes: Using file location: " + file
							+ " for first phase: " + phs);
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CWeb::loadTravelTimes: Using default file location for "
							"first phase: " + phs);
		}

		// set up the first phase travel time
		if (m_pNucleationTravelTime1->setup(phs, file) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::loadTravelTimes: Failed to load file "
							"location " + file + " for first phase: " + phs);
			return (false);
		}

	} else {
		// if no first phase, use default from glass
		m_pNucleationTravelTime1.reset();

		// use overall glass default if available
		if (m_pGlass->getDefaultNucleationTravelTime() != NULL) {
			m_pNucleationTravelTime1 =
					m_pGlass->getDefaultNucleationTravelTime();
		} else {
			// create new traveltime
			m_pNucleationTravelTime1 =
					std::make_shared<traveltime::CTravelTime>();

			// set up the nucleation traveltime
			m_pNucleationTravelTime1->setup("P");
		}

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CWeb::loadTravelTimes: Using default first phase");
	}

	// load the second travel time
	if ((com->HasKey("Phase2"))
			&& ((*com)["Phase2"].GetType() == json::ValueType::ObjectVal)) {
		// get the phase object
		json::Object phsObj = (*com)["Phase2"].ToObject();

		// clean out old phase if any
		m_pNucleationTravelTime2.reset();

		// create new travel time
		m_pNucleationTravelTime2 = std::make_shared<traveltime::CTravelTime>();

		// get the phase name
		// default to S
		std::string phs = "S";
		if (phsObj.HasKey("PhaseName")) {
			phs = phsObj["PhaseName"].ToString();
		}

		// get the file if present
		std::string file = "";
		if (phsObj.HasKey("TravFile")) {
			file = phsObj["TravFile"].ToString();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CWeb::loadTravelTimes: Using file location: " + file
							+ " for second phase: " + phs);
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CWeb::loadTravelTimes: Using default file location for "
							"second phase: " + phs);
		}

		// set up the second phase travel time
		if (m_pNucleationTravelTime2->setup(phs, file) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::loadTravelTimes: Failed to load file "
							"location " + file + " for second phase: " + phs);
			return (false);
		}
	} else {
		// no second phase
		// clean out old phase if any
		m_pNucleationTravelTime2.reset();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CWeb::loadTravelTimes: Not using secondary nuclation phase");
	}

	return (true);
}

// ---------------------------------------------------------generateSiteFilters
bool CWeb::generateSiteFilters(std::shared_ptr<json::Object> com) {
	// nullchecks
	// check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"genSiteFilters: NULL json configuration.");
		return (false);
	}

	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);

	// Get the network names to be included in this web.
	if ((*com).HasKey("Nets")
			&& ((*com)["Nets"].GetType() == json::ValueType::ArrayVal)) {
		// clear any previous network filter list
		m_vNetworksFilter.clear();

		// get the network array
		json::Array arr = (*com)["Nets"].ToArray();

		// for each network in the array
		int netFilterCount = 0;
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].GetType() == json::ValueType::StringVal) {
				// get the network
				std::string snet = arr[i].ToString();

				// add to the network filter list
				m_vNetworksFilter.push_back(snet);
				netFilterCount++;
			}
		}

		if (netFilterCount > 0) {
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CWeb::genSiteFilters: " + std::to_string(netFilterCount)
							+ " network filters configured.");
		}
	}

	// get the SCNL names to be included in this web.
	if ((*com).HasKey("Sites")
			&& ((*com)["Sites"].GetType() == json::ValueType::ArrayVal)) {
		// clear any previous site filter list
		m_vSitesFilter.clear();

		// get the site array
		int staFilterCount = 0;
		json::Array arr = (*com)["Sites"].ToArray();

		// for each site in the array
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].GetType() == json::ValueType::StringVal) {
				// get the scnl
				std::string scnl = arr[i].ToString();

				// add to the site filter list
				m_vSitesFilter.push_back(scnl);
				staFilterCount++;
			}
		}

		if (staFilterCount > 0) {
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CWeb::genSiteFilters: " + std::to_string(staFilterCount)
							+ " SCNL filters configured.");
		}
	}

	// check to see if we're only to use teleseismic stations.
	if ((*com).HasKey("UseOnlyTeleseismicStations")
			&& ((*com)["UseOnlyTeleseismicStations"].GetType()
					== json::ValueType::BoolVal)) {
		m_bUseOnlyTeleseismicStations = (*com)["UseOnlyTeleseismicStations"]
				.ToBool();

		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::genSiteFilters: bUseOnlyTeleseismicStations is "
						+ std::to_string(m_bUseOnlyTeleseismicStations) + ".");
	}

	return (true);
}

// ---------------------------------------------------------isSiteAllowed
bool CWeb::isSiteAllowed(std::shared_ptr<CSite> site) {
	if (site == NULL) {
		return (false);
	}

	// If we have do not have a site and/or network filter, just add it
	if ((m_vNetworksFilter.size() == 0) && (m_vSitesFilter.size() == 0)
			&& (m_bUseOnlyTeleseismicStations == false)) {
		return (true);
	}

	// default to false
	bool returnFlag = false;

	// if we have a network filter, make sure network is allowed before adding
	if ((m_vNetworksFilter.size() > 0)
			&& (find(m_vNetworksFilter.begin(), m_vNetworksFilter.end(),
						site->getNetwork()) != m_vNetworksFilter.end())) {
		returnFlag = true;
	}

	// if we have a site filter, make sure site is allowed before adding
	if ((m_vSitesFilter.size() > 0)
			&& (find(m_vSitesFilter.begin(), m_vSitesFilter.end(),
						site->getSCNL()) != m_vSitesFilter.end())) {
		returnFlag = true;
	}

	// if we're only using teleseismic stations, make sure site is allowed
	// before adding
	if (m_bUseOnlyTeleseismicStations == true) {
		// is this site used for teleseismic
		if (site->getUseForTeleseismic() == true) {
			returnFlag = true;
		} else {
			returnFlag = false;
		}
	}

	return (returnFlag);
}

// ---------------------------------------------------------generateNodeSiteList
bool CWeb::generateNodeSiteList() {
	// nullchecks
	// check pSiteList
	if (m_pSiteList == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genSiteList: NULL pSiteList pointer.");
		return (false);
	}

	// get the total number sites in glass's site list
	int nsite = m_pSiteList->size();

	// don't bother continuing if we have no sites
	if (nsite <= 0) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CWeb::genSiteList: No sites in site list.");
		return (false);
	}

	// log
	char sLog[1024];
	snprintf(sLog, sizeof(sLog),
				"CWeb::genSiteList: %d sites available for web %s", nsite,
				m_sName.c_str());
	glassutil::CLogit::log(glassutil::log_level::debug, sLog);

	// clear web site list
	m_vNodeSites.clear();

	// for each site
	for (int isite = 0; isite < nsite; isite++) {
		// get site from the overall site list
		std::shared_ptr<CSite> site = m_pSiteList->getSite(isite);

		// Ignore if station out of service
		if (!site->getUse()) {
			continue;
		}
		if (!site->getEnable()) {
			continue;
		}

		if (isSiteAllowed(site)) {
			m_vNodeSites.push_back(
					std::pair<double, std::shared_ptr<CSite>>(0.0, site));
		}
	}

	// log
	snprintf(sLog, sizeof(sLog),
				"CWeb::genSiteList: %d sites selected for web %s",
				static_cast<int>(m_vNodeSites.size()), m_sName.c_str());
	glassutil::CLogit::log(glassutil::log_level::info, sLog);

	return (true);
}

// ---------------------------------------------------------sortNodeSiteList
void CWeb::sortNodeSiteList(double lat, double lon) {
	// set to provided geographic location
	glassutil::CGeo geo;

	// NOTE: node depth is ignored here
	geo.setGeographic(lat, lon, 6371.0);

	// set the distance to each site
	for (int i = 0; i < m_vNodeSites.size(); i++) {
		// get the site
		auto p = m_vNodeSites[i];
		std::shared_ptr<CSite> site = p.second;

		// compute the distance
		p.first = site->getDelta(&geo);

		// set the distance in the vector
		m_vNodeSites[i] = p;
	}

	// sort sites
	std::sort(m_vNodeSites.begin(), m_vNodeSites.end(), sortSite);
}

// ---------------------------------------------------------generateNode
std::shared_ptr<CNode> CWeb::generateNode(double lat, double lon, double z,
											double resol) {
	// nullcheck
	if ((m_pNucleationTravelTime1 == NULL)
			&& (m_pNucleationTravelTime2 == NULL)) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genNode: No valid trav pointers.");
		return (NULL);
	}

	// create node
	std::shared_ptr<CNode> node(
			new CNode(m_sName, lat, lon, z, resol));

	// set parent web
	node->setWeb(this);

	// return empty node if we don't
	// have any sites
	if (m_vNodeSites.size() == 0) {
		return (node);
	}

	// generate the sites for the node
	node = generateNodeSites(node);

	// return the populated node
	return (node);
}

// ---------------------------------------------------------addNode
bool CWeb::addNode(std::shared_ptr<CNode> node) {
	// nullcheck
	if (node == NULL) {
		return (false);
	}
	std::lock_guard<std::mutex> vNodeGuard(m_vNodeMutex);

	m_vNode.push_back(node);

	return (true);
}

// ---------------------------------------------------------generateNodeSites
std::shared_ptr<CNode> CWeb::generateNodeSites(std::shared_ptr<CNode> node) {
	// nullchecks
	// check node
	if (node == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genNodeSites: NULL node pointer.");
		return (NULL);
	}
	// check trav
	if ((m_pNucleationTravelTime1 == NULL)
			&& (m_pNucleationTravelTime2 == NULL)) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genNodeSites: No valid trav pointers.");
		return (NULL);
	}
	// check sites
	if (m_vNodeSites.size() == 0) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genNodeSites: No sites.");
		return (node);
	}
	// check nDetect
	if (m_iNumStationsPerNode == 0) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genNodeSites: nDetect is 0.");
		return (node);
	}

	int sitesAllowed = m_iNumStationsPerNode;
	if (m_vNodeSites.size() < m_iNumStationsPerNode) {
		glassutil::CLogit::log(glassutil::log_level::warn,
								"CWeb::genNodeSites: nDetect is greater "
								"than the number of sites.");
		sitesAllowed = m_vNodeSites.size();
	}

	// setup traveltimes for this node
	if (m_pNucleationTravelTime1 != NULL) {
		m_pNucleationTravelTime1->setOrigin(node->getLatitude(),
											node->getLongitude(),
											node->getDepth());
	}
	if (m_pNucleationTravelTime2 != NULL) {
		m_pNucleationTravelTime2->setOrigin(node->getLatitude(),
											node->getLongitude(),
											node->getDepth());
	}

	// clear node of any existing sites
	node->clearSiteLinks();

	// for the number of allowed sites per node
	for (int i = 0; i < sitesAllowed; i++) {
		// get each site
		auto aSite = m_vNodeSites[i];
		std::shared_ptr<CSite> site = aSite.second;

		// compute delta distance between site and node
		double delta = RAD2DEG * aSite.first;

		// compute traveltimes between site and node
		double travelTime1 = -1;
		if (m_pNucleationTravelTime1 != NULL) {
			travelTime1 = m_pNucleationTravelTime1->T(delta);
		}

		double travelTime2 = -1;
		if (m_pNucleationTravelTime2 != NULL) {
			travelTime2 = m_pNucleationTravelTime2->T(delta);
		}

		// skip site if there are no valid times
		if ((travelTime1 < 0) && (travelTime2 < 0)) {
			continue;
		}

		// Link node to site using traveltimes
		node->linkSite(site, node, travelTime1, travelTime2);
	}

	// sort the site links
	node->sortSiteLinks();

	// return updated node
	return (node);
}

// ---------------------------------------------------------addSite
void CWeb::addSite(std::shared_ptr<CSite> site) {
	//  nullcheck
	if (site == NULL) {
		return;
	}

	// don't bother if we're not allowed to update
	if (m_bUpdate == false) {
		return;
	}

	// if this is a remove, send to remSite
	if (site->getEnable() == false) {
		removeSite(site);
		return;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CWeb::addSite: New potential station " + site->getSCNL()
					+ " for web: " + m_sName + ".");

	// don't bother if this site isn't allowed
	if (isSiteAllowed(site) == false) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::addSite: Station " + site->getSCNL()
						+ " not allowed in web " + m_sName + ".");
		return;
	}

	int nodeModCount = 0;
	int nodeCount = 0;
	int totalNodes = m_vNode.size();

	// for each node in web
	for (auto &node : m_vNode) {
		nodeCount++;
		// update thread status
		setThreadHealth(true);

		node->setEnabled(false);

		if (nodeCount % 1000 == 0) {
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CWeb::addSite: Station " + site->getSCNL() + " processed "
							+ std::to_string(nodeCount) + " out of "
							+ std::to_string(totalNodes) + " nodes in web: "
							+ m_sName + ". Modified "
							+ std::to_string(nodeModCount) + " nodes.");
		}

		// check to see if we have this site
		std::shared_ptr<CSite> foundSite = node->getSite(site->getSCNL());

		// update?
		if (foundSite != NULL) {
			// NOTE: what to do here?! anything? only would
			// matter if the site location changed or (future) quality changed
			node->setEnabled(true);
			continue;
		}

		// set to node geographic location
		// NOTE: node depth is ignored here
		glassutil::CGeo geo;
		geo.setGeographic(node->getLatitude(), node->getLongitude(), 6371.0);

		// compute delta distance between site and node
		double newDistance = RAD2DEG * site->getGeo().delta(&geo);

		// get site in node list
		// NOTE: this assumes that the node site list is sorted
		// on distance
		std::shared_ptr<CSite> furthestSite = node->getLastSite();

		// compute distance to farthest site
		double maxDistance = RAD2DEG * geo.delta(&furthestSite->getGeo());

		// Ignore if new site is farther than last linked site
		if ((node->count() >= m_iNumStationsPerNode)
				&& (newDistance > maxDistance)) {
			node->setEnabled(true);
			continue;
		}

		// setup traveltimes for this node
		if (m_pNucleationTravelTime1 != NULL) {
			m_pNucleationTravelTime1->setOrigin(node->getLatitude(),
												node->getLongitude(),
												node->getDepth());
		}
		if (m_pNucleationTravelTime2 != NULL) {
			m_pNucleationTravelTime2->setOrigin(node->getLatitude(),
												node->getLongitude(),
												node->getDepth());
		}

		// compute traveltimes between site and node
		double travelTime1 = -1;
		if (m_pNucleationTravelTime1 != NULL) {
			travelTime1 = m_pNucleationTravelTime1->T(newDistance);
		}
		double travelTime2 = -1;
		if (m_pNucleationTravelTime2 != NULL) {
			travelTime2 = m_pNucleationTravelTime2->T(newDistance);
		}

		// check to see if we're at the limit
		if (node->count() < m_iNumStationsPerNode) {
			// Link node to site using traveltimes
			node->linkSite(site, node, travelTime1, travelTime2);

		} else {
			// remove last site
			// This assumes that the node site list is sorted
			// on distance/traveltime
			node->unlinkLastSite();

			// Link node to site using traveltimes
			node->linkSite(site, node, travelTime1, travelTime2);
		}

		// resort site links
		node->sortSiteLinks();

		// we've added a site
		nodeModCount++;

		node->setEnabled(true);
	}

	// log info if we've added a site
	if (nodeModCount > 0) {
		char sLog[1024];
		snprintf(sLog, sizeof(sLog), "CWeb::addSite: Added site: %s to %d "
					"node(s) in web: %s",
					site->getSCNL().c_str(), nodeModCount, m_sName.c_str());
		glassutil::CLogit::log(glassutil::log_level::info, sLog);
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::addSite: Station " + site->getSCNL()
						+ " not added to any "
								"nodes in web: " + m_sName + ".");
	}
}

// ---------------------------------------------------------removeSite
void CWeb::removeSite(std::shared_ptr<CSite> site) {
	//  nullcheck
	if (site == NULL) {
		return;
	}

	// don't bother if we're not allowed to update
	if (m_bUpdate == false) {
		return;
	}

	// don't bother if this site isn't allowed
	if (isSiteAllowed(site) == false) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::remSite: Station " + site->getSCNL()
						+ " not allowed in web " + m_sName + ".");
		return;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CWeb::remSite: Trying to remove station " + site->getSCNL()
					+ " from web " + m_sName + ".");

	// init flag to check to see if we've generated a site list for this web
	// yet
	bool bSiteList = false;
	int nodeModCount = 0;
	int nodeCount = 0;
	int totalNodes = m_vNode.size();

	// for each node in web
	for (auto &node : m_vNode) {
		nodeCount++;
		// update thread status
		setThreadHealth(true);

		// search through each site linked to this node, see if we have it
		std::shared_ptr<CSite> foundSite = node->getSite(site->getSCNL());

		// don't bother if this node doesn't have this site
		if (foundSite == NULL) {
			continue;
		}

		node->setEnabled(false);

		if (nodeCount % 1000 == 0) {
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CWeb::remSite: Station " + site->getSCNL() + " processed "
							+ std::to_string(nodeCount) + " out of "
							+ std::to_string(totalNodes) + " nodes in web: "
							+ m_sName + ". Modified "
							+ std::to_string(nodeModCount) + " nodes.");
		}

		// lock the site list while we're using it
		std::lock_guard<std::mutex> guard(m_vSiteMutex);

		// generate the site list for this web if this is the first
		// iteration where a node is modified
		if (!bSiteList) {
			// generate the site list
			generateNodeSiteList();
			bSiteList = true;

			// make sure we've got enough sites for a node
			if (m_vNodeSites.size() < m_iNumStationsPerNode) {
				node->setEnabled(true);
				return;
			}
		}

		// sort overall list of sites for this node
		sortNodeSiteList(node->getLatitude(), node->getLongitude());

		// remove site link
		if (node->unlinkSite(foundSite) == true) {
			// get new site
			auto nextSite = m_vNodeSites[m_iNumStationsPerNode];
			std::shared_ptr<CSite> newSite = nextSite.second;

			// compute delta distance between site and node
			double newDistance = RAD2DEG * nextSite.first;

			// compute traveltimes between site and node
			double travelTime1 = -1;
			if (m_pNucleationTravelTime1 != NULL) {
				travelTime1 = m_pNucleationTravelTime1->T(newDistance);
			}
			double travelTime2 = -1;
			if (m_pNucleationTravelTime2 != NULL) {
				travelTime2 = m_pNucleationTravelTime2->T(newDistance);
			}

			// Link node to new site using traveltimes
			if (node->linkSite(newSite, node, travelTime1, travelTime2)
					== false) {
				glassutil::CLogit::log(
						glassutil::log_level::error,
						"CWeb::remSite: Failed to add station "
								+ newSite->getSCNL() + " to web " + m_sName
								+ ".");
			}

			// resort site links
			node->sortSiteLinks();

			// we've removed a site
			nodeModCount++;
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::remSite: Failed to remove station " + site->getSCNL()
							+ " from web " + m_sName + ".");
		}

		node->setEnabled(true);
	}

	// log info if we've removed a site
	char sLog[1024];
	if (nodeModCount > 0) {
		snprintf(
				sLog, sizeof(sLog),
				"CWeb::remSite: Removed site: %s from %d node(s) in web: %s",
				site->getSCNL().c_str(), nodeModCount, m_sName.c_str());
		glassutil::CLogit::log(glassutil::log_level::info, sLog);
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::remSite: Station " + site->getSCNL()
						+ " not removed from any nodes in web: " + m_sName
						+ ".");
	}
}

// ---------------------------------------------------------addJob
void CWeb::addJob(std::function<void()> newjob) {
	if (getNumThreads() == 0) {
		// no background thread, just run the job
		try {
			newjob();
		} catch (const std::exception &e) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::jobLoop: Exception during job(): "
							+ std::string(e.what()));
		}

		return;
	}

	// add the job to the queue
	std::lock_guard<std::mutex> guard(m_QueueMutex);
	m_JobQueue.push(newjob);
}

// ---------------------------------------------------------work
glass3::util::WorkState CWeb::work() {
	glassutil::CLogit::log(glassutil::log_level::debug,
							"CWeb::jobLoop: startup");
	// lock for queue access
	m_QueueMutex.lock();

	// are there any jobs
	if (m_JobQueue.empty() == true) {
		// unlock and skip until next time
		m_QueueMutex.unlock();

		return (glass3::util::WorkState::Idle);
	}

	// get the next job
	std::function<void()> newjob = m_JobQueue.front();
	m_JobQueue.pop();

	// done with queue
	m_QueueMutex.unlock();

	// run the job
	try {
		newjob();
	} catch (const std::exception &e) {
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CWeb::workLoop: Exception during job(): "
						+ std::string(e.what()));
		return (glass3::util::WorkState::Error);
	}

	// give up some time at the end of the loop
	return (glass3::util::WorkState::OK);
}

// ---------------------------------------------------------hasSite
bool CWeb::hasSite(std::shared_ptr<CSite> site) {
	//  nullcheck
	if (site == NULL) {
		return (false);
	}

	std::lock_guard<std::mutex> vNodeGuard(m_vNodeMutex);

	// for each node in web
	for (auto &node : m_vNode) {
		// check to see if we have this site
		if (node->getSite(site->getSCNL()) != NULL) {
			return (true);
		}
	}

	// site not found
	return (false);
}

// ---------------------------------------------------------getAzimuthTaper
double CWeb::getAzimuthTaper() const {
	return (m_dAzimuthTaper);
}

// ---------------------------------------------------------getMaxDepth
double CWeb::getMaxDepth() const {
	return (m_dMaxDepth);
}

// ---------------------------------------------------------getSiteList
const CSiteList* CWeb::getSiteList() const {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
	return (m_pSiteList);
}

// ---------------------------------------------------------setSiteList
void CWeb::setSiteList(CSiteList* siteList) {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
	m_pSiteList = siteList;
}

// ---------------------------------------------------------getGlass
CGlass* CWeb::getGlass() const {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
	return (m_pGlass);
}

// ---------------------------------------------------------setGlass
void CWeb::setGlass(CGlass* glass) {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
	m_pGlass = glass;
}

// ---------------------------------------------------------getUpdate
bool CWeb::getUpdate() const {
	return (m_bUpdate);
}

// ---------------------------------------------------------getResolution
double CWeb::getResolution() const {
	return (m_dResolution);
}

// --------------------------------------------------getNucleationStackThreshold
double CWeb::getNucleationStackThreshold() const {
	return (m_dNucleationStackThreshold);
}

// ---------------------------------------------------------getNumColumns
int CWeb::getNumColumns() const {
	return (m_iNumColumns);
}

// -------------------------------------------------------getNumStationsPerNode
int CWeb::getNumStationsPerNode() const {
	return (m_iNumStationsPerNode);
}

// ---------------------------------------------------getNucleationDataThreshold
int CWeb::getNucleationDataThreshold() const {
	return (m_iNucleationDataThreshold);
}

// ---------------------------------------------------------getNumRows
int CWeb::getNumRows() const {
	return (m_iNumRows);
}

// ---------------------------------------------------------getNumDepths
int CWeb::getNumDepths() const {
	return (m_iNumDepths);
}

// ---------------------------------------------------------getName
const std::string& CWeb::getName() const {
	return (m_sName);
}

// -----------------------------------------------------getNucleationTravelTime1
const std::shared_ptr<traveltime::CTravelTime>& CWeb::getNucleationTravelTime1() const { // NOLINT
	std::lock_guard<std::mutex> webGuard(m_TravelTimeMutex);
	return (m_pNucleationTravelTime1);
}

// -----------------------------------------------------getNucleationTravelTime2
const std::shared_ptr<traveltime::CTravelTime>& CWeb::getNucleationTravelTime2() const { // NOLINT
	std::lock_guard<std::mutex> webGuard(m_TravelTimeMutex);
	return (m_pNucleationTravelTime2);
}

// -----------------------------------------------------getNetworksFilterSize
int CWeb::getNetworksFilterSize() const {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
	return (m_vNetworksFilter.size());
}

// ------------------------------------------------getUseOnlyTeleseismicStations
bool CWeb::getUseOnlyTeleseismicStations() const {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
	return (m_bUseOnlyTeleseismicStations);
}

// -----------------------------------------------------getSitesFilterSize
int CWeb::getSitesFilterSize() const {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
	return (m_vSitesFilter.size());
}

// -----------------------------------------------------size
int CWeb::size() const {
	std::lock_guard<std::mutex> vNodeGuard(m_vNodeMutex);
	return (m_vNode.size());
}
}  // namespace glasscore

