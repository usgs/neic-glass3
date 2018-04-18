#include <json.h>
#include <cmath>
#include <string>
#include "IGlassSend.h"
#include "Date.h"
#include "Geo.h"
#include "Glass.h"
#include "WebList.h"
#include "SiteList.h"
#include "PickList.h"
#include "HypoList.h"
#include "CorrelationList.h"
#include "Detection.h"
#include "Terra.h"
#include "Ray.h"
#include "GenTrv.h"
#include "Trav.h"
#include "TTT.h"
#include "TravelTime.h"
#include "Logit.h"
#include <memory>

namespace glasscore {

// ---------------------------------------------------------CGlass
CGlass::CGlass() {
	clear();

	pWebList = NULL;
	pSiteList = NULL;
	pPickList = NULL;
	pHypoList = NULL;
	pCorrelationList = NULL;
	pDetection = NULL;
	pTrvDefault = NULL;

	nPickMax = 10000;
	nCorrelationMax = 1000;
	nSitePickMax = 200;
	nHypoMax = 100;
	graphicsOut = false;
	graphicsOutFolder = "./";
	graphicsStepKM = 1.;
	graphicsSteps = 100;
	testTimes = false;
	minimizeTTLocator = false;
	testLocator = false;
}

// ---------------------------------------------------------~CGlass
CGlass::~CGlass() {
	clear();

	if (pWebList) {
		delete (pWebList);
	}
	if (pSiteList) {
		delete (pSiteList);
	}
	if (pPickList) {
		delete (pPickList);
	}
	if (pHypoList) {
		delete (pHypoList);
	}
	if (pCorrelationList) {
		delete (pCorrelationList);
	}
	if (pDetection) {
		delete (pDetection);
	}
}

// ---------------------------------------------------------dispatch
bool CGlass::dispatch(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CGlass::dispatch: NULL json communication.");
		return (false);
	}

	// check for a command
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// log cmd if it isn't one of the common ones
		if ((v != "Site") && (v != "ReqHypo") && (v != "Correlation")
				&& (v != "Pick")) {
			glassutil::CLogit::log(glassutil::log_level::debug,
									"CGlass::dispatch: Cmd:" + v.ToString());
		}

		// generate travel time file
		// NOTE: Move to stand alone program
		// if (v == "GenTrv") {
		// return((genTrv(com));
		// }

		// test travel time classes
		// NOTE: Move to unit tests.
		// if (v == "TestTTT") {
		// testTTT(com);
		// return(true);
		// }

		// Initialize glass
		if (v == "Initialize") {
			return (initialize(com));
		}

		// clear glass configuration
		if (v == "ClearGlass") {
			clear();
			// ClearGlass is also relevant to other glass
			// components, Fall through intentional
		}
		// check to see if glass has been set up
		if (!pWebList) {
			glassutil::CLogit::log(
					glassutil::log_level::error,
					"CGlass::dispatch: ***** Glass Not initialized *****.");

			glassutil::CLogit::log(glassutil::log_level::debug,
									"CGlass::dispatch: Cmd is:" + v.ToString());
			return (false);
		}

		// send any other commands to any of the configurable / input classes
		if (pPickList->dispatch(com)) {
			return (true);
		}
		if (pSiteList->dispatch(com)) {
			return (true);
		}
		if (pHypoList->dispatch(com)) {
			return (true);
		}
		if (pCorrelationList->dispatch(com)) {
			return (true);
		}
		if (pDetection->dispatch(com)) {
			return (true);
		}
		if (pWebList->dispatch(com)) {
			return (true);
		}
	}

	// Input data can have Type keys
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// send any input to any of the input processing classes
		if (pPickList->dispatch(com)) {
			return (true);
		}
		if (pSiteList->dispatch(com)) {
			return (true);
		}
		if (pCorrelationList->dispatch(com)) {
			return (true);
		}
		if (pDetection->dispatch(com)) {
			return (true);
		}
	}

	// this communication was not handled
	return (false);
}

// ---------------------------------------------------------send
bool CGlass::send(std::shared_ptr<json::Object> com) {
	// make sure we have something to send to
	if (piSend) {
		// send the communication
		piSend->Send(com);

		// done
		return (true);
	}

	// communication not sent
	return (false);
}

// ---------------------------------------------------------Clear
void CGlass::clear() {
	// reset to defaults
	nNucleate = 7;
	nDetect = 20;
	dThresh = 2.5;
	sdAssociate = 3.0;
	sdPrune = 3.0;
	expAffinity = 2.5;
	avgDelta = 0.0;
	avgSigma = 2.0;
	dCutFactor = 4.0;
	dCutPercentage = 0.4;
	dCutMin = 30.0;
	iCycleLimit = 25;
	testTimes = false;
	testLocator = false;
	graphicsOut = false;
	graphicsOutFolder = "./";
	graphicsStepKM = 1.;
	graphicsSteps = 100;
	minimizeTTLocator = false;
	pickDuplicateWindow = 2.5;
	correlationMatchingTWindow = 2.5;
	correlationMatchingXWindow = .5;
	correlationCancelAge = 900;
	beamMatchingAzimuthWindow = 22.5;
	beamMatchingDistanceWindow = 5;
	nReportCut = 0;
	dReportThresh = 2.5;
}

// ---------------------------------------------------------Initialize
bool CGlass::initialize(std::shared_ptr<json::Object> com) {
	// null check json
	if (com == NULL) {
		glassutil::CLogit::log(glassutil::log_level::error,
								"CGlass::initialize: NULL json.");
		return (false);
	}

	// check cmd
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		std::string cmd = (*com)["Cmd"].ToString();

		if (cmd != "Initialize") {
			glassutil::CLogit::log(
					glassutil::log_level::warn,
					"CGlass::initialize: Non-Initialize Cmd passed in.");
			return (false);
		}
	} else {
		// no command or type
		glassutil::CLogit::log(
				glassutil::log_level::error,
				"CGlass::initialize: Missing required Cmd Key.");
		return (false);
	}

	// Reset parameters
	clear();

	// set up travel times
	// load the first travel time
	if ((com->HasKey("DefaultNucleationPhase"))
			&& ((*com)["DefaultNucleationPhase"].GetType()
					== json::ValueType::ObjectVal)) {
		// get the phase object
		json::Object phsObj = (*com)["DefaultNucleationPhase"].ToObject();

		// clean out old phase if any
		pTrvDefault.reset();

		// create new traveltime
		pTrvDefault = std::make_shared<traveltime::CTravelTime>();

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
					"CGlass::initialize: Using file location: " + file
							+ " for default nucleation phase: " + phs);
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default file location for "
							" default nucleation phase: " + phs);
		}

		// set up the first phase travel time
		pTrvDefault->setup(phs, file);

	} else {
		// if no first phase, default to P
		// clean out old phase if any
		pTrvDefault.reset();

		// create new travel time
		pTrvDefault = std::make_shared<traveltime::CTravelTime>();

		// set up the first phase travel time
		pTrvDefault->setup("P");

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using  default nucleation first phase P");
	}

	// second, association phases
	if ((com->HasKey("AssociationPhases"))
			&& ((*com)["AssociationPhases"].GetType()
					== json::ValueType::ArrayVal)) {
		// get the array of phase entries
		json::Array phases = (*com)["AssociationPhases"].ToArray();

		// create and initialize travel time list
		pTTT = std::make_shared<traveltime::CTTT>();

		// for each phase in the array
		for (auto val : phases) {
			// make sure the phase is an object
			if (val.GetType() != json::ValueType::ObjectVal) {
				continue;
			}

			// get this phase object
			json::Object obj = val.ToObject();

			double range[4];
			double *rng = NULL;
			double assoc[2];
			double * ass = NULL;
			std::string file = "";

			// get the phase name
			std::string phs = obj["PhaseName"].ToString();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using association phase: " + phs);

			// get the Range if present, otherwise look for an Assoc
			if (obj.HasKey("Range")
					&& (obj["Range"].GetType() == json::ValueType::ArrayVal)) {
				// get the range array
				json::Array arr = obj["Range"].ToArray();

				// make sure the range array has the correct number of entries
				if (arr.size() != 4) {
					continue;
				}

				// copy out the range values
				for (int i = 0; i < 4; i++) {
					range[i] = arr[i].ToDouble();
				}

				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using association Range = ["
								+ std::to_string(range[0]) + ","
								+ std::to_string(range[1]) + ","
								+ std::to_string(range[2]) + ","
								+ std::to_string(range[3]) + "]");
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using association Assoc = ["
								+ std::to_string(assoc[0]) + ","
								+ std::to_string(assoc[1]) + "]");

				// set range pointer
				rng = range;

				// populate assoc from range
				assoc[0] = range[0];
				assoc[1] = range[3];

				// set assoc pointer
				ass = assoc;

			} else if (obj.HasKey("Assoc")
					&& (obj["Assoc"].GetType() == json::ValueType::ArrayVal)) {
				// get the assoc array
				json::Array arr = obj["Assoc"].ToArray();

				// make sure the assoc array has the correct number of entries
				if (arr.size() != 2) {
					continue;
				}

				// copy out the assoc values
				for (int i = 0; i < 2; i++) {
					assoc[i] = arr[i].ToDouble();
				}

				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using association Assoc = ["
								+ std::to_string(assoc[0]) + ","
								+ std::to_string(assoc[1]) + "]");

				// set range pointer
				rng = NULL;

				// set assoc pointer
				ass = assoc;
			} else {
				glassutil::CLogit::log(
						glassutil::log_level::error,
						"CGlass::initialize: Missing required Range or Assoc key.");
				continue;
			}

			// get the travel time file if present
			if (obj.HasKey("TravFile")
					&& (obj["TravFile"].GetType() == json::ValueType::StringVal)) {
				file = obj["TravFile"].ToString();

				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using association tt file: "
								+ file);
			} else {
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Using default file location for association phase: "
								+ phs);
			}

			// set up this phase
			pTTT->addPhase(phs, rng, ass, file);

			// test this phase
			if (testTimes) {
				pTTT->testTravelTimes(phs);
			}
		}
	} else {
		glassutil::CLogit::log(glassutil::log_level::error,
								"No association Phase array provided");
		return (false);
	}

	// setup if we are going to print phase travel times for debuging
	if ((com->HasKey("TestTravelTimes"))
			&& ((*com)["TestTravelTimes"].GetType() == json::ValueType::BoolVal)) {
		testTimes = (*com)["TestTravelTimes"].ToBool();
	}

	// Change locator
	if ((com->HasKey("UseL1ResidualLocator"))
			&& ((*com)["UseL1ResidualLocator"].GetType()
					== json::ValueType::BoolVal)) {
		minimizeTTLocator = (*com)["UseL1ResidualLocator"].ToBool();
	}

	// Collect info for files to plot output
	if ((com->HasKey("PlottingInfo"))
			&& ((*com)["PlottingInfo"].GetType() == json::ValueType::ObjectVal)) {
		json::Object paramsPlot = (*com)["PlottingInfo"].ToObject();

		if ((paramsPlot.HasKey("graphicsOut"))
				&& (paramsPlot["graphicsOut"].GetType()
						== json::ValueType::BoolVal)) {
			graphicsOut = paramsPlot["graphicsOut"].ToBool();
			if (graphicsOut == true) {
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Plotting output is on!!!");
			}

			if (graphicsOut == false) {
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Plotting output is off.");
			}
		}

		if ((paramsPlot.HasKey("graphicsStepKM"))
				&& (paramsPlot["graphicsStepKM"].GetType()
						== json::ValueType::DoubleVal)) {
			graphicsStepKM = paramsPlot["graphicsStepKM"].ToDouble();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Plotting Step Increment: "
							+ std::to_string(graphicsStepKM));
		}

		if ((paramsPlot.HasKey("graphicsSteps"))
				&& (paramsPlot["graphicsSteps"].GetType()
						== json::ValueType::IntVal)) {
			graphicsSteps = paramsPlot["graphicsSteps"].ToInt();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Plotting Steps: "
							+ std::to_string(graphicsSteps));
		}

		if ((paramsPlot.HasKey("graphicsOutFolder"))
				&& (paramsPlot["graphicsOutFolder"].GetType()
						== json::ValueType::StringVal)) {
			graphicsOutFolder = paramsPlot["graphicsOutFolder"].ToString();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Plotting Output Location: "
							+ graphicsOutFolder);
		}
	}

	// Collect association and nucleation tuning parameters
	if ((com->HasKey("Params"))
			&& ((*com)["Params"].GetType() == json::ValueType::ObjectVal)) {
		// get object
		json::Object params = (*com)["Params"].ToObject();

		// Thresh
		if ((params.HasKey("Thresh"))
				&& (params["Thresh"].GetType() == json::ValueType::DoubleVal)) {
			dThresh = params["Thresh"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using Thresh: "
							+ std::to_string(dThresh));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default Thresh: "
							+ std::to_string(dThresh));
		}

		// Nucleate
		if ((params.HasKey("Nucleate"))
				&& (params["Nucleate"].GetType() == json::ValueType::IntVal)) {
			nNucleate = params["Nucleate"].ToInt();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using Nucleate: "
							+ std::to_string(nNucleate));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default Nucleate: "
							+ std::to_string(nNucleate));
		}

		// sdAssociate
		if ((params.HasKey("sdAssociate"))
				&& (params["sdAssociate"].GetType()
						== json::ValueType::DoubleVal)) {
			sdAssociate = params["sdAssociate"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using sdAssociate: "
							+ std::to_string(sdAssociate));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default sdAssociate: "
							+ std::to_string(sdAssociate));
		}

		// sdPrune
		if ((params.HasKey("sdPrune"))
				&& (params["sdPrune"].GetType() == json::ValueType::DoubleVal)) {
			sdPrune = params["sdPrune"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using sdPrune: "
							+ std::to_string(sdPrune));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default sdPrune: "
							+ std::to_string(sdPrune));
		}

		// ExpAffinity
		if ((params.HasKey("expAffinity"))
				&& (params["expAffinity"].GetType()
						== json::ValueType::DoubleVal)) {
			expAffinity = params["expAffinity"].ToDouble();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using expAffinity: "
							+ std::to_string(expAffinity));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default expAffinity: "
							+ std::to_string(expAffinity));
		}

		// avgDelta
		if ((params.HasKey("avgDelta"))
				&& (params["avgDelta"].GetType() == json::ValueType::DoubleVal)) {
			avgDelta = params["avgDelta"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using avgDelta: "
							+ std::to_string(avgDelta));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default avgDelta: "
							+ std::to_string(avgDelta));
		}

		// avgSigma
		if ((params.HasKey("avgSigma"))
				&& (params["avgSigma"].GetType() == json::ValueType::DoubleVal)) {
			avgSigma = params["avgSigma"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using avgSigma: "
							+ std::to_string(avgSigma));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default avgSigma: "
							+ std::to_string(avgSigma));
		}

		// dCutFactor
		if ((params.HasKey("dCutFactor"))
				&& (params["dCutFactor"].GetType() == json::ValueType::DoubleVal)) {
			dCutFactor = params["dCutFactor"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using dCutFactor: "
							+ std::to_string(dCutFactor));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default dCutFactor: "
							+ std::to_string(dCutFactor));
		}

		// dCutPercentage
		if ((params.HasKey("dCutPercentage"))
				&& (params["dCutPercentage"].GetType()
						== json::ValueType::DoubleVal)) {
			dCutPercentage = params["dCutPercentage"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using dCutPercentage: "
							+ std::to_string(dCutPercentage));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default dCutPercentage: "
							+ std::to_string(dCutPercentage));
		}

		// dCutMin
		if ((params.HasKey("dCutMin"))
				&& (params["dCutMin"].GetType() == json::ValueType::DoubleVal)) {
			dCutMin = params["dCutMin"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using dCutMin: "
							+ std::to_string(dCutMin));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default dCutMin: "
							+ std::to_string(dCutMin));
		}

		// iCycleLimit
		if ((params.HasKey("iCycleLimit"))
				&& (params["iCycleLimit"].GetType() == json::ValueType::IntVal)) {
			iCycleLimit = params["iCycleLimit"].ToInt();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using iCycleLimit: "
							+ std::to_string(iCycleLimit));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default iCycleLimit: "
							+ std::to_string(iCycleLimit));
		}

		// correlationMatchingTWindow
		if ((params.HasKey("CorrelationTimeWindow"))
				&& (params["CorrelationTimeWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			correlationMatchingTWindow = params["CorrelationTimeWindow"]
					.ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using CorrelationTimeWindow: "
							+ std::to_string(correlationMatchingTWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default "
							"CorrelationTimeWindow: "
							+ std::to_string(correlationMatchingTWindow));
		}

		// correlationMatchingXWindow
		if ((params.HasKey("CorrelationDistanceWindow"))
				&& (params["CorrelationDistanceWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			correlationMatchingXWindow = params["CorrelationDistanceWindow"]
					.ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using CorrelationDistanceWindow: "
							+ std::to_string(correlationMatchingXWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default "
							"CorrelationDistanceWindow: "
							+ std::to_string(correlationMatchingXWindow));
		}

		// correlationCancelAge
		if ((params.HasKey("CorrelationCancelAge"))
				&& (params["CorrelationCancelAge"].GetType()
						== json::ValueType::DoubleVal)) {
			correlationCancelAge = params["CorrelationCancelAge"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using CorrelationCancelAge: "
							+ std::to_string(correlationCancelAge));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default CorrelationCancelAge: "
							+ std::to_string(correlationCancelAge));
		}

		// beamMatchingAzimuthWindow
		if ((params.HasKey("BeamMatchingAzimuthWindow"))
				&& (params["BeamMatchingAzimuthWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			beamMatchingAzimuthWindow = params["BeamMatchingAzimuthWindow"]
					.ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using BeamMatchingAzimuthWindow: "
							+ std::to_string(beamMatchingAzimuthWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default "
							"BeamMatchingAzimuthWindow: "
							+ std::to_string(beamMatchingAzimuthWindow));
		}

		/*
		 // beamMatchingDistanceWindow
		 if ((params.HasKey("dBeamMatchingDistanceWindow"))
		 && (params["dBeamMatchingDistanceWindow"].GetType()
		 == json::ValueType::DoubleVal)) {
		 beamMatchingDistanceWindow = params["dBeamMatchingDistanceWindow"]
		 .ToDouble();

		 glassutil::CLogit::log(
		 glassutil::log_level::info,
		 "CGlass::initialize: Using dBeamMatchingDistanceWindow: "
		 + std::to_string(beamMatchingDistanceWindow));
		 } else {
		 glassutil::CLogit::log(
		 glassutil::log_level::info,
		 "CGlass::initialize: Using default "
		 "dBeamMatchingDistanceWindow: "
		 + std::to_string(beamMatchingDistanceWindow));
		 }
		 */

		// dReportThresh
		if ((params.HasKey("ReportThresh"))
				&& (params["ReportThresh"].GetType()
						== json::ValueType::DoubleVal)) {
			dReportThresh = params["ReportThresh"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using ReportThresh: "
							+ std::to_string(dReportThresh));
		} else {
			// default to overall thresh
			dReportThresh = dThresh;
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default ReportThresh (=Thresh): "
							+ std::to_string(dReportThresh));
		}

		// nReportCut
		if ((params.HasKey("ReportCut"))
				&& (params["ReportCut"].GetType() == json::ValueType::IntVal)) {
			nReportCut = params["ReportCut"].ToInt();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using ReportCut: "
							+ std::to_string(nReportCut));
		} else {
			// default to overall nNucleate
			nReportCut = 0;
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default ReportCut: "
							+ std::to_string(nReportCut));
		}
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default association and nucleation "
				"parameters");
	}

	// Test Locator
	if ((com->HasKey("TestLocator"))
			&& ((*com)["TestLocator"].GetType() == json::ValueType::BoolVal)) {
		testLocator = (*com)["TestLocator"].ToBool();

		if (testLocator) {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: testLocator set to true");
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: testLocator set to false");
		}
	} else {
		glassutil::CLogit::log(glassutil::log_level::info,
								"CGlass::initialize: testLocator not Found!");
	}

	// set maximum number of picks
	if ((com->HasKey("PickMax"))
			&& ((*com)["PickMax"].GetType() == json::ValueType::IntVal)) {
		nPickMax = (*com)["PickMax"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using PickMax: "
						+ std::to_string(nPickMax));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default PickMax: "
						+ std::to_string(nPickMax));
	}

	// set maximum number of pick in a single site
	if ((com->HasKey("SitePickMax"))
			&& ((*com)["SitePickMax"].GetType() == json::ValueType::IntVal)) {
		nSitePickMax = (*com)["SitePickMax"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using SitePickMax: "
						+ std::to_string(nSitePickMax));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default SitePickMax: "
						+ std::to_string(nSitePickMax));
	}

	// set maximum number of correlations
	if ((com->HasKey("CorrelationMax"))
			&& ((*com)["CorrelationMax"].GetType() == json::ValueType::IntVal)) {
		nCorrelationMax = (*com)["CorrelationMax"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using CorrelationMax: "
						+ std::to_string(nCorrelationMax));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default CorrelationMax: "
						+ std::to_string(nCorrelationMax));
	}

	// set pick duplicate window
	if ((com->HasKey("PickDuplicateWindow"))
			&& ((*com)["PickDuplicateWindow"].GetType()
					== json::ValueType::DoubleVal)) {
		pickDuplicateWindow = (*com)["PickDuplicateWindow"].ToDouble();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using PickDuplicateWindow: "
						+ std::to_string(pickDuplicateWindow));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default PickDuplicateWindow: "
						+ std::to_string(pickDuplicateWindow));
	}

	// set maximum number of hypos
	if ((com->HasKey("HypoMax"))
			&& ((*com)["HypoMax"].GetType() == json::ValueType::IntVal)) {
		nHypoMax = (*com)["HypoMax"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using HypoMax: "
						+ std::to_string(nHypoMax));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default HypoMax: "
						+ std::to_string(nHypoMax));
	}

	// set the number of nucleation threads
	int numNucleationThreads = 5;
	if ((com->HasKey("NumNucleationThreads"))
			&& ((*com)["NumNucleationThreads"].GetType()
					== json::ValueType::IntVal)) {
		numNucleationThreads = (*com)["NumNucleationThreads"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using NumNucleationThreads: "
						+ std::to_string(numNucleationThreads));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default NumNucleationThreads: "
						+ std::to_string(numNucleationThreads));
	}

	// set the number of hypo threads
	int numHypoThreads = 3;
	if ((com->HasKey("NumHypoThreads"))
			&& ((*com)["NumHypoThreads"].GetType() == json::ValueType::IntVal)) {
		numHypoThreads = (*com)["NumHypoThreads"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using NumHypoThreads: "
						+ std::to_string(numHypoThreads));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default NumHypoThreads: "
						+ std::to_string(numHypoThreads));
	}

	// set the number of web threads
	int numWebThreads = 0;
	if ((com->HasKey("NumWebThreads"))
			&& ((*com)["NumWebThreads"].GetType()
					== json::ValueType::IntVal)) {
		numWebThreads = (*com)["NumWebThreads"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using NumWebThreads: "
						+ std::to_string(numWebThreads));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default NumWebThreads: "
						+ std::to_string(numWebThreads));
	}

	int iHoursWithoutPicking = -1;
	if ((com->HasKey("SiteHoursWithoutPicking"))
			&& ((*com)["SiteHoursWithoutPicking"].GetType()
					== json::ValueType::IntVal)) {
		iHoursWithoutPicking = (*com)["SiteHoursWithoutPicking"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using SiteHoursWithoutPicking: "
						+ std::to_string(iHoursWithoutPicking));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default SiteHoursWithoutPicking: "
						+ std::to_string(iHoursWithoutPicking));
	}

	int iHoursBeforeLookingUp = -1;
	if ((com->HasKey("SiteLookupInterval"))
			&& ((*com)["SiteLookupInterval"].GetType()
					== json::ValueType::IntVal)) {
		iHoursBeforeLookingUp = (*com)["SiteLookupInterval"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using SiteLookupInterval: "
						+ std::to_string(iHoursBeforeLookingUp));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default SiteLookupInterval: "
						+ std::to_string(iHoursBeforeLookingUp));
	}

	// test sig and gaus
	// NOTE: Keep for unit test reference
	// for (double sg = 0.0; sg < 5.0; sg += 0.1) {
	// printf("%.2f %.4f %.4f\n", sg, Gaus(sg, 1.0), Sig(sg, 1.0));
	// }

	// clean out old site list if any
	if (pSiteList) {
		delete (pSiteList);
	}

	// create site list
	pSiteList = new CSiteList();
	pSiteList->setGlass(this);
	pSiteList->setHoursWithoutPicking(iHoursWithoutPicking);
	pSiteList->setHoursBeforeLookingUp(iHoursBeforeLookingUp);

	// clean out old web list if any
	if (pWebList) {
		delete (pWebList);
	}

	// create detection web list
	pWebList = new CWebList(numWebThreads);
	pWebList->setGlass(this);
	pWebList->setSiteList(pSiteList);

	// clean out old pick list if any
	if (pPickList) {
		delete (pPickList);
	}

	// create pick list
	pPickList = new CPickList(numNucleationThreads);
	pPickList->setGlass(this);
	pPickList->setSiteList(pSiteList);

	// clean out old correlation list if any
	if (pCorrelationList) {
		delete (pCorrelationList);
	}

	// create correlation list
	pCorrelationList = new CCorrelationList();
	pCorrelationList->setGlass(this);
	pCorrelationList->setSiteList(pSiteList);

	// clean out old hypo list if any
	if (pHypoList) {
		delete (pHypoList);
	}

	// create hypo list
	pHypoList = new CHypoList(numHypoThreads);
	pHypoList->setGlass(this);

	// create detection processor
	pDetection = new CDetection();
	pDetection->setGlass(this);

	return (true);
}

/* NOTE: Leave these in place as examples for Travel Time unit tests.
 *
 // ---------------------------------------------------------Test
 // General testing wart
 bool CGlass::test(json::Object *com) {
 CTerra *terra = new CTerra();
 CRay *ray = new CRay();
 double deg2rad = 0.01745329251994;
 double rad2deg = 57.29577951308;
 ray->pTerra = terra;
 string mdl = (*com)["Model"].ToString();
 printf("model:%s\n", mdl.c_str());

 bool bload = terra->load(mdl.c_str());
 if (!bload)
 return(false;
 printf("Terra nLayer:%ld dRadius:%.2f\n", terra->nLayer, terra->dRadius);
 double r = terra->dRadius;

 int iphs = ray->setPhase("P");
 printf("ray->setPhase:%d\n", iphs);
 ray->setDepth(100.0);
 ray->Setup();
 double t1;
 double t2;
 double dtdz;
 double rad;
 for (double deg = 0.0; deg < 30.0; deg += 1.0) {
 rad = deg2rad * deg;
 t1 = ray->Travel(rad, r);
 t2 = ray->Travel(rad, r + 1.0);
 printf("T %6.2f %6.2f %6.2f %6.2f\n", deg, t1, t2, t2 - t1);
 }

 return(true;
 }

 // ---------------------------------------------------------GenTrv
 // Generate travel time interpolation file
 bool CGlass::genTrv(json::Object *com) {
 CGenTrv *gentrv;

 gentrv = new CGenTrv();
 bool bres = gentrv->Generate(com);
 delete gentrv;
 return(bres;
 }

 // ---------------------------------------------------------TestTTT
 // Quick and dirty unit test for TTT functions
 void CGlass::testTTT(json::Object *com) {
 pTerra = new CTerra();
 pRay = new CRay();
 double deg2rad = 0.01745329251994;
 double rad2deg = 57.29577951308;
 pRay->pTerra = pTerra;
 string mdl = (*com)["Model"].ToString();
 printf("model:%s\n", mdl.c_str());
 bool bload = pTerra->load(mdl.c_str());
 if (!bload)
 printf(" ** ERR:Cannot load Earth model\n");
 printf("Terra nLayer:%ld dRadius:%.2f\n", pTerra->nLayer, pTerra->dRadius);
 CTravelTime *trv = new CTravelTime();
 trv->Setup(pRay, "PKPdf");
 trv->setOrigin(0.0, 0.0, 50.0);
 double t = trv->T(50.0);
 printf("T(50.0,50.0) is %.2f\n", t);
 int mn;
 double sc;
 trv->setOrigin(0.0, 0.0, 0.0);
 for (double d = 0.0; d < 360.5; d += 1.0) {
 t = trv->T(d);
 mn = (int) (t / 60.0);
 sc = t - 60.0 * mn;
 printf("%6.1f %3d %5.2f\n", d, mn, sc);
 }
 CTTT *ttt = new CTTT();
 ttt->Setup(pRay);
 ttt->addPhase("P");
 ttt->addPhase("S");
 ttt->setOrigin(0.0, 0.0, 0.0);
 for (double d = 0.0; d < 360.5; d += 1.0) {
 t = ttt->T(d, "P");
 mn = (int) (t / 60.0);
 sc = t - 60.0 * mn;
 printf("%6.1f %3d %5.2f\n", d, mn, sc);
 }
 }*/

// ---------------------------------------------------------Sig
// Calculate the significance function, which is just
// the bell shaped curve with Sig(0, x) pinned to 1.
// It is used for pruning and association, and is roughly
// analogous to residual pruning in least squares approaches
double CGlass::sig(double x, double sigma) {
	return (exp(-0.5 * x * x / sigma / sigma));
}
// ---------------------------------------------------------Sig
// Calculate the laplacian significance function, which is just
// It is used for pruning and association, and is roughly
// analogous to residual pruning in L1 approach.
double CGlass::sig_laplace_pdf(double x, double sigma) {
	if (x > 0) {
		return ((1. / (2. * sigma)) * exp(-1. * x / sigma));
	} else {
		return ((1. / (2. * sigma)) * exp(x / sigma));
	}
}

// ---------------------------------------------------------statusCheck
bool CGlass::statusCheck() {
	// nullcheck
	if (pPickList == NULL) {
		return (false);
	}

	// check pick list
	if (pPickList->statusCheck() == false) {
		return (false);
	}

	// hypo list
	if (pHypoList->statusCheck() == false) {
		return (false);
	}

	// site list
	if (pSiteList->statusCheck() == false) {
		return (false);
	}

	// webs
	if (pWebList->statusCheck() == false) {
		return (false);
	}

	// all is well
	return (true);
}

double CGlass::getAvgDelta() const {
	return (avgDelta);
}

double CGlass::getAvgSigma() const {
	return (avgSigma);
}

double CGlass::getBeamMatchingAzimuthWindow() const {
	return (beamMatchingAzimuthWindow);
}

double CGlass::getBeamMatchingDistanceWindow() const {
	return (beamMatchingDistanceWindow);
}

int CGlass::getCorrelationCancelAge() const {
	return (correlationCancelAge);
}

double CGlass::getCorrelationMatchingTWindow() const {
	return (correlationMatchingTWindow);
}

double CGlass::getCorrelationMatchingXWindow() const {
	return (correlationMatchingXWindow);
}

double CGlass::getCutFactor() const {
	return (dCutFactor);
}

double CGlass::getCutMin() const {
	return (dCutMin);
}

double CGlass::getCutPercentage() const {
	return (dCutPercentage);
}

double CGlass::getReportThresh() const {
	return (dReportThresh);
}

double CGlass::getThresh() const {
	return (dThresh);
}

double CGlass::getExpAffinity() const {
	return (expAffinity);
}

bool CGlass::getGraphicsOut() const {
	return (graphicsOut);
}

const std::string& CGlass::getGraphicsOutFolder() const {
	return (graphicsOutFolder);
}

double CGlass::getGraphicsStepKm() const {
	return (graphicsStepKM);
}

int CGlass::getGraphicsSteps() const {
	return (graphicsSteps);
}

int CGlass::getCycleLimit() const {
	return (iCycleLimit);
}

bool CGlass::getMinimizeTtLocator() const {
	return (minimizeTTLocator);
}

int CGlass::getCorrelationMax() const {
	return (nCorrelationMax);
}

int CGlass::getDetect() const {
	return (nDetect);
}

int CGlass::getHypoMax() const {
	return (nHypoMax);
}

int CGlass::getNucleate() const {
	return (nNucleate);
}

int CGlass::getPickMax() const {
	return (nPickMax);
}

double CGlass::getReportCut() const {
	return (nReportCut);
}

int CGlass::getSitePickMax() const {
	return (nSitePickMax);
}

CCorrelationList*& CGlass::getCorrelationList() {
	return (pCorrelationList);
}

CDetection*& CGlass::getDetection() {
	return (pDetection);
}

CHypoList*& CGlass::getHypoList() {
	return (pHypoList);
}

double CGlass::getPickDuplicateWindow() const {
	return (pickDuplicateWindow);
}

CPickList*& CGlass::getPickList() {
	return (pPickList);
}

CSiteList*& CGlass::getSiteList() {
	return (pSiteList);
}

std::shared_ptr<traveltime::CTravelTime>& CGlass::getTrvDefault() {
	std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
	return (pTrvDefault);
}

std::shared_ptr<traveltime::CTTT>& CGlass::getTTT() {
	std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
	return (pTTT);
}

CWebList*& CGlass::getWebList() {
	return (pWebList);
}

double CGlass::getSdAssociate() const {
	return (sdAssociate);
}

double CGlass::getSdPrune() const {
	return (sdPrune);
}

bool CGlass::getTestLocator() const {
	return (testLocator);
}

bool CGlass::getTestTimes() const {
	return (testTimes);
}
}  // namespace glasscore
