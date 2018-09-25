#include <gtest/gtest.h>
#include "geo.h"
#include "logger.h"

#define GEOCENTRICCLAT 41.921198129865765
#define GEOCENTRICLON -122.56219953573087
#define GEOCENTRICRAD 3342.0
#define CARTX -1338.358747830255
#define CARTY -2095.778248528537
#define CARTZ 2232.8173671621444
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
	glass3::util::Logger::disable();

	// construct a hypo
	glass3::util::Geo * testGeo = new glass3::util::Geo();

	// assert default values
	ASSERT_EQ(0, testGeo->m_dGeocentricLatitude)<< "m_dGeocentricLatitude is zero";
	ASSERT_EQ(0, testGeo->m_dGeocentricLongitude)<< "m_dGeocentricLongitude is zero";
	ASSERT_EQ(0, testGeo->m_dGeocentricRadius)<< "m_dGeocentricRadius is zero";
	ASSERT_EQ(0, testGeo->m_dCartesianX)<< "m_dCartesianX is zero";
	ASSERT_EQ(0, testGeo->m_dCartesianY)<< "m_dCartesianY is zero";
	ASSERT_EQ(0, testGeo->m_dCartesianZ)<< "m_dCartesianZ is zero";
	ASSERT_EQ(0, testGeo->m_dUnitVectorX)<< "m_dUnitVectorX is zero";
	ASSERT_EQ(0, testGeo->m_dUnitVectorY)<< "m_dUnitVectorY is zero";
	ASSERT_EQ(0, testGeo->m_dUnitVectorZ)<< "m_dUnitVectorZ is zero";

	// now init
	testGeo->initialize(GEOCENTRICCLAT, GEOCENTRICLON, GEOCENTRICRAD, CARTX,
	CARTY,
						CARTZ, UNITX, UNITY, UNITZ);

	// check results
	// check lat
	double latitude = testGeo->m_dGeocentricLatitude;
	double expectedGeocentricLatitudeitude = GEOCENTRICCLAT;
	ASSERT_NEAR(latitude, expectedGeocentricLatitudeitude, 0.001);

	// check lon
	double longitude = testGeo->m_dGeocentricLongitude;
	double expectedGeocentricLongitudegitude = GEOCENTRICLON;
	ASSERT_NEAR(longitude, expectedGeocentricLongitudegitude, 0.001);

	// check radius
	double radius = testGeo->m_dGeocentricRadius;
	double expectedGeocentricRadiusius = GEOCENTRICRAD;
	ASSERT_NEAR(radius, expectedGeocentricRadiusius, 0.001);

	// check m_dCartesianX
	double CartesianX = testGeo->m_dCartesianX;
	double expectedCartesianX = CARTX;
	ASSERT_NEAR(CartesianX, expectedCartesianX, 0.001);

	// check m_dCartesianY
	double CartesianY = testGeo->m_dCartesianY;
	double expectedCartesianY = CARTY;
	ASSERT_NEAR(CartesianY, expectedCartesianY, 0.001);

	// check m_dCartesianZ
	double CartesianZ = testGeo->m_dCartesianZ;
	double expectedCartesianZ = CARTZ;
	ASSERT_NEAR(CartesianZ, expectedCartesianZ, 0.001);

	// check m_dUnitVectorX
	double m_dUnitVectorX = testGeo->m_dUnitVectorX;
	double expectedm_dUnitVectorX = UNITX;
	ASSERT_NEAR(m_dUnitVectorX, expectedm_dUnitVectorX, 0.001);

	// check m_dUnitVectorY
	double m_dUnitVectorY = testGeo->m_dUnitVectorY;
	double expectedm_dUnitVectorY = UNITY;
	ASSERT_NEAR(m_dUnitVectorY, expectedm_dUnitVectorY, 0.001);

	// check m_dUnitVectorZ
	double m_dUnitVectorZ = testGeo->m_dUnitVectorZ;
	double expectedm_dUnitVectorZ = UNITZ;
	ASSERT_NEAR(m_dUnitVectorZ, expectedm_dUnitVectorZ, 0.001);

	// now clear
	testGeo->clear();

	// assert default values
	ASSERT_EQ(0, testGeo->m_dGeocentricLatitude)<< "m_dGeocentricLatitude is zero";
	ASSERT_EQ(0, testGeo->m_dGeocentricLongitude)<< "m_dGeocentricLongitude is zero";
	ASSERT_EQ(0, testGeo->m_dGeocentricRadius)<< "m_dGeocentricRadius is zero";
	ASSERT_EQ(0, testGeo->m_dCartesianX)<< "m_dCartesianX is zero";
	ASSERT_EQ(0, testGeo->m_dCartesianY)<< "m_dCartesianY is zero";
	ASSERT_EQ(0, testGeo->m_dCartesianZ)<< "m_dCartesianZ is zero";
	ASSERT_EQ(0, testGeo->m_dUnitVectorX)<< "m_dUnitVectorX is zero";
	ASSERT_EQ(0, testGeo->m_dUnitVectorY)<< "m_dUnitVectorY is zero";
	ASSERT_EQ(0, testGeo->m_dUnitVectorZ)<< "m_dUnitVectorZ is zero";
}

TEST(GeoTest, Geographic) {
	glass3::util::Logger::disable();

	// construct a hypo
	glass3::util::Geo * testGeo = new glass3::util::Geo();

	testGeo->setGeographic(GEOGRAPHICLAT, GEOGRAPHICLON, GEOGRAPHICRAD);

	// check results
	// check lat
	double latitude = testGeo->m_dGeocentricLatitude;
	double expectedGeocentricLatitudeitude = GEOCENTRICCLAT;
	ASSERT_NEAR(latitude, expectedGeocentricLatitudeitude, 0.001);

	// check lon
	double longitude = testGeo->m_dGeocentricLongitude;
	double expectedGeocentricLongitudegitude = GEOCENTRICLON;
	ASSERT_NEAR(longitude, expectedGeocentricLongitudegitude, 0.001);

	// check radius
	double radius = testGeo->m_dGeocentricRadius;
	double expectedGeocentricRadiusius = GEOCENTRICRAD;
	ASSERT_NEAR(radius, expectedGeocentricRadiusius, 0.001);

	// check m_dCartesianX
	double CartesianX = testGeo->m_dCartesianX;
	double expectedCartesianX = CARTX;
	ASSERT_NEAR(CartesianX, expectedCartesianX, 0.001);

	// check m_dCartesianY
	double CartesianY = testGeo->m_dCartesianY;
	double expectedCartesianY = CARTY;
	ASSERT_NEAR(CartesianY, expectedCartesianY, 0.001);

	// check m_dCartesianZ
	double CartesianZ = testGeo->m_dCartesianZ;
	double expectedCartesianZ = CARTZ;
	ASSERT_NEAR(CartesianZ, expectedCartesianZ, 0.001);

	// check m_dUnitVectorX
	double m_dUnitVectorX = testGeo->m_dUnitVectorX;
	double expectedm_dUnitVectorX = UNITX;
	ASSERT_NEAR(m_dUnitVectorX, expectedm_dUnitVectorX, 0.001);

	// check m_dUnitVectorY
	double m_dUnitVectorY = testGeo->m_dUnitVectorY;
	double expectedm_dUnitVectorY = UNITY;
	ASSERT_NEAR(m_dUnitVectorY, expectedm_dUnitVectorY, 0.001);

	// check m_dUnitVectorZ
	double m_dUnitVectorZ = testGeo->m_dUnitVectorZ;
	double expectedm_dUnitVectorZ = UNITZ;
	ASSERT_NEAR(m_dUnitVectorZ, expectedm_dUnitVectorZ, 0.001);

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
	glass3::util::Logger::disable();

	// construct a hypo
	glass3::util::Geo * testGeo = new glass3::util::Geo();

	testGeo->setGeocentric(GEOCENTRICCLAT, GEOCENTRICLON, GEOCENTRICRAD);

	// check results
	// check lat
	double latitude = testGeo->m_dGeocentricLatitude;
	double expectedGeocentricLatitudeitude = GEOCENTRICCLAT;
	ASSERT_NEAR(latitude, expectedGeocentricLatitudeitude, 0.001);

	// check lon
	double longitude = testGeo->m_dGeocentricLongitude;
	double expectedGeocentricLongitudegitude = GEOCENTRICLON;
	ASSERT_NEAR(longitude, expectedGeocentricLongitudegitude, 0.001);

	// check radius
	double radius = testGeo->m_dGeocentricRadius;
	double expectedGeocentricRadiusius = GEOCENTRICRAD;
	ASSERT_NEAR(radius, expectedGeocentricRadiusius, 0.001);

	// check m_dCartesianX
	double CartesianX = testGeo->m_dCartesianX;
	double expectedCartesianX = CARTX;
	ASSERT_NEAR(CartesianX, expectedCartesianX, 0.001);

	// check m_dCartesianY
	double CartesianY = testGeo->m_dCartesianY;
	double expectedCartesianY = CARTY;
	ASSERT_NEAR(CartesianY, expectedCartesianY, 0.001);

	// check m_dCartesianZ
	double CartesianZ = testGeo->m_dCartesianZ;
	double expectedCartesianZ = CARTZ;
	ASSERT_NEAR(CartesianZ, expectedCartesianZ, 0.001);

	// check m_dUnitVectorX
	double m_dUnitVectorX = testGeo->m_dUnitVectorX;
	double expectedm_dUnitVectorX = UNITX;
	ASSERT_NEAR(m_dUnitVectorX, expectedm_dUnitVectorX, 0.001);

	// check m_dUnitVectorY
	double m_dUnitVectorY = testGeo->m_dUnitVectorY;
	double expectedm_dUnitVectorY = UNITY;
	ASSERT_NEAR(m_dUnitVectorY, expectedm_dUnitVectorY, 0.001);

	// check m_dUnitVectorZ
	double m_dUnitVectorZ = testGeo->m_dUnitVectorZ;
	double expectedm_dUnitVectorZ = UNITZ;
	ASSERT_NEAR(m_dUnitVectorZ, expectedm_dUnitVectorZ, 0.001);

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
	glass3::util::Logger::disable();

	// construct a hypo
	glass3::util::Geo * testGeo = new glass3::util::Geo();

	testGeo->setCartesian(CARTX, CARTY, CARTZ);

	// check results
	// check lat
	double latitude = testGeo->m_dGeocentricLatitude;
	double expectedGeocentricLatitudeitude = GEOCENTRICCLAT;
	ASSERT_NEAR(latitude, expectedGeocentricLatitudeitude, 0.001);

	// check lon
	double longitude = testGeo->m_dGeocentricLongitude;
	double expectedGeocentricLongitudegitude = GEOCENTRICLON;
	ASSERT_NEAR(longitude, expectedGeocentricLongitudegitude, 0.001);

	// check radius
	double radius = testGeo->m_dGeocentricRadius;
	double expectedGeocentricRadiusius = GEOCENTRICRAD;
	ASSERT_NEAR(radius, expectedGeocentricRadiusius, 0.001);

	// check m_dCartesianX
	double CartesianX = testGeo->m_dCartesianX;
	double expectedCartesianX = CARTX;
	ASSERT_NEAR(CartesianX, expectedCartesianX, 0.001);

	// check m_dCartesianY
	double CartesianY = testGeo->m_dCartesianY;
	double expectedCartesianY = CARTY;
	ASSERT_NEAR(CartesianY, expectedCartesianY, 0.001);

	// check m_dCartesianZ
	double CartesianZ = testGeo->m_dCartesianZ;
	double expectedCartesianZ = CARTZ;
	ASSERT_NEAR(CartesianZ, expectedCartesianZ, 0.001);

	// check m_dUnitVectorX
	double unitVectorX = testGeo->m_dUnitVectorX;
	double expectedUnitVectorX = UNITX;
	ASSERT_NEAR(unitVectorX, expectedUnitVectorX, 0.001);

	// check m_dUnitVectorY
	double unitVectorY = testGeo->m_dUnitVectorY;
	double expectedUnitVectorY = UNITY;
	ASSERT_NEAR(unitVectorY, expectedUnitVectorY, 0.001);

	// check m_dUnitVectorZ
	double unitVectorZ = testGeo->m_dUnitVectorZ;
	double expectedUnitVectorZ = UNITZ;
	ASSERT_NEAR(unitVectorZ, expectedUnitVectorZ, 0.001);
}

TEST(GeoTest, Distance) {
	glass3::util::Logger::disable();

	// construct
	glass3::util::Geo * testGeo = new glass3::util::Geo();
	glass3::util::Geo * testGeo2 = new glass3::util::Geo();

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
	glass3::util::Logger::disable();

	// construct
	glass3::util::Geo * testGeo = new glass3::util::Geo();
	glass3::util::Geo * testGeo2 = new glass3::util::Geo();

	testGeo->setGeographic(DISTLAT1, DISTLON1, DISTELV1);
	testGeo2->setGeographic(DISTLAT2, DISTLON2, DISTELV2);

	double azimuth = testGeo->azimuth(testGeo2) / DEG2RAD;

	// check result
	double expectedAzimuth = AZIMUTH;
	ASSERT_NEAR(azimuth, expectedAzimuth, 0.001);
}
