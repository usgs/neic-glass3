#include <gtest/gtest.h>

#include <logger.h>

#include "ZoneStats.h"

#define TESTPATH "testdata"
#define ZSFILENAME "qa_zonestats.txt"

#define TESTLAT 36.0
#define TESTLON -115.0

#define LAT 36.0
#define LON -112.0
#define AVGDEPTH 5
#define MINDEPTH -2.52
#define MAXDEPTH 21.99
#define OBSERVABILITY 7.59264

TEST(ZoneStatsTest, CombinedTest) {
	traveltime::CZoneStats zs;
	const traveltime::ZoneStatsInfoStruct * pZSI;

	std::string zsfile = "./" + std::string(TESTPATH) + "/"
			+ std::string(ZSFILENAME);

	float fMaxDepth;
	float fObs;

	// setup
	ASSERT_TRUE(zs.setup(&zsfile));

	// get zone stats info
	pZSI = zs.getZonestatsInfoForLatLon(TESTLAT, TESTLON);
	ASSERT_TRUE(pZSI != NULL);

	// lat
	ASSERT_EQ(LAT, pZSI->fLat)<< "lat Check";

	// lon
	ASSERT_EQ(LON, pZSI->fLon)<< "lon Check";

	// avg depth
	ASSERT_NEAR(AVGDEPTH, pZSI->fAvgDepth, 0.001)<< "avg depth Check";

	// min depth
	ASSERT_NEAR(MINDEPTH, pZSI->fMinDepth, 0.001)<< "min depth Check";

	// max depth
	ASSERT_NEAR(MAXDEPTH, pZSI->fMaxDepth, 0.001)<< "max depth Check";

	// get max depth
	ASSERT_NEAR(MAXDEPTH, zs.getMaxDepthForLatLon(TESTLAT, TESTLON), 0.001)<<
	"get max depth Check";

	// get observability
	ASSERT_NEAR(OBSERVABILITY, zs.getRelativeObservabilityOfSeismicEventsAtLocation(TESTLAT, TESTLON), 0.001)<<  // NOLINT
	"get observability Check";
}
