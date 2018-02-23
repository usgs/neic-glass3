#include <associator.h>
#include <logger.h>
#include <ctime>
#include <string>
#include <memory>
#include <Logit.h>
#include <Glass.h>
#include <HypoList.h>
#include <PickList.h>

namespace glass {
// Construction/Destruction
Associator::Associator()
		: util::ThreadBaseClass("Associator", 5) {
	logger::log("debug", "associator::Associator(): Construction.");

	m_iWorkCounter = 0;
	m_iTotalWorkCounter = 0;
	m_iRunningAverageCounter = 0;
	m_dRunningAverage = 0;

	ReportInterval = 60;
	std::time(&tLastWorkReport);

	Input = NULL;
	Output = NULL;
	m_pGlass = NULL;
	m_MessageQueue = NULL;

	m_iCheckInterval = 600;

	tGlassDuration = std::chrono::duration<double>::zero();

	// clear / create object(s)
	clear();
}

Associator::Associator(util::iInput* inputint, util::iOutput* outputint)
		: util::ThreadBaseClass("Associator", 5) {
	logger::log("debug", "associator::associator(...): Advanced Construction.");

	m_pGlass = NULL;
	m_MessageQueue = NULL;
	m_iWorkCounter = 0;
	m_iTotalWorkCounter = 0;
	m_iRunningAverageCounter = 0;
	m_dRunningAverage = 0;

	ReportInterval = 60;
	std::time(&tLastWorkReport);

	// clear / create object(s)
	clear();

	// fill in the interfaces
	Input = inputint;
	Output = outputint;

	m_iCheckInterval = 600;

	tGlassDuration = std::chrono::duration<double>::zero();
}

Associator::~Associator() {
	logger::log("debug", "associator::~Associator(): Destruction.");

	// stop the processing thread
	stop();

	Input = NULL;
	Output = NULL;

	// delete glass
	if (m_pGlass != NULL)
		delete (m_pGlass);

	// clean up message queue
	m_MessageQueue->clearQueue();
	if (m_MessageQueue != NULL) {
		delete (m_MessageQueue);
	}
}

bool Associator::setup(json::Object *config) {
	if (Input == NULL) {
		logger::log("error", "associator::setup(): Input interface is NULL .");
		return (false);
	}

	if (Output == NULL) {
		logger::log("error", "associator::setup(): Output interface is NULL .");
		return (false);
	}

	if (m_pGlass == NULL) {
		logger::log("error",
					"associator::setup(): Class Core interface is NULL .");
		return (false);
	}
	std::shared_ptr<json::Object> pConfig = std::make_shared<json::Object>(
			*config);
	// send the config to glass
	m_pGlass->dispatch(pConfig);
	logger::log("debug",
				"associator::setup(): Done Passing in provided config.");

	return (true);
}

void Associator::clear() {
	logger::log("debug", "associator::clear(): clearing configuration.");

	// we "clear" by deleting the whole glass object
	if (m_pGlass != NULL) {
		delete (m_pGlass);
	}
	// create the glass object
	m_pGlass = new glasscore::CGlass();

	// hook up glass communication with Associator class
	m_pGlass->piSend = dynamic_cast<glasscore::IGlassSend *>(this);

	// set up glass to use our logging
	glassutil::CLogit::setLogCallback(
			std::bind(&Associator::logGlass, this, std::placeholders::_1));

	if (m_MessageQueue != NULL) {
		delete (m_MessageQueue);
	}
	m_MessageQueue = new util::Queue();

	// finally do baseclass clear
	util::BaseClass::clear();
}

void Associator::logGlass(glassutil::logMessageStruct message) {
	if (message.level == glassutil::log_level::info) {
		logger::log("info", "glasscore: " + message.message);
	} else if (message.level == glassutil::log_level::debug) {
		logger::log("debug", "glasscore: " + message.message);
	} else if (message.level == glassutil::log_level::warn) {
		logger::log("warning", "glasscore: " + message.message);
	} else if (message.level == glassutil::log_level::error) {
		logger::log("error", "glasscore: " + message.message);
	}
}

void Associator::Send(std::shared_ptr<json::Object> communication) {
	// this probably could be the same function as dispatch...
	// ...except the interface won't let it be
	dispatch(communication);
}

void Associator::sendToAssociator(std::shared_ptr<json::Object> message) {
	if (m_MessageQueue != NULL) {
		m_MessageQueue->addDataToQueue(message);
	}
}

bool Associator::work() {
	if (Input == NULL) {
		return (false);
	}

	if (m_pGlass == NULL) {
		return (false);
	}

	if (m_MessageQueue == NULL) {
		return (false);
	}

	// first check to see if we have any messages to send
	std::shared_ptr<json::Object> message = m_MessageQueue->getDataFromQueue();

	if (message != NULL) {
		// send the message into glass
		m_pGlass->dispatch(message);
	}

	std::time_t tNow;
	std::time(&tNow);

	// now grab whatever input might have for us and send it into glass
	std::shared_ptr<json::Object> data = Input->getData();

	// only send in something if we got something
	if (data != NULL) {
		m_iWorkCounter++;

		std::chrono::high_resolution_clock::time_point tGlassStartTime =
				std::chrono::high_resolution_clock::now();
		// glass can sort things out from here
		// note that if this takes too long, we may need to adjust
		// thread monitoring, or add a call to setworkcheck()
		m_pGlass->dispatch(data);
		std::chrono::high_resolution_clock::time_point tGlassEndTime =
				std::chrono::high_resolution_clock::now();

		tGlassDuration += std::chrono::duration_cast<
				std::chrono::duration<double>>(tGlassEndTime - tGlassStartTime);
	}

	if ((tNow - tLastWorkReport) >= ReportInterval) {
		int pendingdata = Input->dataCount();
		double averageglasstime = tGlassDuration.count() / m_iWorkCounter;

		if (m_iWorkCounter == 0) {
			logger::log(
					"warning",
					"associator::work(): Sent NO data to glass in the last "
							+ std::to_string(
									static_cast<int>(tNow - tLastWorkReport))
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

			m_iTotalWorkCounter += m_iWorkCounter;

			// calculate data per second average
			double dataAverage = static_cast<double>(m_iWorkCounter)
					/ static_cast<double>((tNow - tLastWorkReport));

			// calculate running average
			m_iRunningAverageCounter++;
			if (m_iRunningAverageCounter == 1) {
				m_dRunningAverage = dataAverage;
			}

			m_dRunningAverage = (m_dRunningAverage
					* (m_iRunningAverageCounter - 1) + dataAverage)
					/ m_iRunningAverageCounter;

			logger::log(
					"info",
					"Associator::work(): Sent " + std::to_string(m_iWorkCounter)
							+ " data to glass (" + std::to_string(pendingdata)
							+ " in queue, " + std::to_string(m_iTotalWorkCounter)
							+ " total) in "
							+ std::to_string(
									static_cast<int>(tNow - tLastWorkReport))
							+ " seconds. (" + std::to_string(dataAverage)
							+ " dps) (" + std::to_string(m_dRunningAverage)
							+ " avg dps) ("
							+ std::to_string(averageglasstime)
							+ " avg glass time) (" + "vPickSize: "
							+ std::to_string(pickListSize) + " vHypoSize: "
							+ std::to_string(hypoListSize) + ").");
		}

		tLastWorkReport = tNow;
		m_iWorkCounter = 0;
		tGlassDuration = std::chrono::duration<double>::zero();
	}

	// we only send in one item per work loop
	// work was successful
	return (true);
}

bool Associator::check() {
	// don't check m_pGlass if it is not created yet
	if (m_pGlass != NULL) {
		// check glass
		// check glass thread status
		if (m_pGlass->statusCheck() == false) {
			logger::log(
					"error",
					"Associator::check(): GlassLib statusCheck() returned false!.");
			return (false);
		}
	}

	// let threadbaseclass handle background worker thread
	return (ThreadBaseClass::check());
}

// process any messages glasscore sends us
bool Associator::dispatch(std::shared_ptr<json::Object> communication) {
	// tell base class we're still alive
	ThreadBaseClass::setWorkCheck();

	if (communication == NULL) {
		logger::log("critical",
					"associator::dispatch(): NULL message passed in.");
		return (false);
	}

	// send to output
	if (Output != NULL) {
		// Allocate a new json object to avoid
		// multi-thread pointer issues.
		Output->sendToOutput(communication);
	} else {
		logger::log("error",
					"associator::dispatch(): Output interface is NULL, nothing "
					"to dispatch to.");
		return (false);
	}

	return (true);
}
}  // namespace glass
