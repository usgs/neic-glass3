#include <associator.h>
#include <logger.h>
#include <ctime>
#include <string>
#include <memory>
#include <Logit.h>
#include <Glass.h>
#include <HypoList.h>
#include <PickList.h>

namespace glass3 {
namespace process {

// ---------------------------------------------------------Associator
Associator::Associator(glass3::util::iInput* inputint,
						glass3::util::iOutput* outputint)
		: glass3::util::ThreadBaseClass("Associator", 5) {
	glass3::util::log("debug",
						"associator::associator(...): Advanced Construction.");

	m_iInputCounter = 0;
	m_iTotalInputCounter = 0;
	m_iRunningAverageCounter = 0;
	m_dRunningDPSAverage = 0;

	m_iReportInterval = 60;
	std::time(&tLastPerformanceReport);

	m_Input = inputint;
	m_Output = outputint;
	m_pGlass = new glasscore::CGlass();
	m_MessageQueue = new glass3::util::Queue();

	// hook up glass communication with Associator class
	m_pGlass->piSend = dynamic_cast<glasscore::IGlassSend *>(this);

	// set up glass to use our logging
	glassutil::CLogit::setLogCallback(
			std::bind(&Associator::logGlass, this, std::placeholders::_1));

	setHealthCheckInterval(600);

	tGlasscoreDuration = std::chrono::duration<double>::zero();

	// clear / create object(s)
	clear();
}

// ---------------------------------------------------------~Associator
Associator::~Associator() {
	glass3::util::log("debug", "associator::~Associator(): Destruction.");

	// stop the processing thread
	stop();

	m_Input = NULL;
	m_Output = NULL;

	// delete glass
	if (m_pGlass != NULL)
		delete (m_pGlass);

	// clean up message queue
	m_MessageQueue->clear();
	if (m_MessageQueue != NULL) {
		delete (m_MessageQueue);
	}
}

// ---------------------------------------------------------setup
bool Associator::setup(std::shared_ptr<const json::Object> config) {
	if (m_Input == NULL) {
		glass3::util::log("error",
							"associator::setup(): m_Input interface is NULL .");
		return (false);
	}

	if (m_Output == NULL) {
		glass3::util::log(
				"error", "associator::setup(): m_Output interface is NULL .");
		return (false);
	}

	if (m_pGlass == NULL) {
		glass3::util::log(
				"error", "associator::setup(): Class Core interface is NULL .");
		return (false);
	}
	std::shared_ptr<json::Object> pConfig = std::make_shared<json::Object>(
			*config);
	// send the config to glass
	m_pGlass->dispatch(pConfig);
	glass3::util::log(
			"debug", "associator::setup(): Done Passing in provided config.");

	return (true);
}

// ---------------------------------------------------------clear
void Associator::clear() {
	glass3::util::log("debug", "associator::clear(): clearing configuration.");

	// finally do baseclass clear
	glass3::util::ThreadBaseClass::clear();
}

// ---------------------------------------------------------logGlass
void Associator::logGlass(glassutil::logMessageStruct message) {
	if (message.level == glassutil::log_level::info) {
		glass3::util::log("info", "glasscore: " + message.message);
	} else if (message.level == glassutil::log_level::debug) {
		glass3::util::log("debug", "glasscore: " + message.message);
	} else if (message.level == glassutil::log_level::warn) {
		glass3::util::log("warning", "glasscore: " + message.message);
	} else if (message.level == glassutil::log_level::error) {
		glass3::util::log("error", "glasscore: " + message.message);
	}
}

// ---------------------------------------------------------Send
void Associator::Send(std::shared_ptr<json::Object> communication) {
	// tell base class we're still alive
	ThreadBaseClass::setThreadHealth();

	if (communication == NULL) {
		glass3::util::log("critical",
							"associator::dispatch(): NULL message passed in.");
		return;
	}

	// send to output
	if (m_Output != NULL) {
		m_Output->sendToOutput(communication);
	} else {
		glass3::util::log(
				"error",
				"associator::dispatch(): m_Output interface is NULL, nothing "
				"to dispatch to.");
		return;
	}

	return;
}

// ---------------------------------------------------------sendToAssociator
void Associator::sendToAssociator(std::shared_ptr<json::Object> &message) {
	if (m_MessageQueue != NULL) {
		m_MessageQueue->addDataToQueue(message);
	}
}

// -----------------------------------------------------------------------work
glass3::util::WorkState Associator::work() {
	// nullchecks
	if (m_Input == NULL) {
		return (glass3::util::WorkState::Error);
	}
	if (m_pGlass == NULL) {
		return (glass3::util::WorkState::Error);
	}
	if (m_MessageQueue == NULL) {
		return (glass3::util::WorkState::Error);
	}

	// first check to see if we have any messages to send to glass
	// is usually glass configuration or data requests (i.e. ReqHypo)
	std::shared_ptr<json::Object> message = m_MessageQueue->getDataFromQueue();

	if (message != NULL) {
		// send the message into glass
		m_pGlass->dispatch(message);
	}

	std::time_t tNow;
	std::time(&tNow);

	// now get the next input data from the input library,
	// can be a pick, correlation, station, or detection
	std::shared_ptr<json::Object> data = m_Input->getInputData();

	// was there anything
	if (data == NULL) {
		// no
		return (glass3::util::WorkState::Idle);
	}

	// glass can sort things out from here
	// note that if this takes too long, we may need to adjust
	// thread monitoring, or add a call to setworkcheck()
	std::chrono::high_resolution_clock::time_point tGlassStartTime =
			std::chrono::high_resolution_clock::now();
	m_pGlass->dispatch(data);
	std::chrono::high_resolution_clock::time_point tGlassEndTime =
			std::chrono::high_resolution_clock::now();

	m_iInputCounter++;

	// keep track of the time we spent in glassland
	tGlasscoreDuration += std::chrono::duration_cast<
			std::chrono::duration<double>>(tGlassEndTime - tGlassStartTime);

	// generate periodic performance reports, reporting our pending input
	// queue size, data sent to glasscore, average glasscore processing time,
	// data per second and running average of data per second
	// this is used to monitor glasscore performance
	if ((tNow - tLastPerformanceReport) >= m_iReportInterval) {
		int pendingdata = m_Input->getInputDataCount();
		double averageglasstime = tGlasscoreDuration.count() / m_iInputCounter;

		if (m_iInputCounter == 0) {
			glass3::util::log(
					"warning",
					"associator::work(): Sent NO data to glass in the last "
							+ std::to_string(
									static_cast<int>(tNow
											- tLastPerformanceReport))
							+ " seconds.");
		} else {
			int hypoListSize = 0;
			int pickListSize = 0;
			if (m_pGlass->getHypoList()) {
				hypoListSize = m_pGlass->getHypoList()->getVHypoSize();
			}
			if (m_pGlass->getPickList()) {
				pickListSize = m_pGlass->getPickList()->getVPickSize();
			}

			// update the total input count with the input count
			// since hte last report
			m_iTotalInputCounter += m_iInputCounter;

			// calculate data per second average since the last report
			double dataAverage = static_cast<double>(m_iInputCounter)
					/ static_cast<double>((tNow - tLastPerformanceReport));

			// calculate running average of the data per second
			m_iRunningAverageCounter++;
			if (m_iRunningAverageCounter == 1) {
				m_dRunningDPSAverage = dataAverage;
			}
			m_dRunningDPSAverage = (m_dRunningDPSAverage
					* (m_iRunningAverageCounter - 1) + dataAverage)
					/ m_iRunningAverageCounter;

			// log the report
			glass3::util::log(
					"info",
					"Associator::work(): Sent "
							+ std::to_string(m_iInputCounter)
							+ " data to glasscore ("
							+ std::to_string(pendingdata) + " in queue, "
							+ std::to_string(m_iTotalInputCounter)
							+ " total) in "
							+ std::to_string(
									static_cast<int>(tNow
											- tLastPerformanceReport))
							+ " seconds. (" + std::to_string(dataAverage)
							+ " dps) (" + std::to_string(m_dRunningDPSAverage)
							+ " avg dps) (" + std::to_string(averageglasstime)
							+ " avg glass time) (" + "vPickSize: "
							+ std::to_string(pickListSize) + " vHypoSize: "
							+ std::to_string(hypoListSize) + ").");
		}

		// reset for next report
		tLastPerformanceReport = tNow;
		m_iInputCounter = 0;
		tGlasscoreDuration = std::chrono::duration<double>::zero();
	}

	// we only send in one item per work loop
	// work was successful
	return (glass3::util::WorkState::OK);
}

// -----------------------------------------------------------------healthCheck
bool Associator::healthCheck() {
	// don't check m_pGlass if it is not created yet
	if (m_pGlass != NULL) {
		// check glass
		// check glass thread status
		if (m_pGlass->statusCheck() == false) {
			glass3::util::log(
					"error",
					"Associator::statusCheck(): GlassLib statusCheck() returned false!.");
			return (false);
		}
	}

	// let threadbaseclass handle background worker thread
	return (ThreadBaseClass::healthCheck());
}
}  // namespace process
}  // namespace glass3
