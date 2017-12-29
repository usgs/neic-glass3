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

// vertex sorting function
// Compares vertices by latitude with a secondary sort on
// longitude
bool sortVert(const std::pair<double, double> &vert1,
				const std::pair<double, double> &vert2) {
	// get lats and lons
	double lat1 = vert1.first;
	double lon1 = vert1.second;
	double lat2 = vert2.first;
	double lon2 = vert2.second;

	if (lat1 < lat2 - 0.1) {
		return (true);
	}

	if (lat1 < lat2 + 0.1) {
		if (lon1 < lon2) {
			return true;
		}
	}

	// lat1 > lat2
	return (false);
}

// NOTE: Web generation needs to be improved. The global grid generation has
// a platform defendant issue (different number of nodes on linux than windows,
// and is inflexible.
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
			int numRows, int numCols, bool update,
			std::shared_ptr<traveltime::CTravelTime> firstTrav,
			std::shared_ptr<traveltime::CTravelTime> secondTrav,
			bool createBackgroundThread, int sleepTime, int checkInterval) {
	// setup threads
	m_bUseBackgroundThread = createBackgroundThread;
	m_iSleepTimeMS = sleepTime;
	m_iStatusCheckInterval = checkInterval;
	std::time(&tLastStatusCheck);

	clear();

	initialize(name, thresh, numDetect, numNucleate, numRows, numCols, update,
				firstTrav, secondTrav);

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

	// stop the thread
	m_bRunJobLoop = false;

	// wait for the thread to finish
	m_BackgroundThread->join();

	// delete it
	delete (m_BackgroundThread);
	m_BackgroundThread = NULL;
}

// ---------------------------------------------------------clear
void CWeb::clear() {
	nRow = 0;
	nCol = 0;
	nDetect = 16;
	nNucleate = 0;
	dThresh = -1.0;
	sName = "Nemo";
	tOrg = 0;
	vDistLo = 0;
	vDistHi = 0;
	pGlass = NULL;
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

	// clear tesselation vectors
	vVert.clear();
	vFace.clear();

	pTrv1 = NULL;
	pTrv2 = NULL;
}

// ---------------------------------------------------------Initialize
bool CWeb::initialize(std::string name, double thresh, int numDetect,
						int numNucleate, int numRows, int numCols, bool update,
						std::shared_ptr<traveltime::CTravelTime> firstTrav,
						std::shared_ptr<traveltime::CTravelTime> secondTrav) {
	sName = name;
	dThresh = thresh;
	nDetect = numDetect;
	nNucleate = numNucleate;
	nRow = numRows;
	nCol = numCols;
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

	// check pGlass
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::global: NULL glass pointer.");
		return (false);
	}

	char sLog[1024];

	// global grid definition variables and defaults
	std::string name = "Nemo";
	int detect = pGlass->nDetect;
	int nucleate = pGlass->nNucleate;
	double thresh = pGlass->dThresh;
	double resol = 0;
	std::vector<double> zzz;
	int nZ = 0;
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
		nZ = static_cast<int>(zzz.size());
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
	initialize(name, thresh, detect, nucleate, 0, 0, update, pTrv1, pTrv2);

	// generate site and network filter lists
	genSiteFilters(com);

	// Generate eligible station list
	genSiteList();

	// create the 6 geographic vertices defining
	// an octahedron corresponds with the globe
	vVert.clear();

	std::pair<double, double> vert1 = { 90.0, 0.0 };
	vVert.push_back(vert1);

	std::pair<double, double> vert2 = { 0.0, 0.0 };
	vVert.push_back(vert2);

	std::pair<double, double> vert3 = { 0.0, 90.0 };
	vVert.push_back(vert3);

	std::pair<double, double> vert4 = { 0.0, 180.0 };
	vVert.push_back(vert4);

	std::pair<double, double> vert5 = { 0.0, 270.0 };
	vVert.push_back(vert5);

	std::pair<double, double> vert6 = { -90.0, 0.0 };
	vVert.push_back(vert6);

	// create the 8 equilateral triangle faces of the
	// initial octahedron
	vFace.clear();
	vFace.push_back(std::make_tuple(0, 1, 2));
	vFace.push_back(std::make_tuple(0, 2, 3));
	vFace.push_back(std::make_tuple(0, 3, 4));
	vFace.push_back(std::make_tuple(0, 4, 1));
	vFace.push_back(std::make_tuple(5, 2, 1));
	vFace.push_back(std::make_tuple(5, 3, 2));
	vFace.push_back(std::make_tuple(5, 4, 3));
	vFace.push_back(std::make_tuple(5, 1, 4));

	// initialize face vector index and size
	int nface = 0;

	// subdivide the 8 equilateral triangle faces of the octahedron to
	// create a vector of equally spaced vertices
	// Perform 7 iterations to get a nominal vertex
	// spacing of roughly 100km.
	// NOTE: Grid density HARDCODED, the provided node resolution
	// has no effect on how dense this grid ends up
	for (int iIterate = 0; iIterate < 7; iIterate++) {
		// initialize distance bounds
		vDistLo = 100000.0;
		vDistHi = -100000.0;

		// setup face vector index and size
		// for this loop iteration
		int iface0 = nface;
		nface = vFace.size();

		// subdivide all the triangles for this loop
		// starting from the last face from the previous loop
		for (int iface = iface0; iface < nface; iface++) {
			subdivide(vFace.at(iface));
		}

		// log the insanity
		snprintf(sLog, sizeof(sLog),
					"CWeb::global: nVert:%d; DistRange:(%.2f %.2f);",
					static_cast<int>(vVert.size()), vDistLo, vDistHi);
		glassutil::CLogit::log(sLog);
	}

	// sort vertices by latitude
	sort(vVert.begin(), vVert.end(), sortVert);

	// Convert the vertices to detection nodes
	// we only need one vertex for each face for a node, the rest are
	// effectively duplicates
	// construct temporary vector for vertices
	std::vector<std::pair<double, double>> temp;

	// init temp vector by copying first vertex from vVert
	vert1 = vVert.at(0);
	temp.push_back(vert1);

	// for every vertex in vVert
	for (int ivert = 1; ivert < vVert.size(); ivert++) {
		// get the next vertex
		vert2 = vVert.at(ivert);

		// if the second vertex is far enough apart in
		// degrees of latitude to represent a unique
		// node, add to temporary vector
		if (fabs(vert2.first - vert1.first) > 0.05) {
			temp.push_back(vert2);
			vert1 = vert2;
		} else {
			// if the second vertex is far enough apart in
			// degrees of longitude to represent a unique
			// node, add to temporary vector add to temporary vector
			if (fabs(vert2.second - vert1.second) > 0.05) {
				temp.push_back(vert2);
				vert1 = vert2;
			}
		}
	}

	// save the reduced vertext vector
	vVert = temp;

	snprintf(sLog, sizeof(sLog), "CWeb::global: Final nVert:%d;",
				static_cast<int>(vVert.size()));
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

	// Generate global grid
	std::shared_ptr<CNode> node;
	int iNodeCount = 0;

	// for each vertex
	for (auto p : vVert) {
		double lat = p.first;
		double lon = p.second;

		std::lock_guard<std::mutex> guard(vSiteMutex);

		// sort site list for this vertex
		sortSiteList(lat, lon);

		// for each depth
		for (auto z : zzz) {
			// create node
			// NOTE: The resolution used to create each node is NOT the actual
			// resolution of the global grid, but the one provided in the
			// configuration. Caryl says that a provided resolution that is
			// significantly larger than the grid resolution will cause the
			// global grid to start missing events
			node = genNode(lat, lon, z, resol);

			// if we got a valid node, add it
			if (addNode(node) == true) {
				iNodeCount++;
			}

			// write node to grid file
			if (saveGrid) {
				outfile << sName << "," << node->sPid << ","
						<< std::to_string(lat) << "," << std::to_string(lon)
						<< "," << std::to_string(z) << "\n";

				// write to station file
				outstafile << node->getSitesString();
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
			sName.c_str(), phases.c_str(), nZ, resol, nDetect, nNucleate,
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

	// check pGlass
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::grid: NULL glass pointer.");
		return (false);
	}

	char sLog[1024];

	// grid definition variables and defaults
	std::string name = "Nemo";
	int detect = pGlass->nDetect;
	int nucleate = pGlass->nNucleate;
	double thresh = pGlass->dThresh;
	double resol = 0;
	double lat = 0;
	double lon = 0;
	std::vector<double> zzz;
	int rows = 0;
	int cols = 0;
	int nZ = 0;
	tOrg = 0.0;
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
		nZ = static_cast<int>(zzz.size());
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

	// An integer containing an origin time used for testing detection grids
	if ((com->HasKey("Org"))
			&& ((*com)["Org"].GetType() == json::ValueType::StringVal)) {
		std::string torg = (*com)["Org"].ToString();

		// convert time
		glassutil::CDate dt(torg);
		tOrg = dt.time();

		snprintf(sLog, sizeof(sLog), "HYP: %s %.2f", torg.c_str(), tOrg);
		glassutil::CLogit::log(sLog);
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
	initialize(name, thresh, detect, nucleate, rows, cols, update, pTrv1,
				pTrv2);

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
				std::shared_ptr<CNode> node = genNode(latrow, loncol, z, resol);

				// if we got a valid node, add it
				if (addNode(node) == true) {
					iNodeCount++;
				}

				// write node to grid file
				if (saveGrid) {
					outfile << sName << "," << node->sPid << ","
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
				lon0 + (nCol - 1) * lonDistance, nRow, nCol, nZ, resol, nDetect,
				nNucleate, dThresh, static_cast<int>(vNetFilter.size()),
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
	// check pGlass
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::grid_explicit: NULL glass pointer.");
		return (false);
	}

	char sLog[1024];

	// grid definition variables and defaults
	std::string name = "Nemo";
	int detect = pGlass->nDetect;
	int nucleate = pGlass->nNucleate;
	double thresh = pGlass->dThresh;
	int nN = 0;
	tOrg = 0.0;
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

	// An integer containing an origin time used for testing detection grids
	if ((com->HasKey("Org"))
			&& ((*com)["Org"].GetType() == json::ValueType::StringVal)) {
		std::string torg = (*com)["Org"].ToString();

		// convert time
		glassutil::CDate dt(torg);
		tOrg = dt.time();

		snprintf(sLog, sizeof(sLog), "HYP: %s %.2f", torg.c_str(), tOrg);
		glassutil::CLogit::log(sLog);
	}

	// initialize
	initialize(name, thresh, detect, nucleate, 0., 0., update, pTrv1, pTrv2);

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

// --------------------------------------------------------Subdivide
void CWeb::subdivide(std::tuple<int, int, int> face) {
	// Given an equilateral triangle defined by 3 faces (edges)
	// Create a point at the midpoint of each of the three faces
	// (edges)
	// Connect the points through the inside of the triangle to
	// create 3 more faces (edges),
	// Use the resulting 6 faces (edges) to creating 4 smaller
	// equilateral triangles contained within the original
	// triangle
	// Store those 4 triangles in vFace
	//
	// In this implementation, each face (edge) is a line
	// defined by a pair of geographic (latitude, longitude)
	// vertices that are indexed by a face (edge) id.
	// The geographic vertices are stored in the vVert vector
	// as std::pairs
	// The equilateral triangle face indexes are stored in
	// the vFace vector as std::tuples

	// get the 3 indexes of the faces (edges) of the existing
	// triangle
	int n0 = std::get<0>(face);
	int n1 = std::get<1>(face);
	int n2 = std::get<2>(face);

	// compute the indexes of the 3 new faces (edges) inside the
	// existing triangle
	int n3 = vVert.size();
	int n4 = n3 + 1;
	int n5 = n4 + 1;

	// generate the midpoints of each face (edge)
	// by using the face indexes to look up the geographic
	// vertices calling Bisect.
	// Add each resulting new geographic vertex to vVert
	vVert.push_back(bisect(vVert.at(n0), vVert.at(n1)));
	vVert.push_back(bisect(vVert.at(n1), vVert.at(n2)));
	vVert.push_back(bisect(vVert.at(n2), vVert.at(n0)));

	// generate the 4 new triangles from the 6 face (edge)
	// indexes.
	// Add each new face (edge) to vFace
	vFace.push_back(std::make_tuple(n0, n3, n5));
	vFace.push_back(std::make_tuple(n3, n1, n4));
	vFace.push_back(std::make_tuple(n5, n4, n2));
	vFace.push_back(std::make_tuple(n3, n4, n5));
}

// ---------------------------------------------------------Bisect
std::pair<double, double> CWeb::bisect(std::pair<double, double> geo1,
										std::pair<double, double> geo2) {
	double xy;
	double dist;

	// compute the (x, y, z) of the first geographic point
	xy = cos(DEG2RAD * geo1.first);
	double x1 = xy * cos(DEG2RAD * geo1.second);
	double y1 = xy * sin(DEG2RAD * geo1.second);
	double z1 = sin(DEG2RAD * geo1.first);

	// compute the (x, y, z) of the second geographic point
	xy = cos(DEG2RAD * geo2.first);
	double x2 = xy * cos(DEG2RAD * geo2.second);
	double y2 = xy * sin(DEG2RAD * geo2.second);
	double z2 = sin(DEG2RAD * geo2.first);

	// compute the midpoint of the face
	double x = 0.5 * (x1 + x2);
	double y = 0.5 * (y1 + y2);
	double z = 0.5 * (z1 + z2);

	// compute the length of the longest diagonal of the face
	double r = sqrt((x * x) + (y * y) + (z * z));

	// divide the midpoint by the longest diagonal of the face
	x /= r;
	y /= r;
	z /= r;

	// compute the distance between the first point
	// and the midpoint, updating the distance bounds	as needed
	dist = 40000.0 * acos(x * x1 + y * y1 + z * z1) / TWOPI;
	if (dist < vDistLo) {
		vDistLo = dist;
	}
	if (dist > vDistHi) {
		vDistHi = dist;
	}

	// compute the distance between the second point
	// and the midpoint, updating the distance bounds	as needed
	dist = 40000.0 * acos(x * x2 + y * y2 + z * z2) / TWOPI;
	if (dist < vDistLo) {
		vDistLo = dist;
	}
	if (dist > vDistHi) {
		vDistHi = dist;
	}

	// convert the midpoint to latitude and longitude
	double lat = RAD2DEG * asin(z);
	double lon = RAD2DEG * atan2(y, x);

	// return the new midpoint
	return {lat, lon};
}

// ---------------------------------------------------------loadTravelTimes
bool CWeb::loadTravelTimes(json::Object *com) {
	// nullchecks
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::loadTravelTimes: NULL pGlass.");
		return (false);
	}

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
		if (pGlass->pTrvDefault != NULL) {
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
		pTrv1->setup(phs, file);

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
		pTrv2->setup(phs, file);
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
			&& (find(vNetFilter.begin(), vNetFilter.end(), site->sNet)
					!= vNetFilter.end())) {
		return (true);
	}

	// if we have a site filter, make sure site is allowed before adding
	if ((vSitesFilter.size() > 0)
			&& (find(vSitesFilter.begin(), vSitesFilter.end(), site->sScnl)
					!= vSitesFilter.end())) {
		return (true);
	}

	return (false);
}

// ---------------------------------------------------------genSiteList
bool CWeb::genSiteList() {
	// nullchecks
	// check pGlass
	if (pGlass == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CWeb::genSiteList: NULL glass pointer.");
		return (false);
	}

	// get the total number sites in glass's site list
	int nsite = pGlass->pSiteList->getSiteCount();

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
		std::shared_ptr<CSite> site = pGlass->pSiteList->getSite(isite);

		if (site->bUse == false) {
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
	node->pWeb = this;

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
		pTrv1->setOrigin(node->dLat, node->dLon, node->dZ);
	}
	if (pTrv2 != NULL) {
		pTrv2->setOrigin(node->dLat, node->dLon, node->dZ);
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
	if (site->bUse == false) {
		remSite(site);
		return;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CWeb::addSite: New potential station " + site->sScnl + " for web: "
					+ sName + ".");

	// don't bother if this site isn't allowed
	if (isSiteAllowed(site) == false) {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::addSite: Station " + site->sScnl + " not allowed in web "
						+ sName + ".");
		return;
	}

	int nodeModCount = 0;
	int nodeCount = 0;
	int totalNodes = vNode.size();

	// for each node in web
	for (auto &node : vNode) {
		nodeCount++;

		if (nodeCount % 1000 == 0) {
			glassutil::CLogit::log(
					glassutil::log_level::debug,
					"CWeb::addSite: Station " + site->sScnl + " processed "
							+ std::to_string(nodeCount) + " out of "
							+ std::to_string(totalNodes) + " nodes in web: "
							+ sName + ". Modified "
							+ std::to_string(nodeModCount) + " nodes.");
		}

		// check to see if we have this site
		std::shared_ptr<CSite> foundSite = node->getSite(site->sScnl);

		// update?
		if (foundSite != NULL) {
			// NOTE: what to do here?! anything? only would
			// matter if the site location changed or (future) quality changed
			continue;
		}

		// set to node geographic location
		// NOTE: node depth is ignored here
		glassutil::CGeo geo;
		geo.setGeographic(node->dLat, node->dLon, 6371.0);

		// compute delta distance between site and node
		double newDistance = RAD2DEG * site->geo.delta(&geo);

		// get site in node list
		// NOTE: this assumes that the node site list is sorted
		// on distance
		std::shared_ptr<CSite> furthestSite = node->getLastSite();

		// compute distance to farthest site
		double maxDistance = RAD2DEG * geo.delta(&furthestSite->geo);

		// Ignore if new site is farther than last linked site
		if (newDistance > maxDistance) {
			node->bEnabled = true;
			continue;
		}

		// setup traveltimes for this node
		if (pTrv1 != NULL) {
			pTrv1->setOrigin(node->dLat, node->dLon, node->dZ);
		}
		if (pTrv2 != NULL) {
			pTrv2->setOrigin(node->dLat, node->dLon, node->dZ);
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

		// update thread status
		setStatus(true);
	}

	// log info if we've added a site
	if (nodeModCount > 0) {
		char sLog[1024];
		snprintf(sLog, sizeof(sLog), "CWeb::addSite: Added site: %s to %d "
					"node(s) in web: %s",
					site->sScnl.c_str(), nodeModCount, sName.c_str());
		glassutil::CLogit::log(glassutil::log_level::info, sLog);
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::addSite: Station " + site->sScnl + " not added to any "
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
				"CWeb::remSite: Station " + site->sScnl + " not allowed in web "
						+ sName + ".");
		return;
	}

	glassutil::CLogit::log(
			glassutil::log_level::debug,
			"CWeb::remSite: Trying to remove station " + site->sScnl
					+ " from web " + sName + ".");

	// init flag to check to see if we've generated a site list for this web
	// yet
	bool bSiteList = false;
	int nodeCount = 0;

	// for each node in web
	for (auto &node : vNode) {
		// search through each site linked to this node, see if we have it
		std::shared_ptr<CSite> foundSite = node->getSite(site->sScnl);

		// don't bother if this node doesn't have this site
		if (foundSite == NULL) {
			continue;
		}

		std::lock_guard<std::mutex> guard(vSiteMutex);

		// generate the site list for this web if this is the first
		// iteration where a node is modified
		if (!bSiteList) {
			// generate the site list
			genSiteList();
			bSiteList = true;

			// make sure we've got enough sites for a node
			if (vSite.size() < nDetect) {
				node->bEnabled = true;
				return;
			}
		}

		// sort overall list of sites for this node
		sortSiteList(node->dLat, node->dLon);

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
						"CWeb::remSite: Failed to add station " + newSite->sScnl
								+ " to web " + sName + ".");
			}

			// resort site links
			node->sortSiteLinks();

			// we've removed a site
			nodeCount++;
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CWeb::remSite: Failed to remove station " + site->sScnl
							+ " from web " + sName + ".");
		}

		// update thread status
		setStatus(true);
	}

	// log info if we've removed a site
	char sLog[1024];
	if (nodeCount > 0) {
		snprintf(
				sLog, sizeof(sLog),
				"CWeb::remSite: Removed site: %s from %d node(s) in web: %s",
				site->sScnl.c_str(), nodeCount, sName.c_str());
		glassutil::CLogit::log(glassutil::log_level::info, sLog);
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::debug,
				"CWeb::remSite: Station " + site->sScnl
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
}  // namespace glasscore
