#include "Web.h"
#include <json.h>
#include <logger.h>
#include <geo.h>
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
#include "Glass.h"
#include "Terra.h"
#include "Pick.h"
#include "Node.h"
#include "SiteList.h"
#include "Site.h"

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
			int resolution, bool update, bool save,
			std::shared_ptr<traveltime::CTravelTime> firstTrav,
			std::shared_ptr<traveltime::CTravelTime> secondTrav, int numThreads,
			int sleepTime, int checkInterval, double aziTap, double maxDep)
		: glass3::util::ThreadBaseClass("Web", sleepTime, numThreads,
										checkInterval) {
	clear();

	initialize(name, thresh, numDetect, numNucleate, resolution, update, save,
				firstTrav, secondTrav, aziTap, maxDep);

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
	m_iNumStationsPerNode = 10;
	m_iNucleationDataThreshold = 5;
	m_dNucleationStackThreshold = 2.5;
	m_dNodeResolution = 100;
	m_sName = "Nemo";
	m_pSiteList = NULL;
	m_bUpdate = false;
	m_bSaveGrid = false;
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
		m_vSitesSortedForCurrentNode.clear();
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
						int numNucleate, int resolution, bool update, bool save,
						std::shared_ptr<traveltime::CTravelTime> firstTrav,
						std::shared_ptr<traveltime::CTravelTime> secondTrav,
						double aziTap, double maxDep) {
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);

	m_sName = name;
	m_dNucleationStackThreshold = thresh;
	m_iNumStationsPerNode = numDetect;
	m_iNucleationDataThreshold = numNucleate;
	m_dNodeResolution = resolution;
	m_bUpdate = update;
	m_bSaveGrid = save;
	m_pNucleationTravelTime1 = firstTrav;
	m_pNucleationTravelTime2 = secondTrav;
	m_dAzimuthTaper = aziTap;
	m_dMaxDepth = maxDep;
	// done
	return (true);
}

// -------------------------------------------------------receiveExternalMessage
bool CWeb::receiveExternalMessage(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glass3::util::Logger::log(
				"error",
				"CWeb::receiveExternalMessage: NULL json communication.");
		return (false);
	}

	// we only care about messages with a string Cmd key.
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// generate a Global grid
		if (v == "Global") {
			return (generateGlobalGrid(com));
		}

		// generate a regional or local grid
		if (v == "Grid") {
			return (generateLocalGrid(com));
		}

		// generate an explicitly defined grid
		if (v == "GridExplicit") {
			return (generateExplicitGrid(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------generateGlobalGrid
bool CWeb::generateGlobalGrid(std::shared_ptr<json::Object> gridConfiguration) {
	glass3::util::Logger::log("debug",
							"CWeb::generateGlobalGrid");

	// nullchecks
	// check json
	if (gridConfiguration == NULL) {
		glass3::util::Logger::log(
				"error",
				"CWeb::generateGlobalGrid: NULL json configuration.");
		return (false);
	}

	char sLog[1024];
	std::vector<double> depthLayerArray;
	int numDepthLayers = 0;

	// load basic (common) grid configuration
	if (loadGridConfiguration(gridConfiguration) == false) {
		return (false);
	}

	// list of depth layers to generate for this grid
	if (((*gridConfiguration).HasKey("DepthLayers"))
			&& ((*gridConfiguration)["DepthLayers"].GetType()
					== json::ValueType::ArrayVal)) {
		json::Array zarray = (*gridConfiguration)["DepthLayers"].ToArray();
		for (auto v : zarray) {
			if (v.GetType() == json::ValueType::DoubleVal) {
				depthLayerArray.push_back(v.ToDouble());
			}
		}
		numDepthLayers = static_cast<int>(depthLayerArray.size());
	} else {
		glass3::util::Logger::log(
				"error",
				"CWeb::generateGlobalGrid: Missing required DepthLayers Array.");
		return (false);
	}

	// calculate the number of nodes from the desired resolution
	// using a function that was empirically determined via using different
	// numNode values and computing the average resolution from a node to
	// the nearest other 6 nodes. The spreadsheet used to calculate this function
	// is located in NodesToResoultionCalculations.xlsx.
	// The intention is to calculate the number of nodes to ensure the desired
	// node resolution when the grid is generated below
	int numNodes = 5.0E8 * std::pow(getNodeResolution(), -1.965);

	// should have an odd number of nodes (see paper named below)
	if ((numNodes % 2) == 0) {
		numNodes += 1;
	}

	snprintf(sLog, sizeof(sLog),
				"CWeb::generateGlobalGrid: Calculated numNodes:%d;", numNodes);
	glass3::util::Logger::log(sLog);

	// create / open gridfile for saving
	std::ofstream outfile;
	std::ofstream outstafile;
	if (getSaveGrid()) {
		std::string filename = m_sName + "_gridfile.txt";
		outfile.open(filename, std::ios::out);
		outfile << "generateLocalGrid,NodeID,NodeLat,NodeLon,NodeDepth" << "\n";

		filename = m_sName + "_gridstafile.txt";
		outstafile.open(filename, std::ios::out);
		outstafile << "NodeID,[StationSCNL;StationLat;StationLon;StationRad],"
					<< "\n";
	}

	// Generate equally spaced grid of nodes over the globe (more or less)
	// Follows Paper (Gonzalez, 2010) Measurement of Areas on a Sphere Using
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
		sortSiteListForNode(aLat, aLon);

		// for each depth
		for (auto z : depthLayerArray) {
			// create node
			std::shared_ptr<CNode> node = generateNode(aLat, aLon, z,
														getNodeResolution());

			// if we got a valid node, add it
			if (addNode(node) == true) {
				iNodeCount++;

				// write node to generateLocalGrid file
				if (getSaveGrid()) {
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

	// close generateLocalGrid file
	if (getSaveGrid()) {
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
			sLog, sizeof(sLog),
			"CWeb::generateGlobalGrid sName:%s Phase(s):%s; nZ:%d; resol:%.2f;"
			" nDetect:%d; nNucleate:%d; dThresh:%.2f; vNetFilter:%d;"
			" vSitesFilter:%d; bUseOnlyTeleseismicStations:%d; iNodeCount:%d;",
			m_sName.c_str(), phases.c_str(), numDepthLayers,
			getNodeResolution(), getNumStationsPerNode(),
			getNucleationDataThreshold(), getNucleationStackThreshold(),
			static_cast<int>(m_vNetworksFilter.size()),
			static_cast<int>(m_vSitesFilter.size()),
			static_cast<int>(m_bUseOnlyTeleseismicStations), iNodeCount);
	glass3::util::Logger::log("info", sLog);

	// success
	return (true);
}

// ---------------------------------------------------------generateLocalGrid
bool CWeb::generateLocalGrid(std::shared_ptr<json::Object> gridConfiguration) {
	glass3::util::Logger::log("debug",
							"CWeb::generateLocalGrid");

	// nullchecks
	// check json
	if (gridConfiguration == NULL) {
		glass3::util::Logger::log(
				"error",
				"CWeb::generateLocalGrid: NULL json configuration.");
		return (false);
	}

	char sLog[1024];
	double lat = 0;
	double lon = 0;
	int rows = 0;
	int cols = 0;
	std::vector<double> depthLayerArray;
	int numDepthLayers = 0;

	// load basic (common) grid configuration
	if (loadGridConfiguration(gridConfiguration) == false) {
		return (false);
	}

	// Latitude of the center point of this generateLocalGrid
	if (((*gridConfiguration).HasKey("CenterLatitude"))
			&& ((*gridConfiguration)["CenterLatitude"].GetType()
					== json::ValueType::DoubleVal)) {
		lat = (*gridConfiguration)["CenterLatitude"].ToDouble();
	} else {
		glass3::util::Logger::log(
				"error",
				"CWeb::generateLocalGrid: Missing required CenterLatitude Key.");
		return (false);
	}

	// Longitude of the center point of this generateLocalGrid
	if (((*gridConfiguration).HasKey("CenterLongitude"))
			&& ((*gridConfiguration)["CenterLongitude"].GetType()
					== json::ValueType::DoubleVal)) {
		lon = (*gridConfiguration)["CenterLongitude"].ToDouble();
	} else {
		glass3::util::Logger::log(
				"error",
				"CWeb::generateLocalGrid: Missing required Lon Key.");
		return (false);
	}

	// list of depth layers to generate for this generateLocalGrid
	if (((*gridConfiguration).HasKey("DepthLayers"))
			&& ((*gridConfiguration)["DepthLayers"].GetType()
					== json::ValueType::ArrayVal)) {
		json::Array jsonLayers = (*gridConfiguration)["DepthLayers"].ToArray();
		for (auto aLayer : jsonLayers) {
			if (aLayer.GetType() == json::ValueType::DoubleVal) {
				depthLayerArray.push_back(aLayer.ToDouble());
			}
		}
		numDepthLayers = static_cast<int>(depthLayerArray.size());
	} else {
		glass3::util::Logger::log(
				"error",
				"CWeb::generateLocalGrid: Missing required DepthLayers Array.");
		return (false);
	}

	// the number of rows in this generateLocalGrid
	if (((*gridConfiguration).HasKey("NumberOfRows"))
			&& ((*gridConfiguration)["NumberOfRows"].GetType()
					== json::ValueType::IntVal)) {
		rows = (*gridConfiguration)["NumberOfRows"].ToInt();
	} else {
		glass3::util::Logger::log(
				"error",
				"CWeb::generateLocalGrid: Missing required NumberOfRows Key.");
		return (false);
	}

	// the number of columns in this generateLocalGrid
	if (((*gridConfiguration).HasKey("NumberOfColumns"))
			&& ((*gridConfiguration)["NumberOfColumns"].GetType()
					== json::ValueType::IntVal)) {
		cols = (*gridConfiguration)["NumberOfColumns"].ToInt();
	} else {
		glass3::util::Logger::log(
				"error",
				"CWeb::generateLocalGrid: Missing required NumberOfColumns Key.");
		return (false);
	}

	// Generate initial node values
	// compute latitude distance in geographic degrees by converting
	// the provided resolution in kilometers to degrees
	// NOTE: Hard coded conversion factor
	double latDistance = getNodeResolution() / DEG2KM;

	// compute the longitude distance in geographic degrees by
	// dividing the latitude distance by the cosine of the center latitude
	double lonDistance = latDistance / cos(DEG2RAD * lat);

	// compute the middle row index
	int irow0 = rows / 2;

	// compute the middle column index
	int icol0 = cols / 2;

	// compute the maximum latitude using the provided center latitude,
	// middle row index, and the latitude distance
	double lat0 = lat + (irow0 * latDistance);

	// compute the minimum longitude using the provided center longitude,
	// middle column index, and the longitude distance
	double lon0 = lon - (icol0 * lonDistance);

	// create / open gridfile for saving
	std::ofstream outfile;
	std::ofstream outstafile;
	if (getSaveGrid()) {
		std::string filename = m_sName + "_gridfile.txt";
		outfile.open(filename, std::ios::out);
		outfile << "generateLocalGrid,NodeID,NodeLat,NodeLon,NodeDepth" << "\n";

		filename = m_sName + "_gridstafile.txt";
		outstafile.open(filename, std::ios::out);
		outstafile << "NodeID,[StationSCNL;StationLat;StationLon;StationRad],"
					<< "\n";
	}

	// init node count
	int iNodeCount = 0;

	// generate grid
	// for each row
	for (int irow = 0; irow < rows; irow++) {
		// compute the current row latitude by subtracting
		// the row index multiplied by the latitude distance from the
		// maximum latitude
		double latrow = lat0 - (irow * latDistance);

		// log the latitude
		snprintf(sLog, sizeof(sLog), "LatRow:%.2f", latrow);
		glass3::util::Logger::log("debug", sLog);
		// for each col
		for (int icol = 0; icol < cols; icol++) {
			// compute the current column longitude by adding the
			// column index multiplied by the longitude distance to the
			// minimum longitude
			double loncol = lon0 + (icol * lonDistance);

			std::lock_guard<std::mutex> guard(m_vSiteMutex);

			// sort site list for this generateLocalGrid point
			sortSiteListForNode(latrow, loncol);

			// for each depth at this generateLocalGrid point
			for (auto z : depthLayerArray) {
				// generate this node
				std::shared_ptr<CNode> node = generateNode(
						latrow, loncol, z, getNodeResolution());

				// if we got a valid node, add it
				if (addNode(node) == true) {
					iNodeCount++;
				}

				// write node to generateLocalGrid file
				if (getSaveGrid()) {
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

	// close generateLocalGrid file
	if (getSaveGrid()) {
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

	// log local grid info
	snprintf(
			sLog,
			sizeof(sLog),
			"CWeb::generateLocalGrid sName:%s Phase(s):%s; Ranges:Lat(%.2f,%.2f),"
			"Lon:(%.2f,%.2f); nRow:%d; nCol:%d; nZ:%d; resol:%.2f;"
			" nDetect:%d; nNucleate:%d; dThresh:%.2f; vNetFilter:%d;"
			" vSitesFilter:%d;  bUseOnlyTeleseismicStations:%d;"
			" iNodeCount:%d;",
			m_sName.c_str(), phases.c_str(), lat0,
			lat0 - (rows - 1) * latDistance, lon0,
			lon0 + (cols - 1) * lonDistance, rows, cols, numDepthLayers,
			getNodeResolution(), getNumStationsPerNode(),
			getNucleationDataThreshold(), getNucleationStackThreshold(),
			static_cast<int>(m_vNetworksFilter.size()),
			static_cast<int>(m_vSitesFilter.size()),
			static_cast<int>(m_bUseOnlyTeleseismicStations), iNodeCount);
	glass3::util::Logger::log("info", sLog);

	// success
	return (true);
}

// ---------------------------------------------------------generateExplicitGrid
bool CWeb::generateExplicitGrid(
		std::shared_ptr<json::Object> gridConfiguration) {
	glass3::util::Logger::log("debug",
							"CWeb::generateExplicitGrid");

	// nullchecks
	// check json
	if (gridConfiguration == NULL) {
		glass3::util::Logger::log(
				"error",
				"CWeb::generateExplicitGrid: NULL json configuration.");
		return (false);
	}

	char sLog[1024];

	// load basic (common) grid configuration
	if (loadGridConfiguration(gridConfiguration) == false) {
		return (false);
	}

	int nN = 0;
	std::vector<std::vector<double>> nodes;
	double resol = 0;

	// the number of columns in this generateLocalGrid
	if ((gridConfiguration->HasKey("NodeList"))
			&& ((*gridConfiguration)["NodeList"].GetType()
					== json::ValueType::ArrayVal)) {
		json::Array nodeList = (*gridConfiguration)["NodeList"].ToArray();

		for (const auto &val : nodeList) {
			nN++;
		}
		nodes.resize(nN);

		int i = 0;
		for (const auto &val : nodeList) {
			if (val.GetType() != json::ValueType::ObjectVal) {
				glass3::util::Logger::log(
						"error",
						"CWeb::generateExplicitGrid: Bad Node Definition found"
						" in Node List.");
				continue;
			}
			json::Object obj = val.ToObject();
			if (obj["Latitude"].GetType() != json::ValueType::DoubleVal
					|| obj["Longitude"].GetType() != json::ValueType::DoubleVal
					|| obj["Depth"].GetType() != json::ValueType::DoubleVal) {
				glass3::util::Logger::log(
						"error",
						"CWeb::generateExplicitGrid: Node Lat, Lon, or Depth not"
						" a  double in param file.");
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
		glass3::util::Logger::log(
				"error",
				"CWeb::generateExplicitGrid: Missing required NodeList Key.");
		return (false);
	}

	// create / open gridfile for saving
	std::ofstream outfile;
	std::ofstream outstafile;
	if (getSaveGrid()) {
		std::string filename = m_sName + "_gridfile.txt";
		outfile.open(filename, std::ios::out);
		outfile << "generateLocalGrid,NodeID,NodeLat,NodeLon,NodeDepth" << "\n";

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
		sortSiteListForNode(lat, lon);

		// create node, note resolution set to 0
		std::shared_ptr<CNode> node = generateNode(lat, lon, Z,
													getNodeResolution());
		if (addNode(node) == true) {
			iNodeCount++;
		}

		// write to station file
		outstafile << node->getSitesString();
	}

	// close grid file
	if (getSaveGrid()) {
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

	// log explicit grid info
	snprintf(
			sLog, sizeof(sLog),
			"CWeb::generateExplicitGrid sName:%s Phase(s):%s; nDetect:%d;"
			" nNucleate:%d; dThresh:%.2f; vNetFilter:%d;"
			" bUseOnlyTeleseismicStations:%d; vSitesFilter:%d; iNodeCount:%d;",
			m_sName.c_str(), phases.c_str(), getNumStationsPerNode(),
			getNucleationDataThreshold(), getNucleationStackThreshold(),
			static_cast<int>(m_vNetworksFilter.size()),
			static_cast<int>(m_vSitesFilter.size()),
			static_cast<int>(m_bUseOnlyTeleseismicStations), iNodeCount);
	glass3::util::Logger::log("info", sLog);

	// success
	return (true);
}

// ---------------------------------------------------------loadGridConfiguration
bool CWeb::loadGridConfiguration(
		std::shared_ptr<json::Object> gridConfiguration) {
	// nullchecks
	// check json
	if (gridConfiguration == NULL) {
		glass3::util::Logger::log(
				"error",
				"CWeb::loadGridConfiguration: NULL json configuration.");
		return (false);
	}

	// grid definition variables and defaults
	std::string name = "Nemo";
	int detect = CGlass::getNumStationsPerNode();
	int nucleate = CGlass::getNucleationDataThreshold();
	double thresh = CGlass::getNucleationStackThreshold();

	double resol = 0;
	double aziTaper = 360.0;
	double maxDepth = 800.0;
	bool saveGrid = false;
	bool update = false;

	// get grid configuration from json
	// name
	if (((*gridConfiguration).HasKey("Name"))
			&& ((*gridConfiguration)["Name"].GetType()
					== json::ValueType::StringVal)) {
		name = (*gridConfiguration)["Name"].ToString();
	}

	// Nucleation Phases to be used for this generateLocalGrid
	if (((*gridConfiguration).HasKey("NucleationPhases"))
			&& ((*gridConfiguration)["NucleationPhases"].GetType()
					== json::ValueType::ObjectVal)) {
		json::Object nucleationPhases = (*gridConfiguration)["NucleationPhases"]
				.ToObject();

		if (loadTravelTimes(&nucleationPhases) == false) {
			glass3::util::Logger::log(
					"error",
					"CWeb::loadGridConfiguration: Error Loading NucleationPhases");
			return (false);
		}
	} else {
		if (loadTravelTimes((json::Object *) NULL) == false) {
			glass3::util::Logger::log(
					"error",
					"CWeb::loadGridConfiguration: Error Loading default "
					"NucleationPhases");
			return (false);
		}
	}

	// the number of stations that are assigned to each node
	if (((*gridConfiguration).HasKey("NumStationsPerNode"))
			&& ((*gridConfiguration)["NumStationsPerNode"].GetType()
					== json::ValueType::IntVal)) {
		detect = (*gridConfiguration)["NumStationsPerNode"].ToInt();
	}

	// number of picks that need to associate to start an event
	if (((*gridConfiguration).HasKey("NucleationDataThreshold"))
			&& ((*gridConfiguration)["NucleationDataThreshold"].GetType()
					== json::ValueType::IntVal)) {
		nucleate = (*gridConfiguration)["NucleationDataThreshold"].ToInt();
	}

	// viability threshold needed to exceed for a nucleation to be successful.
	if (((*gridConfiguration).HasKey("NucleationStackThreshold"))
			&& ((*gridConfiguration)["NucleationStackThreshold"].GetType()
					== json::ValueType::DoubleVal)) {
		thresh = (*gridConfiguration)["NucleationStackThreshold"].ToDouble();
	}

	// Node resolution for this grid
	if (((*gridConfiguration).HasKey("NodeResolution"))
			&& ((*gridConfiguration)["NodeResolution"].GetType()
					== json::ValueType::DoubleVal)) {
		resol = (*gridConfiguration)["NodeResolution"].ToDouble();
	} else {
		glass3::util::Logger::log(
				"error",
				"CWeb::loadGridConfiguration: Missing required Resolution Key.");
		return (false);
	}

	// sets the aziTaper value
	if ((*gridConfiguration).HasKey("AzimuthGapTaper")
			&& ((*gridConfiguration)["AzimuthGapTaper"].GetType()
					== json::ValueType::DoubleVal)) {
		aziTaper = (*gridConfiguration)["AzimuthGapTaper"].ToDouble();
	}

	// sets the maxDepth value
	if ((*gridConfiguration).HasKey("MaximumDepth")
			&& ((*gridConfiguration)["MaximumDepth"].GetType()
					== json::ValueType::DoubleVal)) {
		maxDepth = (*gridConfiguration)["MaximumDepth"].ToDouble();
	}

	// whether to create a file detailing the node configuration for
	// this grid
	if ((gridConfiguration->HasKey("SaveGrid"))
			&& ((*gridConfiguration)["SaveGrid"].GetType()
					== json::ValueType::BoolVal)) {
		saveGrid = (*gridConfiguration)["SaveGrid"].ToBool();
	}

	// set whether to update weblists
	if ((gridConfiguration->HasKey("UpdateGrid"))
			&& ((*gridConfiguration)["UpdateGrid"].GetType()
					== json::ValueType::BoolVal)) {
		update = (*gridConfiguration)["UpdateGrid"].ToBool();
	}

	// initialize
	initialize(name, thresh, detect, nucleate, resol, update, saveGrid,
				m_pNucleationTravelTime1, m_pNucleationTravelTime2, aziTaper,
				maxDepth);

	// generate site and network filter lists
	loadSiteFilters(gridConfiguration);

	// Generate eligible station list
	loadWebSiteList();

	return (true);
}

// ---------------------------------------------------------loadTravelTimes
bool CWeb::loadTravelTimes(json::Object *gridConfiguration) {
	// check json
	if (gridConfiguration == NULL) {
		glass3::util::Logger::log(
				"info",
				"CWeb::loadTravelTimes: NULL json configuration "
				"Using default first phase and no second phase");

		// if no json object, default to P
		// clean out old phase if any
		m_pNucleationTravelTime1.reset();

		// use overall glass default if available
		if (CGlass::getDefaultNucleationTravelTime() != NULL) {
			m_pNucleationTravelTime1 = CGlass::getDefaultNucleationTravelTime();
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
	if ((gridConfiguration->HasKey("Phase1"))
			&& ((*gridConfiguration)["Phase1"].GetType()
					== json::ValueType::ObjectVal)) {
		// get the phase object
		json::Object phsObj = (*gridConfiguration)["Phase1"].ToObject();

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

			glass3::util::Logger::log(
					"info",
					"CWeb::loadTravelTimes: Using file location: " + file
							+ " for first phase: " + phs);
		} else {
			glass3::util::Logger::log(
					"info",
					"CWeb::loadTravelTimes: Using default file location for "
							"first phase: " + phs);
		}

		// set up the first phase travel time
		if (m_pNucleationTravelTime1->setup(phs, file) == false) {
			glass3::util::Logger::log(
					"error",
					"CWeb::loadTravelTimes: Failed to load file "
							"location " + file + " for first phase: " + phs);
			return (false);
		}

	} else {
		// if no first phase, use default from glass
		m_pNucleationTravelTime1.reset();

		// use overall glass default if available
		if (CGlass::getDefaultNucleationTravelTime() != NULL) {
			m_pNucleationTravelTime1 = CGlass::getDefaultNucleationTravelTime();
		} else {
			// create new traveltime
			m_pNucleationTravelTime1 =
					std::make_shared<traveltime::CTravelTime>();

			// set up the nucleation traveltime
			m_pNucleationTravelTime1->setup("P");
		}

		glass3::util::Logger::log(
				"info",
				"CWeb::loadTravelTimes: Using default first phase");
	}

	// load the second travel time
	if ((gridConfiguration->HasKey("Phase2"))
			&& ((*gridConfiguration)["Phase2"].GetType()
					== json::ValueType::ObjectVal)) {
		// get the phase object
		json::Object phsObj = (*gridConfiguration)["Phase2"].ToObject();

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

			glass3::util::Logger::log(
					"info",
					"CWeb::loadTravelTimes: Using file location: " + file
							+ " for second phase: " + phs);
		} else {
			glass3::util::Logger::log(
					"info",
					"CWeb::loadTravelTimes: Using default file location for "
							"second phase: " + phs);
		}

		// set up the second phase travel time
		if (m_pNucleationTravelTime2->setup(phs, file) == false) {
			glass3::util::Logger::log(
					"error",
					"CWeb::loadTravelTimes: Failed to load file "
							"location " + file + " for second phase: " + phs);
			return (false);
		}
	} else {
		// no second phase
		// clean out old phase if any
		m_pNucleationTravelTime2.reset();

		glass3::util::Logger::log(
				"info",
				"CWeb::loadTravelTimes: Not using secondary nucleation phase");
	}

	return (true);
}

// ---------------------------------------------------------loadSiteFilters
bool CWeb::loadSiteFilters(std::shared_ptr<json::Object> gridConfiguration) {
	// nullchecks
	// check json
	if (gridConfiguration == NULL) {
		glass3::util::Logger::log("error",
								"genSiteFilters: NULL json configuration.");
		return (false);
	}

	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);

	// Get the network names to be included in this web.
	if ((*gridConfiguration).HasKey("IncludeNetworks")
			&& ((*gridConfiguration)["IncludeNetworks"].GetType()
					== json::ValueType::ArrayVal)) {
		// clear any previous network filter list
		m_vNetworksFilter.clear();

		// get the network array
		json::Array arr = (*gridConfiguration)["IncludeNetworks"].ToArray();

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
			glass3::util::Logger::log(
					"debug",
					"CWeb::genSiteFilters: " + std::to_string(netFilterCount)
							+ " network filters configured.");
		}
	}

	// get the SCNL names to be included in this web.
	if ((*gridConfiguration).HasKey("IncludeSites")
			&& ((*gridConfiguration)["IncludeSites"].GetType()
					== json::ValueType::ArrayVal)) {
		// clear any previous site filter list
		m_vSitesFilter.clear();

		// get the site array
		int staFilterCount = 0;
		json::Array arr = (*gridConfiguration)["IncludeSites"].ToArray();

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
			glass3::util::Logger::log(
					"debug",
					"CWeb::genSiteFilters: " + std::to_string(staFilterCount)
							+ " SCNL filters configured.");
		}
	}

	// check to see if we're only to use teleseismic stations.
	if ((*gridConfiguration).HasKey("UseOnlyTeleseismicStations")
			&& ((*gridConfiguration)["UseOnlyTeleseismicStations"].GetType()
					== json::ValueType::BoolVal)) {
		m_bUseOnlyTeleseismicStations =
				(*gridConfiguration)["UseOnlyTeleseismicStations"].ToBool();

		glass3::util::Logger::log(
				"debug",
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

// ---------------------------------------------------------loadWebSiteList
bool CWeb::loadWebSiteList() {
	// nullchecks
	// check pSiteList
	if (m_pSiteList == NULL) {
		glass3::util::Logger::log(
				"error",
				"CWeb::loadWebSiteList: NULL pSiteList pointer.");
		return (false);
	}

	// get the total number sites in glass's site list
	int nsite = m_pSiteList->size();

	// don't bother continuing if we have no sites
	if (nsite <= 0) {
		glass3::util::Logger::log(
				"warning",
				"CWeb::loadWebSiteList: No sites in site list.");
		return (false);
	}

	// log
	char sLog[1024];
	snprintf(sLog, sizeof(sLog),
				"CWeb::loadWebSiteList: %d sites available for web %s", nsite,
				m_sName.c_str());
	glass3::util::Logger::log("debug", sLog);

	// clear web site list
	m_vSitesSortedForCurrentNode.clear();

	std::vector<std::shared_ptr<CSite>> siteList =
			m_pSiteList->getListOfSites();

	// for each site
	for (std::vector<std::shared_ptr<CSite>>::iterator it = siteList.begin();
			it != siteList.end(); ++it) {
		// get site from the overall site list
		std::shared_ptr<CSite> site = *it;

		if (site == NULL) {
			continue;
		}

		// Ignore if station out of service
		if (!site->getUse()) {
			continue;
		}
		if (!site->getEnable()) {
			continue;
		}

		if (isSiteAllowed(site)) {
			m_vSitesSortedForCurrentNode.push_back(
					std::pair<double, std::shared_ptr<CSite>>(0.0, site));
		}
	}

	// log
	snprintf(sLog, sizeof(sLog),
				"CWeb::loadWebSiteList: %d sites selected for web %s",
				static_cast<int>(m_vSitesSortedForCurrentNode.size()),
				m_sName.c_str());
	glass3::util::Logger::log("info", sLog);

	return (true);
}

// ---------------------------------------------------------sortSiteListForNode
void CWeb::sortSiteListForNode(double lat, double lon) {
	// set to provided geographic location
	glass3::util::Geo geo;

	// NOTE: node depth is ignored here
	geo.setGeographic(lat, lon, 6371.0);

	// set the distance to each site
	for (int i = 0; i < m_vSitesSortedForCurrentNode.size(); i++) {
		// get the site
		auto p = m_vSitesSortedForCurrentNode[i];
		std::shared_ptr<CSite> site = p.second;

		// compute the distance
		p.first = site->getDelta(&geo);

		// set the distance in the vector
		m_vSitesSortedForCurrentNode[i] = p;
	}

	// sort sites
	std::sort(m_vSitesSortedForCurrentNode.begin(),
				m_vSitesSortedForCurrentNode.end(), sortSite);
}

// ---------------------------------------------------------generateNode
std::shared_ptr<CNode> CWeb::generateNode(double lat, double lon, double z,
											double resol) {
	// nullcheck
	if ((m_pNucleationTravelTime1 == NULL)
			&& (m_pNucleationTravelTime2 == NULL)) {
		glass3::util::Logger::log("error",
								"CWeb::genNode: No valid trav pointers.");
		return (NULL);
	}

	// create node
	std::shared_ptr<CNode> node(new CNode(m_sName, lat, lon, z, resol));

	// set parent web
	node->setWeb(this);

	// return empty node if we don't
	// have any sites
	if (m_vSitesSortedForCurrentNode.size() == 0) {
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
		glass3::util::Logger::log("error",
								"CWeb::genNodeSites: NULL node pointer.");
		return (NULL);
	}
	// check trav
	if ((m_pNucleationTravelTime1 == NULL)
			&& (m_pNucleationTravelTime2 == NULL)) {
		glass3::util::Logger::log("error",
								"CWeb::genNodeSites: No valid trav pointers.");
		return (NULL);
	}
	// check sites
	if (m_vSitesSortedForCurrentNode.size() == 0) {
		glass3::util::Logger::log("error",
								"CWeb::genNodeSites: No sites.");
		return (node);
	}
	// check nDetect
	if (m_iNumStationsPerNode == 0) {
		glass3::util::Logger::log("error",
								"CWeb::genNodeSites: nDetect is 0.");
		return (node);
	}

	int sitesAllowed = m_iNumStationsPerNode;
	if (m_vSitesSortedForCurrentNode.size() < m_iNumStationsPerNode) {
		glass3::util::Logger::log("warning",
								"CWeb::genNodeSites: nDetect is greater "
								"than the number of sites.");
		sitesAllowed = m_vSitesSortedForCurrentNode.size();
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
		auto aSite = m_vSitesSortedForCurrentNode[i];
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
		node->linkSite(site, node, delta, travelTime1, travelTime2);
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

	glass3::util::Logger::log(
			"debug",
			"CWeb::addSite: New potential station " + site->getSCNL()
					+ " for web: " + m_sName + ".");

	// don't bother if this site isn't allowed
	if (isSiteAllowed(site) == false) {
		glass3::util::Logger::log(
				"debug",
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
			glass3::util::Logger::log(
					"debug",
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
		glass3::util::Geo geo;
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
		if ((node->getSiteLinksCount() >= m_iNumStationsPerNode)
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
		if (node->getSiteLinksCount() < m_iNumStationsPerNode) {
			// Link node to site using traveltimes
			node->linkSite(site, node, newDistance, travelTime1, travelTime2);

		} else {
			// remove last site
			// This assumes that the node site list is sorted
			// on distance/traveltime
			node->unlinkLastSite();

			// Link node to site using traveltimes
			node->linkSite(site, node, newDistance, travelTime1, travelTime2);
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
		glass3::util::Logger::log("info", sLog);
	} else {
		glass3::util::Logger::log(
				"debug",
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
		glass3::util::Logger::log(
				"debug",
				"CWeb::remSite: Station " + site->getSCNL()
						+ " not allowed in web " + m_sName + ".");
		return;
	}

	glass3::util::Logger::log(
			"debug",
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
			glass3::util::Logger::log(
					"debug",
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
			loadWebSiteList();
			bSiteList = true;

			// make sure we've got enough sites for a node
			if (m_vSitesSortedForCurrentNode.size() < m_iNumStationsPerNode) {
				node->setEnabled(true);
				return;
			}
		}

		// sort overall list of sites for this node
		sortSiteListForNode(node->getLatitude(), node->getLongitude());

		// remove site link
		if (node->unlinkSite(foundSite) == true) {
			// get new site
			auto nextSite = m_vSitesSortedForCurrentNode[m_iNumStationsPerNode];
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
			if (node->linkSite(newSite, node, newDistance, travelTime1,
								travelTime2) == false) {
				glass3::util::Logger::log(
						"error",
						"CWeb::remSite: Failed to add station "
								+ newSite->getSCNL() + " to web " + m_sName
								+ ".");
			}

			// resort site links
			node->sortSiteLinks();

			// we've removed a site
			nodeModCount++;
		} else {
			glass3::util::Logger::log(
					"error",
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
		glass3::util::Logger::log("info", sLog);
	} else {
		glass3::util::Logger::log(
				"debug",
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
			glass3::util::Logger::log(
					"error",
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
	glass3::util::Logger::log("debug",
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
		glass3::util::Logger::log(
				"error",
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

// ---------------------------------------------------------getUpdate
bool CWeb::getUpdate() const {
	return (m_bUpdate);
}

// ---------------------------------------------------------getSaveGrid
bool CWeb::getSaveGrid() const {
	return (m_bSaveGrid);
}

// ---------------------------------------------------------getResolution
double CWeb::getNodeResolution() const {
	return (m_dNodeResolution);
}

// --------------------------------------------------getNucleationStackThreshold
double CWeb::getNucleationStackThreshold() const {
	return (m_dNucleationStackThreshold);
}

// -------------------------------------------------------getNumStationsPerNode
int CWeb::getNumStationsPerNode() const {
	return (m_iNumStationsPerNode);
}

// ---------------------------------------------------getNucleationDataThreshold
int CWeb::getNucleationDataThreshold() const {
	return (m_iNucleationDataThreshold);
}

// ---------------------------------------------------------getName
const std::string& CWeb::getName() const {
	return (m_sName);
}

// -----------------------------------------------------getNucleationTravelTime1
const std::shared_ptr<traveltime::CTravelTime>& CWeb::getNucleationTravelTime1() const {  // NOLINT
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
	return (m_pNucleationTravelTime1);
}

// -----------------------------------------------------getNucleationTravelTime2
const std::shared_ptr<traveltime::CTravelTime>& CWeb::getNucleationTravelTime2() const {  // NOLINT
	std::lock_guard<std::recursive_mutex> webGuard(m_WebMutex);
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

