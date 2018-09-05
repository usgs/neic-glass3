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

	m_pWebList = NULL;
	m_pSiteList = NULL;
	m_pPickList = NULL;
	m_pHypoList = NULL;
	m_pCorrelationList = NULL;
	m_pDetectionProcessor = NULL;
	m_pDefaultNucleationTravelTime = NULL;

	m_iMaxNumPicks = 10000;
	m_iMaxNumCorrelations = 1000;
	m_iMaxNumPicksPerSite = 200;
	m_iMaxNumHypos = 100;
	m_bGraphicsOut = false;
	m_sGraphicsOutFolder = "./";
	m_dGraphicsStepKM = 1.;
	m_iGraphicsSteps = 100;
	m_bTestTravelTimes = false;
	m_bMinimizeTTLocator = false;
	m_bTestLocator = false;
}

// ---------------------------------------------------------~CGlass
CGlass::~CGlass() {
	clear();

	if (m_pWebList) {
		delete (m_pWebList);
	}
	if (m_pSiteList) {
		delete (m_pSiteList);
	}
	if (m_pPickList) {
		delete (m_pPickList);
	}
	if (m_pHypoList) {
		delete (m_pHypoList);
	}
	if (m_pCorrelationList) {
		delete (m_pCorrelationList);
	}
	if (m_pDetectionProcessor) {
		delete (m_pDetectionProcessor);
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

	// check for a command, usually configuration or a request
	if (com->HasKey("Cmd")
			&& ((*com)["Cmd"].GetType() == json::ValueType::StringVal)) {
		// dispatch to appropriate function based on Cmd value
		json::Value v = (*com)["Cmd"].ToString();

		// Initialize glass
		if (v == "Initialize") {
			return (initialize(com));
		}

		// send any other commands to any of the configurable / input classes
		if (m_pSiteList->dispatch(com)) {
			return (true);
		}
		if (m_pHypoList->dispatch(com)) {
			return (true);
		}
		if (m_pWebList->dispatch(com)) {
			return (true);
		}
	}

	// Check for a type, usually input data
	if (com->HasKey("Type")
			&& ((*com)["Type"].GetType() == json::ValueType::StringVal)) {
		// send any input to any of the input processing classes
		if (m_pPickList->dispatch(com)) {
			return (true);
		}
		if (m_pSiteList->dispatch(com)) {
			return (true);
		}
		if (m_pCorrelationList->dispatch(com)) {
			return (true);
		}
		if (m_pDetectionProcessor->dispatch(com)) {
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
	m_iNucleationDataThreshold = 7;
	m_iNumStationsPerNode = 20;
	m_dNucleationStackThreshold = 2.5;
	m_dAssociationSDCutoff = 3.0;
	m_dPruningSDCutoff = 3.0;
	m_dPickAffinityExpFactor = 2.5;
	m_dDistanceCutoffFactor = 4.0;
	m_dDistanceCutoffPercentage = 0.4;
	m_dMinDistanceCutoff = 30.0;
	m_iProcessLimit = 25;
	m_bTestTravelTimes = false;
	m_bTestLocator = false;
	m_bGraphicsOut = false;
	m_sGraphicsOutFolder = "./";
	m_dGraphicsStepKM = 1.;
	m_iGraphicsSteps = 100;
	m_bMinimizeTTLocator = false;
	m_dPickDuplicateTimeWindow = 2.5;
	m_dCorrelationMatchingTimeWindow = 2.5;
	m_dCorrelationMatchingDistanceWindow = .5;
	m_iCorrelationCancelAge = 900;
	m_dBeamMatchingAzimuthWindow = 22.5;
	m_dBeamMatchingDistanceWindow = 5;
	m_iReportingDataThreshold = 0;
	m_dReportingStackThreshold = 2.5;
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
		m_pDefaultNucleationTravelTime.reset();

		// create new traveltime
		m_pDefaultNucleationTravelTime = std::make_shared<
				traveltime::CTravelTime>();

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
		m_pDefaultNucleationTravelTime->setup(phs, file);

	} else {
		// if no first phase, default to P
		// clean out old phase if any
		m_pDefaultNucleationTravelTime.reset();

		// create new travel time
		m_pDefaultNucleationTravelTime = std::make_shared<
				traveltime::CTravelTime>();

		// set up the first phase travel time
		m_pDefaultNucleationTravelTime->setup("P");

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
		m_pAssociationTravelTimes = std::make_shared<traveltime::CTTT>();

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
			m_pAssociationTravelTimes->addPhase(phs, rng, ass, file);

			// test this phase
			if (m_bTestTravelTimes) {
				m_pAssociationTravelTimes->testTravelTimes(phs);
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
		m_bTestTravelTimes = (*com)["TestTravelTimes"].ToBool();
	}

	// Change locator
	if ((com->HasKey("UseL1ResidualLocator"))
			&& ((*com)["UseL1ResidualLocator"].GetType()
					== json::ValueType::BoolVal)) {
		m_bMinimizeTTLocator = (*com)["UseL1ResidualLocator"].ToBool();
	}

	// Collect info for files to plot output
	if ((com->HasKey("PlottingInfo"))
			&& ((*com)["PlottingInfo"].GetType() == json::ValueType::ObjectVal)) {
		json::Object paramsPlot = (*com)["PlottingInfo"].ToObject();

		if ((paramsPlot.HasKey("graphicsOut"))
				&& (paramsPlot["graphicsOut"].GetType()
						== json::ValueType::BoolVal)) {
			m_bGraphicsOut = paramsPlot["graphicsOut"].ToBool();
			if (m_bGraphicsOut == true) {
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Plotting output is on!!!");
			}

			if (m_bGraphicsOut == false) {
				glassutil::CLogit::log(
						glassutil::log_level::info,
						"CGlass::initialize: Plotting output is off.");
			}
		}

		if ((paramsPlot.HasKey("graphicsStepKM"))
				&& (paramsPlot["graphicsStepKM"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dGraphicsStepKM = paramsPlot["graphicsStepKM"].ToDouble();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Plotting Step Increment: "
							+ std::to_string(m_dGraphicsStepKM));
		}

		if ((paramsPlot.HasKey("graphicsSteps"))
				&& (paramsPlot["graphicsSteps"].GetType()
						== json::ValueType::IntVal)) {
			m_iGraphicsSteps = paramsPlot["graphicsSteps"].ToInt();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Plotting Steps: "
							+ std::to_string(m_iGraphicsSteps));
		}

		if ((paramsPlot.HasKey("graphicsOutFolder"))
				&& (paramsPlot["graphicsOutFolder"].GetType()
						== json::ValueType::StringVal)) {
			m_sGraphicsOutFolder = paramsPlot["graphicsOutFolder"].ToString();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Plotting Output Location: "
							+ m_sGraphicsOutFolder);
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
			m_dNucleationStackThreshold = params["Thresh"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using Thresh: "
							+ std::to_string(m_dNucleationStackThreshold));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default Thresh: "
							+ std::to_string(m_dNucleationStackThreshold));
		}

		// Nucleate
		if ((params.HasKey("Nucleate"))
				&& (params["Nucleate"].GetType() == json::ValueType::IntVal)) {
			m_iNucleationDataThreshold = params["Nucleate"].ToInt();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using Nucleate: "
							+ std::to_string(m_iNucleationDataThreshold));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default Nucleate: "
							+ std::to_string(m_iNucleationDataThreshold));
		}

		// sdAssociate
		if ((params.HasKey("sdAssociate"))
				&& (params["sdAssociate"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dAssociationSDCutoff = params["sdAssociate"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using sdAssociate: "
							+ std::to_string(m_dAssociationSDCutoff));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default sdAssociate: "
							+ std::to_string(m_dAssociationSDCutoff));
		}

		// sdPrune
		if ((params.HasKey("sdPrune"))
				&& (params["sdPrune"].GetType() == json::ValueType::DoubleVal)) {
			m_dPruningSDCutoff = params["sdPrune"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using sdPrune: "
							+ std::to_string(m_dPruningSDCutoff));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default sdPrune: "
							+ std::to_string(m_dPruningSDCutoff));
		}

		// ExpAffinity
		if ((params.HasKey("expAffinity"))
				&& (params["expAffinity"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dPickAffinityExpFactor = params["expAffinity"].ToDouble();
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using expAffinity: "
							+ std::to_string(m_dPickAffinityExpFactor));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default expAffinity: "
							+ std::to_string(m_dPickAffinityExpFactor));
		}

		// dCutFactor
		if ((params.HasKey("dCutFactor"))
				&& (params["dCutFactor"].GetType() == json::ValueType::DoubleVal)) {
			m_dDistanceCutoffFactor = params["dCutFactor"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using dCutFactor: "
							+ std::to_string(m_dDistanceCutoffFactor));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default dCutFactor: "
							+ std::to_string(m_dDistanceCutoffFactor));
		}

		// dCutPercentage
		if ((params.HasKey("dCutPercentage"))
				&& (params["dCutPercentage"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dDistanceCutoffPercentage = params["dCutPercentage"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using dCutPercentage: "
							+ std::to_string(m_dDistanceCutoffPercentage));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default dCutPercentage: "
							+ std::to_string(m_dDistanceCutoffPercentage));
		}

		// dCutMin
		if ((params.HasKey("dCutMin"))
				&& (params["dCutMin"].GetType() == json::ValueType::DoubleVal)) {
			m_dMinDistanceCutoff = params["dCutMin"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using dCutMin: "
							+ std::to_string(m_dMinDistanceCutoff));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default dCutMin: "
							+ std::to_string(m_dMinDistanceCutoff));
		}

		// iCycleLimit
		if ((params.HasKey("iCycleLimit"))
				&& (params["iCycleLimit"].GetType() == json::ValueType::IntVal)) {
			m_iProcessLimit = params["iCycleLimit"].ToInt();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using iCycleLimit: "
							+ std::to_string(m_iProcessLimit));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default iCycleLimit: "
							+ std::to_string(m_iProcessLimit));
		}

		// correlationMatchingTWindow
		if ((params.HasKey("CorrelationTimeWindow"))
				&& (params["CorrelationTimeWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dCorrelationMatchingTimeWindow = params["CorrelationTimeWindow"]
					.ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using CorrelationTimeWindow: "
							+ std::to_string(m_dCorrelationMatchingTimeWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default "
							"CorrelationTimeWindow: "
							+ std::to_string(m_dCorrelationMatchingTimeWindow));
		}

		// correlationMatchingXWindow
		if ((params.HasKey("CorrelationDistanceWindow"))
				&& (params["CorrelationDistanceWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dCorrelationMatchingDistanceWindow =
					params["CorrelationDistanceWindow"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using CorrelationDistanceWindow: "
							+ std::to_string(
									m_dCorrelationMatchingDistanceWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default "
							"CorrelationDistanceWindow: "
							+ std::to_string(
									m_dCorrelationMatchingDistanceWindow));
		}

		// correlationCancelAge
		if ((params.HasKey("CorrelationCancelAge"))
				&& (params["CorrelationCancelAge"].GetType()
						== json::ValueType::DoubleVal)) {
			m_iCorrelationCancelAge = params["CorrelationCancelAge"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using CorrelationCancelAge: "
							+ std::to_string(m_iCorrelationCancelAge));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default CorrelationCancelAge: "
							+ std::to_string(m_iCorrelationCancelAge));
		}

		// beamMatchingAzimuthWindow
		if ((params.HasKey("BeamMatchingAzimuthWindow"))
				&& (params["BeamMatchingAzimuthWindow"].GetType()
						== json::ValueType::DoubleVal)) {
			m_dBeamMatchingAzimuthWindow = params["BeamMatchingAzimuthWindow"]
					.ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using BeamMatchingAzimuthWindow: "
							+ std::to_string(m_dBeamMatchingAzimuthWindow));
		} else {
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default "
							"BeamMatchingAzimuthWindow: "
							+ std::to_string(m_dBeamMatchingAzimuthWindow));
		}

		/*
		 * NOTE: Keep for when beams are fully implemented
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
			m_dReportingStackThreshold = params["ReportThresh"].ToDouble();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using ReportThresh: "
							+ std::to_string(m_dReportingStackThreshold));
		} else {
			// default to overall thresh
			m_dReportingStackThreshold = m_dNucleationStackThreshold;
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default ReportThresh (=Thresh): "
							+ std::to_string(m_dReportingStackThreshold));
		}

		// nReportCut
		if ((params.HasKey("ReportCut"))
				&& (params["ReportCut"].GetType() == json::ValueType::IntVal)) {
			m_iReportingDataThreshold = params["ReportCut"].ToInt();

			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using ReportCut: "
							+ std::to_string(m_iReportingDataThreshold));
		} else {
			// default to overall nNucleate
			m_iReportingDataThreshold = 0;
			glassutil::CLogit::log(
					glassutil::log_level::info,
					"CGlass::initialize: Using default ReportCut: "
							+ std::to_string(m_iReportingDataThreshold));
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
		m_bTestLocator = (*com)["TestLocator"].ToBool();

		if (m_bTestLocator) {
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
		m_iMaxNumPicks = (*com)["PickMax"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using PickMax: "
						+ std::to_string(m_iMaxNumPicks));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default PickMax: "
						+ std::to_string(m_iMaxNumPicks));
	}

	// set maximum number of pick in a single site
	if ((com->HasKey("SitePickMax"))
			&& ((*com)["SitePickMax"].GetType() == json::ValueType::IntVal)) {
		m_iMaxNumPicksPerSite = (*com)["SitePickMax"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using SitePickMax: "
						+ std::to_string(m_iMaxNumPicksPerSite));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default SitePickMax: "
						+ std::to_string(m_iMaxNumPicksPerSite));
	}

	// set maximum number of correlations
	if ((com->HasKey("CorrelationMax"))
			&& ((*com)["CorrelationMax"].GetType() == json::ValueType::IntVal)) {
		m_iMaxNumCorrelations = (*com)["CorrelationMax"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using CorrelationMax: "
						+ std::to_string(m_iMaxNumCorrelations));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default CorrelationMax: "
						+ std::to_string(m_iMaxNumCorrelations));
	}

	// set pick duplicate window
	if ((com->HasKey("PickDuplicateWindow"))
			&& ((*com)["PickDuplicateWindow"].GetType()
					== json::ValueType::DoubleVal)) {
		m_dPickDuplicateTimeWindow = (*com)["PickDuplicateWindow"].ToDouble();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using PickDuplicateWindow: "
						+ std::to_string(m_dPickDuplicateTimeWindow));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default PickDuplicateWindow: "
						+ std::to_string(m_dPickDuplicateTimeWindow));
	}

	// set maximum number of hypos
	if ((com->HasKey("HypoMax"))
			&& ((*com)["HypoMax"].GetType() == json::ValueType::IntVal)) {
		m_iMaxNumHypos = (*com)["HypoMax"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using HypoMax: "
						+ std::to_string(m_iMaxNumHypos));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default HypoMax: "
						+ std::to_string(m_iMaxNumHypos));
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
			&& ((*com)["NumWebThreads"].GetType() == json::ValueType::IntVal)) {
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

	int iMaxPicksPerHour = -1;
	if ((com->HasKey("MaxPicksPerHour"))
			&& ((*com)["MaxPicksPerHour"].GetType() == json::ValueType::IntVal)) {
		iMaxPicksPerHour = (*com)["MaxPicksPerHour"].ToInt();

		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using MaxPicksPerHour: "
						+ std::to_string(iMaxPicksPerHour));
	} else {
		glassutil::CLogit::log(
				glassutil::log_level::info,
				"CGlass::initialize: Using default MaxPicksPerHour: "
						+ std::to_string(iMaxPicksPerHour));
	}

	// test sig and gaus
	// NOTE: Keep for unit test reference
	// for (double sg = 0.0; sg < 5.0; sg += 0.1) {
	// printf("%.2f %.4f %.4f\n", sg, Gaus(sg, 1.0), Sig(sg, 1.0));
	// }

	// clean out old site list if any
	if (m_pSiteList) {
		delete (m_pSiteList);
	}

	// create site list
	m_pSiteList = new CSiteList();
	m_pSiteList->setGlass(this);
	m_pSiteList->setHoursWithoutPicking(iHoursWithoutPicking);
	m_pSiteList->setHoursBeforeLookingUp(iHoursBeforeLookingUp);
	m_pSiteList->setMaxPicksPerHour(iMaxPicksPerHour);

	// clean out old web list if any
	if (m_pWebList) {
		delete (m_pWebList);
	}

	// create detection web list
	m_pWebList = new CWebList(numWebThreads);
	m_pWebList->setGlass(this);
	m_pWebList->setSiteList(m_pSiteList);

	// clean out old pick list if any
	if (m_pPickList) {
		delete (m_pPickList);
	}

	// create pick list
	m_pPickList = new CPickList(numNucleationThreads);
	m_pPickList->setGlass(this);
	m_pPickList->setSiteList(m_pSiteList);

	// clean out old correlation list if any
	if (m_pCorrelationList) {
		delete (m_pCorrelationList);
	}

	// create correlation list
	m_pCorrelationList = new CCorrelationList();
	m_pCorrelationList->setGlass(this);
	m_pCorrelationList->setSiteList(m_pSiteList);

	// clean out old hypo list if any
	if (m_pHypoList) {
		delete (m_pHypoList);
	}

	// create hypo list
	m_pHypoList = new CHypoList(numHypoThreads);
	m_pHypoList->setGlass(this);

	// create detection processor
	m_pDetectionProcessor = new CDetection();
	m_pDetectionProcessor->setGlass(this);

	return (true);
}

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
bool CGlass::healthCheck() {
	// nullcheck
	if (m_pPickList == NULL) {
		return (false);
	}

	// check pick list
	if (m_pPickList->healthCheck() == false) {
		return (false);
	}

	// hypo list
	if (m_pHypoList->healthCheck() == false) {
		return (false);
	}

	// site list
	if (m_pSiteList->healthCheck() == false) {
		return (false);
	}

	// webs
	if (m_pWebList->healthCheck() == false) {
		return (false);
	}

	// all is well
	return (true);
}

// -------------------------------------------------getBeamMatchingAzimuthWindow
double CGlass::getBeamMatchingAzimuthWindow() const {
	return (m_dBeamMatchingAzimuthWindow);
}

// ------------------------------------------------getBeamMatchingDistanceWindow
double CGlass::getBeamMatchingDistanceWindow() const {
	return (m_dBeamMatchingDistanceWindow);
}

// ------------------------------------------------getCorrelationCancelAge
int CGlass::getCorrelationCancelAge() const {
	return (m_iCorrelationCancelAge);
}

// ---------------------------------------------getCorrelationMatchingTimeWindow
double CGlass::getCorrelationMatchingTimeWindow() const {
	return (m_dCorrelationMatchingTimeWindow);
}

// -----------------------------------------getCorrelationMatchingDistanceWindow
double CGlass::getCorrelationMatchingDistanceWindow() const {
	return (m_dCorrelationMatchingDistanceWindow);
}

// ------------------------------------------------getDistanceCutoffFactor
double CGlass::getDistanceCutoffFactor() const {
	return (m_dDistanceCutoffFactor);
}

// ------------------------------------------------getMinDistanceCutoff
double CGlass::getMinDistanceCutoff() const {
	return (m_dMinDistanceCutoff);
}

// ------------------------------------------------getDistanceCutoffPercentage
double CGlass::getDistanceCutoffPercentage() const {
	return (m_dDistanceCutoffPercentage);
}

// ------------------------------------------------getReportingStackThreshold
double CGlass::getReportingStackThreshold() const {
	return (m_dReportingStackThreshold);
}

// ------------------------------------------------getNucleationStackThreshold
double CGlass::getNucleationStackThreshold() const {
	return (m_dNucleationStackThreshold);
}

// ------------------------------------------------getPickAffinityExpFactor
double CGlass::getPickAffinityExpFactor() const {
	return (m_dPickAffinityExpFactor);
}

// ------------------------------------------------getGraphicsOut
bool CGlass::getGraphicsOut() const {
	return (m_bGraphicsOut);
}

// ------------------------------------------------getGraphicsOutFolder
const std::string& CGlass::getGraphicsOutFolder() const {
	return (m_sGraphicsOutFolder);
}

// ------------------------------------------------getGraphicsStepKm
double CGlass::getGraphicsStepKm() const {
	return (m_dGraphicsStepKM);
}

// ------------------------------------------------getGraphicsSteps
int CGlass::getGraphicsSteps() const {
	return (m_iGraphicsSteps);
}

// ------------------------------------------------getCycleLimit
int CGlass::getProcessLimit() const {
	return (m_iProcessLimit);
}

// ------------------------------------------------getMinimizeTTLocator
bool CGlass::getMinimizeTTLocator() const {
	return (m_bMinimizeTTLocator);
}

// ------------------------------------------------getMaxNumCorrelations
int CGlass::getMaxNumCorrelations() const {
	return (m_iMaxNumCorrelations);
}

// ------------------------------------------------getNumStationsPerNode
int CGlass::getNumStationsPerNode() const {
	return (m_iNumStationsPerNode);
}

// ------------------------------------------------getMaxNumHypos
int CGlass::getMaxNumHypos() const {
	return (m_iMaxNumHypos);
}

// ------------------------------------------------getNucleationDataThreshold
int CGlass::getNucleationDataThreshold() const {
	return (m_iNucleationDataThreshold);
}

// ------------------------------------------------getMaxNumPicks
int CGlass::getMaxNumPicks() const {
	return (m_iMaxNumPicks);
}

// ------------------------------------------------getReportingDataThreshold
double CGlass::getReportingDataThreshold() const {
	return (m_iReportingDataThreshold);
}

// ------------------------------------------------getMaxNumPicksPerSite
int CGlass::getMaxNumPicksPerSite() const {
	return (m_iMaxNumPicksPerSite);
}

// ------------------------------------------------getCorrelationList
CCorrelationList*& CGlass::getCorrelationList() {
	return (m_pCorrelationList);
}

// ------------------------------------------------getDetectionProcessor
CDetection*& CGlass::getDetectionProcessor() {
	return (m_pDetectionProcessor);
}

// ------------------------------------------------getHypoList
CHypoList*& CGlass::getHypoList() {
	return (m_pHypoList);
}

// ------------------------------------------------getPickDuplicateTimeWindow
double CGlass::getPickDuplicateTimeWindow() const {
	return (m_dPickDuplicateTimeWindow);
}

// ------------------------------------------------getPickList
CPickList*& CGlass::getPickList() {
	return (m_pPickList);
}

// ------------------------------------------------getSiteList
CSiteList*& CGlass::getSiteList() {
	return (m_pSiteList);
}

// -----------------------------------------------getDefaultNucleationTravelTime
std::shared_ptr<traveltime::CTravelTime>& CGlass::getDefaultNucleationTravelTime() {  // NOLINT
	std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
	return (m_pDefaultNucleationTravelTime);
}

// ------------------------------------------------getAssociationTravelTimes
std::shared_ptr<traveltime::CTTT>& CGlass::getAssociationTravelTimes() {
	std::lock_guard<std::mutex> ttGuard(m_TTTMutex);
	return (m_pAssociationTravelTimes);
}

// ------------------------------------------------getWebList
CWebList*& CGlass::getWebList() {
	return (m_pWebList);
}

// ------------------------------------------------getAssociationSDCutoff
double CGlass::getAssociationSDCutoff() const {
	return (m_dAssociationSDCutoff);
}

// ------------------------------------------------getPruningSDCutoff
double CGlass::getPruningSDCutoff() const {
	return (m_dPruningSDCutoff);
}

// ------------------------------------------------getTestLocator
bool CGlass::getTestLocator() const {
	return (m_bTestLocator);
}

// ------------------------------------------------getTestTravelTimes
bool CGlass::getTestTravelTimes() const {
	return (m_bTestTravelTimes);
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

}  // namespace glasscore
