/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef ZONESTATS_H
#define ZONESTATS_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

namespace traveltime {

/**
 * \brief CZoneStats information structure
 *
 * ZoneStatsInfoStruct contains the information for a single line in the
 * CZoneStats vector.
 */
typedef struct _ZoneStatsInfoStruct {
	float fLat;
	float fLon;
	int nEventCount;
	float fMaxDepth;
	float fMinDepth;
	float fAvgDepth;
	float fMaxMag;
	float fMinMag;
	float fAvgMag;
} ZoneStatsInfoStruct;

/**
 * \brief ZoneStatsInfoStruct comparison function
 *
 * ZSLatLonCompareLessThan contains the comparison function for sorting the
 * CZoneStats vector.
 */
bool ZSLatLonCompareLessThan(const ZoneStatsInfoStruct & zs1,
								const ZoneStatsInfoStruct & zs2);
/**
 * \brief ZoneStatsInfoStruct Latitude/Longitude comparison function
 *
 * ZSCompareByLatLon contains the comparison function used by lower_bound when
 * searching for data in the CZoneStats vector
 */
bool ZSCompareByLatLon(const ZoneStatsInfoStruct &lhs,
						const ZoneStatsInfoStruct &rhs);

/**
 * \brief glassutil zonestats class
 *
 * The CZonestats class encapsulates the logic and functionality needed
 * to load and serve zone statistics for historical seismic events
 */
class CZoneStats {
 public:
	/**
	 * \brief CZoneStats constructor
	 *
	 * The constructor for the CZoneStats class.
	 */
	CZoneStats();

	/**
	 * \brief CZoneStats destructor
	 *
	 * The destructor for the CZoneStats class.
	 */
	~CZoneStats();

	/**
	 * \brief CZoneStats configuration function
	 *
	 * This function configures the CZoneStats class
	 * \param pConfigFileName - A pointer to the config filename, from which to
	 * read zonestats info.
	 * \return returns true if successful.
	 */
	virtual bool setup(const std::string * pConfigFileName);

	/**
	 * \brief CZoneStats clear function
	 *
	 * The clear function for the CZoneStats class.
	 * Clears all zonestats info.
	 */
	virtual void clear();

	/**
	 * \brief CZoneStats retrieval function
	 *
	 * This function calculates the significance function for glasscore,
	 * which is the bell shaped curve with sig(0, x) pinned to 0.
	 *
	 * \param tdif - A double containing x value.
	 * \param sig - A double value containing the sigma,
	 * \return Returns a double value containing significance function result
	 */
	const ZoneStatsInfoStruct * getZonestatsInfoForLatLon(double dLat,
															double dLon);

	float getMaxDepthForLatLon(double dLat, double dLon);

	float getRelativeObservabilityOfSeismicEventsAtLocation(double dLat,
															double dLon);

	static const char* szLatGridBinSize;

	static const char* szLonGridBinSize;

	static const char* szLatLonHeader;

	static const int nNumExpectedFields = 9;

	static const int iParseStateGetSZRecords = 3;

	static const int iParseStateGetLatGBS = 0;

	static const int iParseStateGetLonGBS = 1;

	static const int iParseStateGetRecordHeader = 2;

	static const float depthInvalid;

 protected:
	std::vector<ZoneStatsInfoStruct> m_vZSData;

	int m_nTotalNumEvents;

	double m_dAvgObservabilityPerBin;

	float fLatBinSizeDeg;

	float fLonBinSizeDeg;

	ZoneStatsInfoStruct m_ZSDefault;
};  // CZoneStats class
}  // namespace traveltime
#endif  // ZONESTATS_H
