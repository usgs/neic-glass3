#include "ZoneStats.h"
#include <logger.h>
#include <stringutil.h>
#include <limits>
#include <algorithm>
#include <vector>
#include <string>

namespace traveltime {

// parsing values
const char* traveltime::CZoneStats::szLatGridBinSize = "LatGridBinSize";
const char* traveltime::CZoneStats::szLonGridBinSize = "LonGridBinSize";
const char* traveltime::CZoneStats::szLatLonHeader = "LAT  LON ";
const float traveltime::CZoneStats::depthInvalid = -999.0;

// -----------------------------------------------------ZSLatLonCompareLessThan
bool ZSLatLonCompareLessThan(const ZoneStatsInfoStruct & zs1,
								const ZoneStatsInfoStruct & zs2) {
	if (zs1.fLat + std::numeric_limits<float>::epsilon() < zs2.fLat) {
		return (true);
	} else if (zs1.fLat > zs2.fLat + std::numeric_limits<float>::epsilon()) {
		return (false);
	} else if (zs1.fLon + std::numeric_limits<float>::epsilon() >= zs2.fLon) {
		return (false);
	} else {
		return (true);
	}
}  // end ZSLatLonCompareLessThan()

// -----------------------------------------------------------ZSCompareByLatLon
bool ZSCompareByLatLon(const ZoneStatsInfoStruct &lhs,
						const ZoneStatsInfoStruct &rhs) {
	return (ZSLatLonCompareLessThan(lhs, rhs));
}

// ------------------------------------------------------------------CZoneStats
CZoneStats::CZoneStats() {
	clear();
}

// -----------------------------------------------------------------~CZoneStats
CZoneStats::~CZoneStats() {
	clear();
}

// -----------------------------------------------------------------------setup
bool CZoneStats::setup(const std::string * pConfigFileName) {
	std::ifstream fIn;
	ZoneStatsInfoStruct zsRecord;
	if ((!pConfigFileName) || (!pConfigFileName->length())) {
		glass3::util::Logger::log(
				"error",
				"CZoneStats::setup(): NULL Pointer or BLANK filename! Failing!");
		return (false);
	}
	fIn.open(*pConfigFileName, std::ios::in);
	if (!fIn.is_open()) {
		glass3::util::Logger::log(
				"error",
				"CZoneStats::setup(): Failed to open file: "
						+ *pConfigFileName);
		return (false);
	}
	std::string sLine = "";
	int iLineNum = 0;
	int iState = 0;
	while (fIn.good()) {
		std::getline(fIn, sLine);
		iLineNum++;
		if (sLine.length() == 0 || sLine[0] == '#')
			continue;
		static const int iParseStateGetSZRecords = 3;
		static const int iParseStateGetLatGBS = 0;
		static const int iParseStateGetLonGBS = 1;
		static const int iParseStateGetRecordHeader = 2;
		if (iState == this->iParseStateGetSZRecords) {
			// parse data line.
			// found the sLine we expected, parse the value.
			std::vector<std::string> vLineStrings = glass3::util::split(sLine,
																		' ');

			// LAT  LON   EVENT_COUNT  MAX_DEPTH  MIN_DEPTH  AVG_DEPTH  MIN_MAG  MAX_MAG  AVG_MAG  // NOLINT
			if (vLineStrings.size() < this->nNumExpectedFields) {
				glass3::util::Logger::log(
						"error",
						"CZoneStats::setup(): At line "
								+ std::to_string(iLineNum) + "of file \""
								+ (*pConfigFileName) + "\", parsed bad value <"
								+ std::to_string(fLonBinSizeDeg)
								+ "> during parsing of line <" + sLine
								+ "> while looking for "
								+ std::to_string(this->nNumExpectedFields)
								+ " fields, \"Ignoring line!");
				continue;
			}

			zsRecord.fLat = static_cast<float>(atof(vLineStrings[0].c_str()));
			zsRecord.fLon = static_cast<float>(atof(vLineStrings[1].c_str()));
			zsRecord.nEventCount = atoi(vLineStrings[2].c_str());
			zsRecord.fMaxDepth = static_cast<float>(atof(
					vLineStrings[3].c_str()));
			zsRecord.fMinDepth = static_cast<float>(atof(
					vLineStrings[4].c_str()));
			zsRecord.fAvgDepth = static_cast<float>(atof(
					vLineStrings[5].c_str()));
			zsRecord.fMaxMag =
					static_cast<float>(atof(vLineStrings[6].c_str()));
			zsRecord.fMinMag =
					static_cast<float>(atof(vLineStrings[7].c_str()));
			zsRecord.fAvgMag =
					static_cast<float>(atof(vLineStrings[8].c_str()));
			m_vZSData.push_back(zsRecord);
			m_nTotalNumEvents += zsRecord.nEventCount;
		} else if (iState == this->iParseStateGetLatGBS) {
			if (strncmp(this->szLatGridBinSize, sLine.c_str(),
						strlen(this->szLatGridBinSize)) == 0) {
				// found the sLine we expected, parse the value.
				std::vector<std::string> vLineStrings = glass3::util::split(
						sLine, ' ');

				if (vLineStrings.size() < 2) {
					glass3::util::Logger::log(
							"error",
							"CZoneStats::setup(): At line "
									+ std::to_string(iLineNum) + "of file \""
									+ *pConfigFileName
									+ "\", found imcomplete line <" + sLine
									+ "> while looking for \""
									+ this->szLatGridBinSize + "\" Aborting!");
					return (false);
				} else {
					fLatBinSizeDeg = static_cast<float>(atof(
							vLineStrings[1].c_str()));

					if (fLatBinSizeDeg <= 0.0) {
						glass3::util::Logger::log(
								"error",
								"CZoneStats::setup(): At line "
										+ std::to_string(iLineNum)
										+ "of file \"" + (*pConfigFileName)
										+ "\", parsed bad value <"
										+ std::to_string(fLatBinSizeDeg)
										+ "> during parsing of line <" + sLine
										+ "> while looking for \""
										+ std::string(this->szLatGridBinSize)
										+ "\" Aborting!");
						return (false);
					}
				}

				iState = this->iParseStateGetLonGBS;
				continue;
			} else {
				glass3::util::Logger::log(
						"error",
						"CZoneStats::setup(): At line "
								+ std::to_string(iLineNum) + "of file \""
								+ *pConfigFileName
								+ "\", found unexpected line <" + sLine
								+ "> while looking for \""
								+ this->szLatGridBinSize + "\" Aborting!");
				return (false);
			}
			// end if State == 0 - we're looking for LatGridSize
		} else if (iState == this->iParseStateGetLonGBS) {
			if (strncmp(this->szLonGridBinSize, sLine.c_str(),
						strlen(this->szLonGridBinSize)) == 0) {
				// found the sLine we expected, parse the value.
				std::vector<std::string> vLineStrings = glass3::util::split(
						sLine, ' ');

				if (vLineStrings.size() < 2) {
					glass3::util::Logger::log(
							"error",
							"CZoneStats::setup(): At line "
									+ std::to_string(iLineNum) + "of file \""
									+ *pConfigFileName
									+ "\", found imcomplete line <" + sLine
									+ "> while looking for \""
									+ this->szLonGridBinSize + "\" Aborting!");
					return (false);
				} else {
					fLonBinSizeDeg = static_cast<float>(atof(
							vLineStrings[1].c_str()));
					if (fLonBinSizeDeg <= 0.0) {
						glass3::util::Logger::log(
								"error",
								"CZoneStats::setup(): At line "
										+ std::to_string(iLineNum)
										+ "of file \"" + *pConfigFileName
										+ "\", parsed bad value <"
										+ std::to_string(fLonBinSizeDeg)
										+ "> during parsing of line <" + sLine
										+ "> while looking for \""
										+ this->szLonGridBinSize
										+ "\" Aborting!");
						return (false);
					}
				}

				iState = this->iParseStateGetRecordHeader;
				continue;
			} else {
				glass3::util::Logger::log(
						"error",
						"CZoneStats::setup(): At line "
								+ std::to_string(iLineNum) + "of file \""
								+ *pConfigFileName
								+ "\", found unexpected line <" + sLine
								+ "> while looking for \""
								+ this->szLonGridBinSize + "\" Aborting!");
				return (false);
			}
			// end if State == 1 - we're looking for LonGridSize
		} else if (iState == this->iParseStateGetRecordHeader) {
			if (strncmp(this->szLatLonHeader, sLine.c_str(),
						strlen(this->szLatLonHeader)) == 0) {
				// found the sLine we expected
				iState = this->iParseStateGetSZRecords;
				continue;
			} else {
				glass3::util::Logger::log(
						"error",
						"CZoneStats::setup(): At line "
								+ std::to_string(iLineNum) + "of file \""
								+ *pConfigFileName
								+ "\", found unexpected line <" + sLine
								+ "> while looking for \""
								+ this->szLatLonHeader + "\" Aborting!");
				return (false);
			}
			// end if State == 2 - we're looking for LonGridSize
		} else {
			glass3::util::Logger::log(
					"error",
					"CZoneStats::setup(): At line " + std::to_string(iLineNum)
							+ "of file \"" + *pConfigFileName
							+ "\", ended up in unexpected parsing state: "
							+ std::to_string(iState) + ".  Aborting!");
			return (false);
		}
	}  // end while file still has good stuff for us to grab.

	// check to see if we got anything meaningful.
	if (m_vZSData.size() <= 0) {
		glass3::util::Logger::log(
				"error",
				"CZoneStats::setup(): After parsing " + std::to_string(iLineNum)
						+ " lines of file \"" + *pConfigFileName
						+ "\", no Zone Statistics found!  In state "
						+ std::to_string(iState) + ".  Aborting!");
		return (false);
	}

	// sort data by lat/lon
	std::sort(m_vZSData.begin(), m_vZSData.end(), ZSLatLonCompareLessThan);
	m_dAvgObservabilityPerBin = m_nTotalNumEvents
			/ (180.0 / fLatBinSizeDeg * 360.0 / this->fLonBinSizeDeg);

	return (true);
}  // end setup()

// -----------------------------------------------------------------------clear
void CZoneStats::clear() {
	m_ZSDefault.fLat = static_cast<float>(0.0);
	m_ZSDefault.fLon = static_cast<float>(0.0);
	m_ZSDefault.nEventCount = 0;
	m_ZSDefault.fMaxDepth = static_cast<float>(10.0);
	m_ZSDefault.fMinDepth = static_cast<float>(0);
	m_ZSDefault.fAvgDepth = static_cast<float>(10.0);
	m_ZSDefault.fMaxMag = static_cast<float>(0.0);
	m_ZSDefault.fMinMag = static_cast<float>(0.0);
	m_ZSDefault.fAvgMag = static_cast<float>(0.0);
	m_vZSData.clear();
	m_nTotalNumEvents = 0;
	m_dAvgObservabilityPerBin = 0.0;
	fLatBinSizeDeg = 0.0;
	fLonBinSizeDeg = 0.0;
}

// ---------------------------------------------------GetZonestatsInfoForLatLon
const ZoneStatsInfoStruct * CZoneStats::getZonestatsInfoForLatLon(
		double dLat, double dLon) {
	if (m_nTotalNumEvents <= 0) {
		glass3::util::Logger::log(
				"error",
				"CZoneStats::GetZonestatsInfoForLatLon(): No ZS event data!");
		return (NULL);
	}
	if (fLatBinSizeDeg <= 0.0 || fLonBinSizeDeg <= 0.0) {
		glass3::util::Logger::log(
				"error",
				"CZoneStats::GetZonestatsInfoForLatLon(): No LL Bin size!");
		return (NULL);
	}

	ZoneStatsInfoStruct zsTemp;
	zsTemp.fLat = static_cast<float>((static_cast<int>(dLat / fLatBinSizeDeg)
			* fLatBinSizeDeg));
	zsTemp.fLon = static_cast<float>((static_cast<int>(dLon / fLonBinSizeDeg)
			* fLonBinSizeDeg));

	std::vector<ZoneStatsInfoStruct>::iterator itZS;
	itZS = std::lower_bound(m_vZSData.begin(), m_vZSData.end(), zsTemp,
							ZSCompareByLatLon);

	if (itZS == m_vZSData.end()) {
		return (&m_ZSDefault);
	} else if (fabs(itZS->fLat - zsTemp.fLat)
			> std::numeric_limits<float>::epsilon()
			|| fabs(itZS->fLon - zsTemp.fLon)
					> std::numeric_limits<float>::epsilon()) {
		return (&m_ZSDefault);
	} else {
		return (&(*itZS));
	}
}  // end GetZonestatsInfoForLatLon()

// --------------------------------------------------------GetMaxDepthForLatLon
float CZoneStats::getMaxDepthForLatLon(double dLat, double dLon) {
	const ZoneStatsInfoStruct * pZS;
	pZS = getZonestatsInfoForLatLon(dLat, dLon);

	if (!pZS) {
		return (this->depthInvalid);
	} else {
		return (pZS->fMaxDepth);
	}
}

// ---------------------------GetRelativeObservabilityOfSeismicEventsAtLocation
float CZoneStats::getRelativeObservabilityOfSeismicEventsAtLocation(
		double dLat, double dLon) {
	const ZoneStatsInfoStruct * pZS;
	pZS = getZonestatsInfoForLatLon(dLat, dLon);

	if (!pZS) {
		return (static_cast<float>(-1.0));
	} else {
		return (static_cast<float>(pZS->nEventCount / m_dAvgObservabilityPerBin));
	}
}
}  // namespace traveltime
