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
	 * This function gets the zone stats information for a given latitude and
	 * longitude
	 * \param dLat - A double containing the latitude to use
	 * \param dLon - A double containing the longitude to use
	 * \return Returns pointer to a ZoneStatsInfoStruct containing the zone
	 * stats information
	 */
	const ZoneStatsInfoStruct * getZonestatsInfoForLatLon(double dLat,
															double dLon);

	/**
	 * \brief Max Depth retrieval function
	 *
	 * This function gets the maximum depth for a given latitude and longitude
	 * \param dLat - A double containing the latitude to use
	 * \param dLon - A double containing the longitude to use
	 * \return Returns a double value containing the maximum depth
	 */
	float getMaxDepthForLatLon(double dLat, double dLon);

	/**
	 * \brief relative observability retrieval function
	 *
	 * This function gets the relative observability for a given latitude and
	 * longitude
	 * \param dLat - A double containing the latitude to use
	 * \param dLon - A double containing the longitude to use
	 * \return Returns a double value containing the relative observability
	 */
	float getRelativeObservabilityOfSeismicEventsAtLocation(double dLat,
															double dLon);

	/**
	 * \brief A const char array representing the Lat grid bin string size
	 */
	static const char* szLatGridBinSize;

	/**
	 * \brief A const char array representing the Lon grid bin string size
	 */
	static const char* szLonGridBinSize;

	/**
	 * \brief A const char array representing the Lat/Lon header string size
	 */
	static const char* szLatLonHeader;

	/**
	 * \brief An integer constant containing the number of fields for parsing
	 */
	static const int nNumExpectedFields;

	/**
	 * \brief An integer constant defining the state for getting sz records
	 */
	static const int iParseStateGetSZRecords;

	/**
	 * \brief An integer constant defining the state for getting lat gbs
	 */
	static const int iParseStateGetLatGBS;

	/**
	 * \brief An integer constant defining the state for getting lon gbs
	 */
	static const int iParseStateGetLonGBS;

	/**
	 * \brief An integer constant defining the state for getting the record
	 * header
	 */
	static const int iParseStateGetRecordHeader;

	/**
	 * \brief A float constant containing the invalid depth value
	 */
	static const float depthInvalid;

 protected:
	/**
	 * \brief a std::vector of ZoneStatsInfoStruct's containing the zone stats
	 * information
	 */
	std::vector<ZoneStatsInfoStruct> m_vZSData;

	/**
	 * \brief An integer containing the total number of events in the zone stats
	 * information
	 */
	int m_nTotalNumEvents;

	/**
	 * \brief A double containing the average observability per bin
	 */
	double m_dAvgObservabilityPerBin;

	/**
	 * \brief A float containing the laititude bin size in degrees
	 */
	float fLatBinSizeDeg;

	/**
	 * \brief A float containing the longitude bin size in degrees
	 */
	float fLonBinSizeDeg;

	/**
	 * \brief a ZoneStatsInfoStruct containing the default zone stats
	 * information
	 */
	ZoneStatsInfoStruct m_ZSDefault;
};  // End CZoneStats class
}  // namespace traveltime
#endif  // ZONESTATS_H
