#include <gtest/gtest.h>
#include "Geo.h"
#include "Logit.h"

#define GEOCENTRICCLAT 41.921198129865765
#define GEOCENTRICLON -122.56219953573087
#define GEOCENTRICRAD 3342.0
#define CARTX -1338.358747830255
#define CARTY -2095.778248528537
#define CARTZ 2232.8173671621444
#define CARTT 4
#define TAG 1
#define UNITX -0.4004
#define UNITY -0.6271
#define UNITZ 0.6681

#define GEOGRAPHICLAT  42.113419953573087
#define GEOGRAPHICLON -122.56219953573087
#define GEOGRAPHICRAD 3342.0

#define DISTLAT1 0
#define DISTLON1 0
#define DISTELV1 1

#define DISTLAT2 42.113419953573087
#define DISTLON2 -122.56219953573087
#define DISTELV2 1

#define DISTANCE 113.60733936060119
#define AZIMUTH 316.81332287453034

#define DEG2RAD	0.01745329251994

// test to see if the hypo can be constructed
TEST(GeoTest, Construction) {
	glassutil::CLogit::disable();

	// construct a hypo
	glassutil::CGeo * testGeo = new glassutil::CGeo();

	// assert default values
	ASSERT_EQ(0, testGeo->dLat)<< "dLat is zero";
	ASSERT_EQ(0, testGeo->dLon)<< "dLon is zero";
	ASSERT_EQ(0, testGeo->dRad)<< "dRad is zero";
	ASSERT_EQ(0, testGeo->dX)<< "dX is zero";
	ASSERT_EQ(0, testGeo->dY)<< "dY is zero";
	ASSERT_EQ(0, testGeo->dZ)<< "dZ is zero";
	ASSERT_EQ(0, testGeo->dT)<< "dT is zero";
	ASSERT_EQ(0, testGeo->iTag)<< "iTag is zero";
	ASSERT_EQ(0, testGeo->uX)<< "uX is zero";
	ASSERT_EQ(0, testGeo->uY)<< "uY is zero";
	ASSERT_EQ(0, testGeo->uZ)<< "uZ is zero";

	// now init
	testGeo->initialize(GEOCENTRICCLAT, GEOCENTRICLON, GEOCENTRICRAD, CARTX,
	CARTY,
						CARTZ, CARTT, TAG, UNITX, UNITY, UNITZ);

	// check results
	// check lat
	double latitude = testGeo->dLat;
	double expectedLatitude = GEOCENTRICCLAT;
	ASSERT_NEAR(latitude, expectedLatitude, 0.001);

	// check lon
	double longitude = testGeo->dLon;
	double expectedLongitude = GEOCENTRICLON;
	ASSERT_NEAR(longitude, expectedLongitude, 0.001);

	// check radius
	double radius = testGeo->dRad;
	double expectedRadius = GEOCENTRICRAD;
	ASSERT_NEAR(radius, expectedRadius, 0.001);

	// check dX
	double x = testGeo->dX;
	double expectedX = CARTX;
	ASSERT_NEAR(x, expectedX, 0.001);

	// check dY
	double y = testGeo->dY;
	double expectedY = CARTY;
	ASSERT_NEAR(y, expectedY, 0.001);

	// check dZ
	double z = testGeo->dZ;
	double expectedZ = CARTZ;
	ASSERT_NEAR(z, expectedZ, 0.001);

	// check dT
	double t = testGeo->dT;
	double expectedT = CARTT;
	ASSERT_NEAR(t, expectedT, 0.001);

	// check tag
	int tag = testGeo->iTag;
	int expectedTag = TAG;
	ASSERT_EQ(tag, expectedTag);

	// check uX
	double uX = testGeo->uX;
	double expecteduX = UNITX;
	ASSERT_NEAR(uX, expecteduX, 0.001);

	// check uY
	double uY = testGeo->uY;
	double expecteduY = UNITY;
	ASSERT_NEAR(uY, expecteduY, 0.001);

	// check uZ
	double uZ = testGeo->uZ;
	double expecteduZ = UNITZ;
	ASSERT_NEAR(uZ, expecteduZ, 0.001);

	// now clear
	testGeo->clear();

	// assert default values
	ASSERT_EQ(0, testGeo->dLat)<< "dLat is zero";
	ASSERT_EQ(0, testGeo->dLon)<< "dLon is zero";
	ASSERT_EQ(0, testGeo->dRad)<< "dRad is zero";
	ASSERT_EQ(0, testGeo->dX)<< "dX is zero";
	ASSERT_EQ(0, testGeo->dY)<< "dY is zero";
	ASSERT_EQ(0, testGeo->dZ)<< "dZ is zero";
	ASSERT_EQ(0, testGeo->dT)<< "dT is zero";
	ASSERT_EQ(0, testGeo->iTag)<< "iTag is zero";
	ASSERT_EQ(0, testGeo->uX)<< "uX is zero";
	ASSERT_EQ(0, testGeo->uY)<< "uY is zero";
	ASSERT_EQ(0, testGeo->uZ)<< "uZ is zero";
}

TEST(GeoTest, Geographic) {
	glassutil::CLogit::disable();

	// construct a hypo
	glassutil::CGeo * testGeo = new glassutil::CGeo();

	testGeo->setGeographic(GEOGRAPHICLAT, GEOGRAPHICLON, GEOGRAPHICRAD);

	// check results
	// check lat
	double latitude = testGeo->dLat;
	double expectedLatitude = GEOCENTRICCLAT;
	ASSERT_NEAR(latitude, expectedLatitude, 0.001);

	// check lon
	double longitude = testGeo->dLon;
	double expectedLongitude = GEOCENTRICLON;
	ASSERT_NEAR(longitude, expectedLongitude, 0.001);

	// check radius
	double radius = testGeo->dRad;
	double expectedRadius = GEOCENTRICRAD;
	ASSERT_NEAR(radius, expectedRadius, 0.001);

	// check dX
	double x = testGeo->dX;
	double expectedX = CARTX;
	ASSERT_NEAR(x, expectedX, 0.001);

	// check dY
	double y = testGeo->dY;
	double expectedY = CARTY;
	ASSERT_NEAR(y, expectedY, 0.001);

	// check dZ
	double z = testGeo->dZ;
	double expectedZ = CARTZ;
	ASSERT_NEAR(z, expectedZ, 0.001);

	// check uX
	double uX = testGeo->uX;
	double expecteduX = UNITX;
	ASSERT_NEAR(uX, expecteduX, 0.001);

	// check uY
	double uY = testGeo->uY;
	double expecteduY = UNITY;
	ASSERT_NEAR(uY, expecteduY, 0.001);

	// check uZ
	double uZ = testGeo->uZ;
	double expecteduZ = UNITZ;
	ASSERT_NEAR(uZ, expecteduZ, 0.001);

	double geographicLat;
	double geographicLon;
	double geographicRad;

	// test getGeographic
	testGeo->getGeographic(&geographicLat, &geographicLon, &geographicRad);

	// check results
	// check lat
	double expectedGeographicLatitude = GEOGRAPHICLAT;
	ASSERT_NEAR(geographicLat, expectedGeographicLatitude, 0.001);

	// check lon
	double expectedGeographicLongitude = GEOGRAPHICLON;
	ASSERT_NEAR(geographicLon, expectedGeographicLongitude, 0.001);

	// check radius
	double expectedGeographicRadius = GEOGRAPHICRAD;
	ASSERT_NEAR(geographicRad, expectedGeographicRadius, 0.001);
}

TEST(GeoTest, Geocentric) {
	glassutil::CLogit::disable();

	// construct a hypo
	glassutil::CGeo * testGeo = new glassutil::CGeo();

	testGeo->setGeocentric(GEOCENTRICCLAT, GEOCENTRICLON, GEOCENTRICRAD);

	// check results
	// check lat
	double latitude = testGeo->dLat;
	double expectedLatitude = GEOCENTRICCLAT;
	ASSERT_NEAR(latitude, expectedLatitude, 0.001);

	// check lon
	double longitude = testGeo->dLon;
	double expectedLongitude = GEOCENTRICLON;
	ASSERT_NEAR(longitude, expectedLongitude, 0.001);

	// check radius
	double radius = testGeo->dRad;
	double expectedRadius = GEOCENTRICRAD;
	ASSERT_NEAR(radius, expectedRadius, 0.001);

	// check dX
	double x = testGeo->dX;
	double expectedX = CARTX;
	ASSERT_NEAR(x, expectedX, 0.001);

	// check dY
	double y = testGeo->dY;
	double expectedY = CARTY;
	ASSERT_NEAR(y, expectedY, 0.001);

	// check dZ
	double z = testGeo->dZ;
	double expectedZ = CARTZ;
	ASSERT_NEAR(z, expectedZ, 0.001);

	// check uX
	double uX = testGeo->uX;
	double expecteduX = UNITX;
	ASSERT_NEAR(uX, expecteduX, 0.001);

	// check uY
	double uY = testGeo->uY;
	double expecteduY = UNITY;
	ASSERT_NEAR(uY, expecteduY, 0.001);

	// check uZ
	double uZ = testGeo->uZ;
	double expecteduZ = UNITZ;
	ASSERT_NEAR(uZ, expecteduZ, 0.001);

	double geocentricLat;
	double geocentricLon;
	double geocentricRad;

	// test getGeocentric
	testGeo->getGeocentric(&geocentricLat, &geocentricLon, &geocentricRad);

	// check results
	// check lat
	double expectedeocentricLatitude = GEOCENTRICCLAT;
	ASSERT_NEAR(geocentricLat, expectedeocentricLatitude, 0.001);

	// check lon
	double expectedeocentricLongitude = GEOCENTRICLON;
	ASSERT_NEAR(geocentricLon, expectedeocentricLongitude, 0.001);

	// check radius
	double expectedeocentricRadius = GEOCENTRICRAD;
	ASSERT_NEAR(geocentricRad, expectedeocentricRadius, 0.001);
}

TEST(GeoTest, Cartesian) {
	glassutil::CLogit::disable();

	// construct a hypo
	glassutil::CGeo * testGeo = new glassutil::CGeo();

	testGeo->setCart(CARTX, CARTY, CARTZ);

	// check results
	// check lat
	double latitude = testGeo->dLat;
	double expectedLatitude = GEOCENTRICCLAT;
	ASSERT_NEAR(latitude, expectedLatitude, 0.001);

	// check lon
	double longitude = testGeo->dLon;
	double expectedLongitude = GEOCENTRICLON;
	ASSERT_NEAR(longitude, expectedLongitude, 0.001);

	// check radius
	double radius = testGeo->dRad;
	double expectedRadius = GEOCENTRICRAD;
	ASSERT_NEAR(radius, expectedRadius, 0.001);

	// check dX
	double x = testGeo->dX;
	double expectedX = CARTX;
	ASSERT_NEAR(x, expectedX, 0.001);

	// check dY
	double y = testGeo->dY;
	double expectedY = CARTY;
	ASSERT_NEAR(y, expectedY, 0.001);

	// check dZ
	double z = testGeo->dZ;
	double expectedZ = CARTZ;
	ASSERT_NEAR(z, expectedZ, 0.001);

	// check uX
	double uX = testGeo->uX;
	double expecteduX = UNITX;
	ASSERT_NEAR(uX, expecteduX, 0.001);

	// check uY
	double uY = testGeo->uY;
	double expecteduY = UNITY;
	ASSERT_NEAR(uY, expecteduY, 0.001);

	// check uZ
	double uZ = testGeo->uZ;
	double expecteduZ = UNITZ;
	ASSERT_NEAR(uZ, expecteduZ, 0.001);
}

TEST(GeoTest, Distance) {
	glassutil::CLogit::disable();

	// construct
	glassutil::CGeo * testGeo = new glassutil::CGeo();
	glassutil::CGeo * testGeo2 = new glassutil::CGeo();

	testGeo->setGeographic(DISTLAT1, DISTLON1, DISTELV1);
	testGeo2->setGeographic(DISTLAT2, DISTLON2, DISTELV2);

	double distance = testGeo->delta(testGeo2) / DEG2RAD;

	// check result
	double expectedDistance = DISTANCE;
	ASSERT_NEAR(distance, expectedDistance, 0.001);

	double distance2 = testGeo2->delta(testGeo) / DEG2RAD;

	// check result
	ASSERT_NEAR(distance2, expectedDistance, 0.001);
}

TEST(GeoTest, Azimuth) {
	glassutil::CLogit::disable();

	// construct
	glassutil::CGeo * testGeo = new glassutil::CGeo();
	glassutil::CGeo * testGeo2 = new glassutil::CGeo();

	testGeo->setGeographic(DISTLAT1, DISTLON1, DISTELV1);
	testGeo2->setGeographic(DISTLAT2, DISTLON2, DISTELV2);

	double azimuth = testGeo->azimuth(testGeo2) / DEG2RAD;

	// check result
	double expectedAzimuth = AZIMUTH;
	ASSERT_NEAR(azimuth, expectedAzimuth, 0.001);
}
