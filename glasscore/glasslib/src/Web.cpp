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
CWeb::CWeb(bool createBackgroundThread, int sleepTime, int checkInterval) {
	// setup threadsto
	m_bUseBackgroundThread = createBackgroundThread;
	m_iSleepTimeMS = sleepTime;
	m_iStatusCheckInterval = checkInterval;
	std::time(&tLastStatusCheck);

	clear();

	// start the thread
	if (m_bUseBackgroundThread == true) {
		m_bRunJobLoop = true;
		m_BackgroundThread = new std::thread(&CWeb::workLoop, this);
		m_bThreadStatus = true;
	} else {
		m_bRunJobLoop = false;
		m_BackgroundThread = NULL;
		m_bThreadStatus = false;
	}
}

// ---------------------------------------------------------CWeb
CWeb::CWeb(std::string name, double thresh, int numDetect, int numNucleate,
			int resolution, int numRows, int numCols, int numZ, bool update,
			std::shared_ptr<traveltime::CTravelTime> firstTrav,
			std::shared_ptr<traveltime::CTravelTime> secondTrav,
			bool createBackgroundThread, int sleepTime, int checkInterval) {
	// setup threads
	m_bUseBackgroundThread = createBackgroundThread;
	m_iSleepTimeMS = sleepTime;
	m_iStatusCheckInterval = checkInterval;
	std::time(&tLastStatusCheck);

	clear();

	initialize(name, thresh, numDetect, numNucleate, resolution, numRows,
				numCols, numZ, update, firstTrav, secondTrav);

	// start the thread
	if (m_bUseBackgroundThread == true) {
		m_bRunJobLoop = true;
		m_BackgroundThread = new std::thread(&CWeb::workLoop, this);
		m_bThreadStatus = true;
	} else {
		m_bRunJobLoop = false;
		m_BackgroundThread = NULL;
		m_bThreadStatus = false;
	}
}

// ---------------------------------------------------------~CWeb
CWeb::~CWeb() {
	glassutil::CLogit::log("CWeb::~CWeb");

	clear();

	if (m_bUseBackgroundThread == true) {
		// stop the thread
		m_bRunJobLoop = false;

		// wait for the thread to finish
		m_BackgroundThread->join();

		// delete it
		delete (m_BackgroundThread);
		m_BackgroundThread = NULL;
	}
}

// ---------------------------------------------------------clear
void CWeb::clear() {
	nRow = 0;
	nCol = 0;
	nZ = 0;
	nDetect = 10;
	nNucleate = 5;
	dThresh = 2.5;
	dResolution = 100;
	sName = "Nemo";
	pGlass = NULL;
	pSiteList = NULL;
	bUpdate = false;

	// clear out all the nodes in the web
	for (auto &node : vNode) {
		node->clear();
	}
	vNode.clear();

	// clear the network and site filters
	vNetFilter.clear();
	vSitesFilter.clear();

	// clear sites
	vSite.clear();

	pTrv1 = NULL;
	pTrv2 = NULL;
}

// ---------------------------------------------------------Initialize
bool CWeb::initialize(std::string name, double thresh, int numDetect,
						int numNucleate, int resolution, int numRows,
						int numCols, int numZ, bool update,
						std::shared_ptr<traveltime::CTravelTime> firstTrav,
						std::shared_ptr<traveltime::CTravelTime> secondTrav) {
	sName = name;
	dThresh = thresh;
	nDetect = numDetect;
	nNucleate = numNucleate;
	dResolution = resolution;
	nRow = numRows;
	nCol = numCols;
	nZ = numZ;
	bUpdate = update;
	pTrv1 = firstTrav;
	pTrv2 = secondTrav;

	// done
	return (true);
}

// ---------------------------------------------------------Dispatch
bool CWeb::dispatch(json::Object *com) {
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
			return (grid_explicit(com));
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------Global
bool CWeb::global(json::Object*com) {
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
	if (pGlass != NULL) {
		detect = pGlass->nDetect;
		nucleate = pGlass->nNucleate;
		thresh = pGlass->dThresh;
	}

	double resol = 100;
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
	initialize(name, thresh, detect, nucleate, resol, 0, 0, zs, update, pTrv1,
				pTrv2);

	// generate site and network filter lists
	genSiteFilters(com);

	// Generate eligible station list
	genSiteList();

	// calculate the number of nodes from the desired resolution
	// This function was empirically determined via using different
	// numNode values and computing the average resolution from a node to
	// the nearest other 6 nodes
	int numNodes = 5.0E8 * std::pow(dResolution, -1.965);

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
		std::string filename = sName + "_gridfile.txt";
		outfile.open(filename, std::ios::out);
		outfile << "Grid,NodeID,NodeLat,NodeLon,NodeDepth" << "\n";

		filename = sName + "_gridstafile.txt";
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
		std::lock_guard<std::mutex> guard(vSiteMutex);

		// sort site list for this vertex
		sortSiteList(aLat, aLon);

		// for each depth
		for (auto z : zzz) {
			// create node
			std::shared_ptr<CNode> node = genNode(aLat, aLon, z, dResolution);

			// if we got a valid node, add it
			if (addNode(node) == true) {
				iNodeCount++;

				// write node to grid file
				if (saveGrid) {
					outfile << sName << "," << node->getPid() << ","
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
	if (pTrv1 != NULL) {
		phases += pTrv1->sPhase;
	}
	if (pTrv2 != NULL) {
		phases += ", " + pTrv2->sPhase;
	}

	// log global grid info
	snprintf(
			sLog, sizeof(sLog),
			"CWeb::global sName:%s Phase(s):%s; nZ:%d; resol:%.2f; nDetect:%d;"
			" nNucleate:%d; dThresh:%.2f; vNetFilter:%d;"
			" vSitesFilter:%d; iNodeCount:%d;",
			sName.c_str(), phases.c_str(), nZ, dResolution, nDetect, nNucleate,
			dThresh, static_cast<int>(vNetFilter.size()),
			static_cast<int>(vSitesFilter.size()), iNodeCount);
	glassutil::CLogit::log(glassutil::log_level::info, sLog);

	// success
	return (true);
}

// ---------------------------------------------------------Grid
bool CWeb::grid(json::Object *com) {
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
	if (pGlass != NULL) {
		detect = pGlass->nDetect;
		nucleate = pGlass->nNucleate;
		thresh = pGlass->dThresh;
	}

	double resol = 0;
	double lat = 0;
	double lon = 0;
	int rows = 0;
	int cols = 0;
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
				pTrv1, pTrv2);

	// generate site and network filter lists
	genSiteFilters(com);

	// Generate eligible station list
	genSiteList();

	// Generate initial node values
	// compute latitude distance in geographic degrees by converting
	// the provided resolution in kilometers to degrees
	// NOTE: Hard coded conversion factor
	double latDistance = resol / 111.11;

	// compute the longitude distance in geographic degrees by
	// dividing the latitude distance by the cosine of the center latitude
	double lonDistance = latDistance / cos(DEG2RAD * lat);

	// compute the middle row index
	int irow0 = nRow / 2;

	// compute the middle column index
	int icol0 = nCol / 2;

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
		std::string filename = sName + "_gridfile.txt";
		outfile.open(filename, std::ios::out);
		outfile << "Grid,NodeID,NodeLat,NodeLon,NodeDepth" << "\n";

		filename = sName + "_gridstafile.txt";
		outstafile.open(filename, std::ios::out);
		outstafile << "NodeID,[StationSCNL;StationLat;StationLon;StationRad],"
					<< "\n";
	}

	// init node count
	int iNodeCount = 0;

	// generate grid
	// for each row
	for (int irow = 0; irow < nRow; irow++) {
		// compute the current row latitude by subtracting
		// the row index multiplied by the latitude distance from the
		// maximum latitude
		double latrow = lat0 - (irow * latDistance);

		// log the latitude
		snprintf(sLog, sizeof(sLog), "LatRow:%.2f", latrow);
		glassutil::CLogit::log(glassutil::log_level::debug, sLog);
		// for each col
		for (int icol = 0; icol < nCol; icol++) {
			// compute the current column longitude by adding the
			// column index multiplied by the longitude distance to the
			// minimum longitude
			double loncol = lon0 + (icol * lonDistance);

			std::lock_guard<std::mutex> guard(vSiteMutex);

			// sort site list for this grid point
			sortSiteList(latrow, loncol);

			// for each depth at this grid point
			for (auto z : zzz) {
				// generate this node
				std::shared_ptr<CNode> node = genNode(latrow, loncol, z,
														dResolution);

				// if we got a valid node, add it
				if (addNode(node) == true) {
					iNodeCount++;
				}

				// write node to grid file
				if (saveGrid) {
					outfile << sName << "," << node->getPid() << ","
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
	if (pTrv1 != NULL) {
		phases += pTrv1->sPhase;
	}
	if (pTrv2 != NULL) {
		phases += ", " + pTrv2->sPhase;
	}

	// log grid info
	snprintf(sLog, sizeof(sLog),
				"CWeb::grid sName:%s Phase(s):%s; Ranges:Lat(%.2f,%.2f),"
				"Lon:(%.2f,%.2f); nRow:%d; nCol:%d; nZ:%d; resol:%.2f;"
				" nDetect:%d; nNucleate:%d; dThresh:%.2f; vNetFilter:%d;"
				" vSitesFilter:%d; iNodeCount:%d;",
				sName.c_str(), phases.c_str(), lat0,
				lat0 - (nRow - 1) * latDistance, lon0,
				lon0 + (nCol - 1) * lonDistance, nRow, nCol, nZ, dResolution,
				nDetect, nNucleate, dThresh,
				static_cast<int>(vNetFilter.size()),
				static_cast<int>(vSitesFilter.size()), iNodeCount);
	glassutil::CLogit::log(glassutil::log_level::info, sLog);

	// success
	return (true);
}

// ---------------------------------------------Grid w/ explicit nodes
bool CWeb::grid_explicit(json::Object *com) {
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
	if (pGlass != NULL) {
		detect = pGlass->nDetect;
		nucleate = pGlass->nNucleate;
		thresh = pGlass->dThresh;
	}

	int nN = 0;
	bool saveGrid = false;
	bool update = false;
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
	initialize(name, thresh, detect, nucleate, resol, 0., 0., 0., update, pTrv1,
				pTrv2);

	// generate site and network filter lists
	genSiteFilters(com);

	// Generate eligible station list
	genSiteList();

	// create / open gridfile for saving
	std::ofstream outfile;
	std::ofstream outstafile;
	if (saveGrid) {
		std::string filename = sName + "_gridfile.txt";
		outfile.open(filename, std::ios::out);
		outfile << "Grid,NodeID,NodeLat,NodeLon,NodeDepth" << "\n";

		filename = sName + "_gridstafile.txt";
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

		std::lock_guard<std::mutex> guard(vSiteMutex);

		// sort site list
		sortSiteList(lat, lon);

		// create node, note resolution set to 0
		std::shared_ptr<CNode> node = genNode(lat, lon, Z, resol);
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
	if (pTrv1 != NULL) {
		phases += pTrv1->sPhase;
	}
	if (pTrv2 != NULL) {
		phases += ", " + pTrv2->sPhase;
	}

	// log grid info
	snprintf(
			sLog,
			sizeof(sLog),
			"CWeb::grid_explicit sName:%s Phase(s):%s; nDetect:%d; nNucleate:%d;"
			" dThresh:%.2f; vNetFilter:%d; vSitesFilter:%d;"
			" iNodeCount:%d;",
			sName.c_str(), phases.c_str(), nDetect, nNucleate, dThresh,
			static_cast<int>(vNetFilter.size()),
			static_cast<int>(vSitesFilter.size()), iNodeCount);
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
		pTrv1.reset();

		// use overall glass default if available
		if ((pGlass != NULL) && (pGlass->pTrvDefault != NULL)) {
			pTrv1 = pGlass->pTrvDefault;
		} else {
			// create new traveltime
			pTrv1 = std::make_shared<traveltime::CTravelTime>();

			// set up the nucleation traveltime
			pTrv1->setup("P");
		}

		// no second phase
		// clean out old phase if any
		pTrv2.reset();

		return (true);
	}

	// load the first travel time
	if ((com->HasKey("Phase1"))
			&& ((*com)["Phase1"].GetType() == json::ValueType::ObjectVal)) {
		// get the phase object
		json::Object phsObj = (*com)["Phase1"].ToObject();

		// clean out old phase if any
		pTrv1.reset();

		// create new traveltime
		pTrv1 = std::make_shared<traveltime::CTravelTime>();

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
		if (pTrv1->setup(phs, file) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::loadTravelTimes: Failed to load file "
							"location " + file + " for first phase: " + phs);
			return (false);
		}

	} else {
		// if no first phase, use default from glass
		pTrv1.reset();

		// use overall glass default if available
		if (pGlass->pTrvDefault != NULL) {
			pTrv1 = pGlass->pTrvDefault;
		} else {
			// create new traveltime
			pTrv1 = std::make_shared<traveltime::CTravelTime>();

			// set up the nucleation traveltime
			pTrv1->setup("P");
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
		pTrv2.reset();

		// create new travel time
		pTrv2 = std::make_shared<traveltime::CTravelTime>();

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
		if (pTrv2->setup(phs, file) == false) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::loadTravelTimes: Failed to load file "
							"location " + file + " for second phase: " + phs);
			return (false);
		}
	} else {
		// no second phase
		// clean out old phase if any
		pTrv2.reset();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CWeb::loadTravelTimes: Not using secondary nuclation phase");
	}

	return (true);
}

// ---------------------------------------------------------genSiteFilters
bool CWeb::genSiteFilters(json::Object *com) {
	// nullchecks
	// check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"genSiteFilters: NULL json configuration.");
		return (false);
	}

	// Get the network names to be included in this web.
	if ((*com).HasKey("Nets")
			&& ((*com)["Nets"].GetType() == json::ValueType::ArrayVal)) {
		// clear any previous network filter list
		vNetFilter.clear();

		// get the network array
		json::Array arr = (*com)["Nets"].ToArray();

		// for each network in the array
		int netFilterCount = 0;
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].GetType() == json::ValueType::StringVal) {
				// get the network
				std::string snet = arr[i].ToString();

				// add to the network filter list
				vNetFilter.push_back(snet);
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
		vSitesFilter.clear();

		// get the site array
		int staFilterCount = 0;
		json::Array arr = (*com)["Sites"].ToArray();

		// for each site in the array
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].GetType() == json::ValueType::StringVal) {
				// get the scnl
				std::string scnl = arr[i].ToString();

				// add to the site filter list
				vSitesFilter.push_back(scnl);
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

	return (true);
}

bool CWeb::isSiteAllowed(std::shared_ptr<CSite> site) {
	if (site == NULL) {
		return (false);
	}

	// If we have do not have a site and/or network filter, just add it
	if ((vNetFilter.size() == 0) && (vSitesFilter.size() == 0)) {
		return (true);
	}

	// if we have a network filter, make sure network is allowed before adding
	if ((vNetFilter.size() > 0)
			&& (find(vNetFilter.begin(), vNetFilter.end(), site->getNet())
					!= vNetFilter.end())) {
		return (true);
	}

	// if we have a site filter, make sure site is allowed before adding
	if ((vSitesFilter.size() > 0)
			&& (find(vSitesFilter.begin(), vSitesFilter.end(), site->getScnl())
					!= vSitesFilter.end())) {
		return (true);
	}

	return (false);
}

// ---------------------------------------------------------genSiteList
bool CWeb::genSiteList() {
	// nullchecks
	// check pSiteList
	if (pSiteList == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genSiteList: NULL pSiteList pointer.");
		return (false);
	}

	// get the total number sites in glass's site list
	int nsite = pSiteList->getSiteCount();

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
				sName.c_str());
	glassutil::CLogit::log(glassutil::log_level::debug, sLog);

	// clear web site list
	vSite.clear();

	// for each site
	for (int isite = 0; isite < nsite; isite++) {
		// get site from the overall site list
		std::shared_ptr<CSite> site = pSiteList->getSite(isite);

		if (site->getUse() == false) {
			continue;
		}

		if (isSiteAllowed(site)) {
			vSite.push_back(
					std::pair<double, std::shared_ptr<CSite>>(0.0, site));
		}
	}

	// log
	snprintf(sLog, sizeof(sLog),
				"CWeb::genSiteList: %d sites selected for web %s",
				static_cast<int>(vSite.size()), sName.c_str());
	glassutil::CLogit::log(glassutil::log_level::info, sLog);

	return (true);
}

// ---------------------------------------------------------sortSite
void CWeb::sortSiteList(double lat, double lon) {
	// set to provided geographic location
	glassutil::CGeo geo;

	// NOTE: node depth is ignored here
	geo.setGeographic(lat, lon, 6371.0);

	// set the distance to each site
	for (int i = 0; i < vSite.size(); i++) {
		// get the site
		auto p = vSite[i];
		std::shared_ptr<CSite> site = p.second;

		// compute the distance
		p.first = site->getDelta(&geo);

		// set the distance in the vector
		vSite[i] = p;
	}

	// sort sites
	std::sort(vSite.begin(), vSite.end(), sortSite);
}

// ---------------------------------------------------------genNode
std::shared_ptr<CNode> CWeb::genNode(double lat, double lon, double z,
										double resol) {
	// nullcheck
	if ((pTrv1 == NULL) && (pTrv2 == NULL)) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genNode: No valid trav pointers.");
		return (NULL);
	}

	// create node
	std::shared_ptr<CNode> node(
			new CNode(sName, lat, lon, z, resol, glassutil::CPid::pid()));

	// set parent web
	node->setWeb(this);

	// return empty node if we don't
	// have any sites
	if (vSite.size() == 0) {
		return (node);
	}

	// generate the sites for the node
	node = genNodeSites(node);

	// return the populated node
	return (node);
}

// ---------------------------------------------------------addNode
bool CWeb::addNode(std::shared_ptr<CNode> node) {
	// nullcheck
	if (node == NULL) {
		return (false);
	}

	vNode.push_back(node);

	return (true);
}

std::shared_ptr<CNode> CWeb::genNodeSites(std::shared_ptr<CNode> node) {
	// nullchecks
	// check node
	if (node == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genNodeSites: NULL node pointer.");
		return (NULL);
	}
	// check trav
	if ((pTrv1 == NULL) && (pTrv2 == NULL)) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genNodeSites: No valid trav pointers.");
		return (NULL);
	}
	// check sites
	if (vSite.size() == 0) {
		return (node);
	}
	// check nDetect
	if (nDetect == 0) {
		return (node);
	}
	if (vSite.size() < nDetect) {
		return (node);
	}

	// setup traveltimes for this node
	if (pTrv1 != NULL) {
		pTrv1->setOrigin(node->getLat(), node->getLon(), node->getZ());
	}
	if (pTrv2 != NULL) {
		pTrv2->setOrigin(node->getLat(), node->getLon(), node->getZ());
	}

	// clear node of any existing sites
	node->clearSiteLinks();

	// for the number of allowed sites per node
	for (int i = 0; i < nDetect; i++) {
		// get each site
		auto aSite = vSite[i];
		std::shared_ptr<CSite> site = aSite.second;

		// compute delta distance between site and node
		double delta = RAD2DEG * aSite.first;

		// compute traveltimes between site and node
		double travelTime1 = -1;
		if (pTrv1 != NULL) {
			travelTime1 = pTrv1->T(delta);
		}

		double travelTime2 = -1;
		if (pTrv2 != NULL) {
			travelTime2 = pTrv2->T(delta);
		}

		// Link node to site using traveltimes
		node->linkSite(site, node, travelTime1, travelTime2);
	}

	// sort the site links
	node->sortSiteLinks();

	// return updated node
	return (node);
}

void CWeb::addSite(std::shared_ptr<CSite> site) {
	//  nullcheck
	if (site == NULL) {
		return;
	}

	// if this is a remove, send to remSite
	if (site->getUse() == false) {
		remSite(site);
		return;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CWeb::addSite: New potential station " + site->getScnl()
					+ " for web: " + sName + ".");

	// don't bother if this site isn't allowed
	if (isSiteAllowed(site) == false) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::addSite: Station " + site->getScnl()
						+ " not allowed in web " + sName + ".");
		return;
	}

	int nodeModCount = 0;
	int nodeCount = 0;
	int totalNodes = vNode.size();

	// for each node in web
	for (auto &node : vNode) {
		nodeCount++;

		node->setEnabled(false);

		if (nodeCount % 1000 == 0) {
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CWeb::addSite: Station " + site->getScnl() + " processed "
							+ std::to_string(nodeCount) + " out of "
							+ std::to_string(totalNodes) + " nodes in web: "
							+ sName + ". Modified "
							+ std::to_string(nodeModCount) + " nodes.");
		}

		// check to see if we have this site
		std::shared_ptr<CSite> foundSite = node->getSite(site->getScnl());

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
		geo.setGeographic(node->getLat(), node->getLon(), 6371.0);

		// compute delta distance between site and node
		double newDistance = RAD2DEG * site->getGeo().delta(&geo);

		// get site in node list
		// NOTE: this assumes that the node site list is sorted
		// on distance
		std::shared_ptr<CSite> furthestSite = node->getLastSite();

		// compute distance to farthest site
		double maxDistance = RAD2DEG * geo.delta(&furthestSite->getGeo());

		// Ignore if new site is farther than last linked site
		if (newDistance > maxDistance) {
			node->setEnabled(true);
			continue;
		}

		// setup traveltimes for this node
		if (pTrv1 != NULL) {
			pTrv1->setOrigin(node->getLat(), node->getLon(), node->getZ());
		}
		if (pTrv2 != NULL) {
			pTrv2->setOrigin(node->getLat(), node->getLon(), node->getZ());
		}

		// compute traveltimes between site and node
		double travelTime1 = -1;
		if (pTrv1 != NULL) {
			travelTime1 = pTrv1->T(newDistance);
		}
		double travelTime2 = -1;
		if (pTrv2 != NULL) {
			travelTime2 = pTrv2->T(newDistance);
		}

		// remove last site
		// This assumes that the node site list is sorted
		// on distance/traveltime
		node->unlinkLastSite();

		// Link node to site using traveltimes
		node->linkSite(site, node, travelTime1, travelTime2);

		// resort site links
		node->sortSiteLinks();

		// we've added a site
		nodeModCount++;

		node->setEnabled(true);

		// update thread status
		setStatus(true);
	}

	// log info if we've added a site
	if (nodeModCount > 0) {
		char sLog[1024];
		snprintf(sLog, sizeof(sLog), "CWeb::addSite: Added site: %s to %d "
					"node(s) in web: %s",
					site->getScnl().c_str(), nodeModCount, sName.c_str());
		glassutil::CLogit::log(glassutil::log_level::info, sLog);
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::addSite: Station " + site->getScnl()
						+ " not added to any "
								"nodes in web: " + sName + ".");
	}
}

// ---------------------------------------------------------remSite
void CWeb::remSite(std::shared_ptr<CSite> site) {
	//  nullcheck
	if (site == NULL) {
		return;
	}

	// don't bother if this site isn't allowed
	if (isSiteAllowed(site) == false) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::remSite: Station " + site->getScnl()
						+ " not allowed in web " + sName + ".");
		return;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CWeb::remSite: Trying to remove station " + site->getScnl()
					+ " from web " + sName + ".");

	// init flag to check to see if we've generated a site list for this web
	// yet
	bool bSiteList = false;
	int nodeCount = 0;

	// for each node in web
	for (auto &node : vNode) {
		// search through each site linked to this node, see if we have it
		std::shared_ptr<CSite> foundSite = node->getSite(site->getScnl());

		// don't bother if this node doesn't have this site
		if (foundSite == NULL) {
			continue;
		}

		node->setEnabled(false);
		std::lock_guard<std::mutex> guard(vSiteMutex);

		// generate the site list for this web if this is the first
		// iteration where a node is modified
		if (!bSiteList) {
			// generate the site list
			genSiteList();
			bSiteList = true;

			// make sure we've got enough sites for a node
			if (vSite.size() < nDetect) {
				node->setEnabled(true);
				return;
			}
		}

		// sort overall list of sites for this node
		sortSiteList(node->getLat(), node->getLon());

		// remove site link
		if (node->unlinkSite(foundSite) == true) {
			// get new site
			auto nextSite = vSite[nDetect];
			std::shared_ptr<CSite> newSite = nextSite.second;

			// compute delta distance between site and node
			double newDistance = RAD2DEG * nextSite.first;

			// compute traveltimes between site and node
			double travelTime1 = -1;
			if (pTrv1 != NULL) {
				travelTime1 = pTrv1->T(newDistance);
			}
			double travelTime2 = -1;
			if (pTrv2 != NULL) {
				travelTime2 = pTrv2->T(newDistance);
			}

			// Link node to new site using traveltimes
			if (node->linkSite(newSite, node, travelTime1, travelTime2)
					== false) {
				glassutil::CLogit::log(
						glassutil::log_level::error,
						"CWeb::remSite: Failed to add station "
								+ newSite->getScnl() + " to web " + sName
								+ ".");
			}

			// resort site links
			node->sortSiteLinks();

			// we've removed a site
			nodeCount++;
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::remSite: Failed to remove station " + site->getScnl()
							+ " from web " + sName + ".");
		}

		node->setEnabled(true);

		// update thread status
		setStatus(true);
	}

	// log info if we've removed a site
	char sLog[1024];
	if (nodeCount > 0) {
		snprintf(
				sLog, sizeof(sLog),
				"CWeb::remSite: Removed site: %s from %d node(s) in web: %s",
				site->getScnl().c_str(), nodeCount, sName.c_str());
		glassutil::CLogit::log(glassutil::log_level::info, sLog);
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::remSite: Station " + site->getScnl()
						+ " not removed from any nodes in web: " + sName + ".");
	}
}

// ---------------------------------------------------------addJob
void CWeb::addJob(std::function<void()> newjob) {
	if (m_bUseBackgroundThread == false) {
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

// ---------------------------------------------------------workLoop
void CWeb::workLoop() {
	glassutil::CLogit::log(glassutil::log_level::debug,
							"CWeb::jobLoop: startup");

	while (m_bRunJobLoop == true) {
		// make sure we're still running
		if (m_bRunJobLoop == false)
			break;

		// update thread status
		setStatus(true);

		// lock for queue access
		m_QueueMutex.lock();

		// are there any jobs
		if (m_JobQueue.empty() == true) {
			// unlock and skip until next time
			m_QueueMutex.unlock();

			// give up some time at the end of the loop
			jobSleep();

			// on to the next loop
			continue;
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
					"CWeb::jobLoop: Exception during job(): "
							+ std::string(e.what()));
			break;
		}

		// give up some time at the end of the loop
		jobSleep();
	}

	setStatus(false);
	glassutil::CLogit::log(glassutil::log_level::debug,
							"CWeb::jobLoop: thread exit");
}

// ---------------------------------------------------------statusCheck
bool CWeb::statusCheck() {
	// if we have a negative check interval,
	// we shouldn't worry about thread status checks.
	if (m_iStatusCheckInterval < 0) {
		return (true);
	}
	if (m_bUseBackgroundThread == false) {
		return (true);
	}

	// thread is dead if we're not running
	if (m_bRunJobLoop == false) {
		glassutil::CLogit::log(
				glassutil::log_level::warn,
				"CWeb::statusCheck(): m_bRunProcessLoop is false.");
		return (false);
	}

	// see if it's time to check
	time_t tNow;
	std::time(&tNow);
	if ((tNow - tLastStatusCheck) >= m_iStatusCheckInterval) {
		// get the thread status
		m_StatusMutex.lock();

		// The thread is dead
		if (m_bThreadStatus != true) {
			m_StatusMutex.unlock();

			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::statusCheck(): Thread"
							" did not respond in the last"
							+ std::to_string(m_iStatusCheckInterval)
							+ "seconds.");

			return (false);
		}

		// mark check as false until next time
		// if the thread is alive, it'll mark it
		// as true again.
		m_bThreadStatus = false;

		m_StatusMutex.unlock();

		// remember the last time we checked
		tLastStatusCheck = tNow;
	}

	// everything is awesome
	return (true);
}

// ---------------------------------------------------------setStatus
void CWeb::setStatus(bool status) {
	// update thread status
	m_StatusMutex.lock();
	m_bThreadStatus = status;
	m_StatusMutex.unlock();
}

// ---------------------------------------------------------jobSleep
void CWeb::jobSleep() {
	if (m_bRunJobLoop == true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(m_iSleepTimeMS));
	}
}

bool CWeb::hasSite(std::shared_ptr<CSite> site) {
	//  nullcheck
	if (site == NULL) {
		return (false);
	}

	// for each node in web
	for (auto &node : vNode) {
		// check to see if we have this site
		if (node->getSite(site->getScnl()) != NULL) {
			return (true);
		}
	}

	// site not found
	return (false);
}
}  // namespace glasscore
