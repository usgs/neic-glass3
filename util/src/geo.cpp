#include <geo.h>
#include <cmath>

namespace glass3 {
namespace util {

// Geographic constants
constexpr double Geo::k_EarthRadiusKm;
constexpr double Geo::k_DegreesToKm;
constexpr double Geo::k_KmToDegrees;
constexpr double Geo::k_MaximumLatitude;
constexpr double Geo::k_MinimumLatitude;
constexpr double Geo::k_MaximumLongitude;
constexpr double Geo::k_MinimumLongitude;
constexpr double Geo::k_LongitudeWrap;
constexpr double Geo::k_GeographicToGeocentric;
constexpr double Geo::k_dMetersToKm;
constexpr double Geo::k_dElevationToDepth;

Geo::Geo() {
	clear();
}

Geo::Geo(double lat, double lon, double rad, double cartX, double cartY,
			double cartZ, double unitX, double unitY, double unitZ) {
	initialize(lat, lon, rad, cartX, cartY, cartZ, unitX, unitY, unitZ);
}

Geo::Geo(Geo *geo) {
	clear();

	clone(geo);
}

Geo::~Geo() {
}

void Geo::clone(Geo *geo) {
	initialize(geo->m_dGeocentricLatitude, geo->m_dGeocentricLongitude,
				geo->m_dGeocentricRadius, geo->m_dCartesianX,
				geo->m_dCartesianY, geo->m_dCartesianZ, geo->m_dUnitVectorX,
				geo->m_dUnitVectorY, geo->m_dUnitVectorZ);
}

void Geo::clear() {
	m_dGeocentricLatitude = 0;
	m_dGeocentricLongitude = 0;
	m_dGeocentricRadius = 0;
	m_dCartesianX = 0;
	m_dCartesianY = 0;
	m_dCartesianZ = 0;
	m_dUnitVectorX = 0;
	m_dUnitVectorY = 0;
	m_dUnitVectorZ = 0;
}

void Geo::initialize(double lat, double lon, double rad, double cartX,
						double cartY, double cartZ, double unitX, double unitY,
						double unitZ) {
	m_dGeocentricLatitude = lat;
	m_dGeocentricLongitude = lon;
	m_dGeocentricRadius = rad;
	m_dCartesianX = cartX;
	m_dCartesianY = cartY;
	m_dCartesianZ = cartZ;
	m_dUnitVectorX = unitX;
	m_dUnitVectorY = unitY;
	m_dUnitVectorZ = unitZ;
}

// Converts geographic latitude into geocentric latitude.
void Geo::setGeographic(double lat, double lon, double r) {
	// convert latitude
	m_dGeocentricLatitude = GlassMath::k_RadiansToDegrees
			* atan(k_GeographicToGeocentric
					* tan(GlassMath::k_DegreesToRadians * lat));

	// longitude wrap check
	if (lon > k_MaximumLongitude) {
		// dLon is greater than 180
		m_dGeocentricLongitude = lon - k_LongitudeWrap;
	} else if (lon < k_MinimumLongitude) {
		// dLon is less than -180
		m_dGeocentricLongitude = lon + k_LongitudeWrap;
	} else {
		m_dGeocentricLongitude = lon;
	}

	m_dGeocentricRadius = r;

	// compute Cartesian coordinates
	m_dCartesianZ = r
			* sin(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude);
	double rxy = r * cos(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude);
	m_dCartesianX = rxy
			* cos(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);
	m_dCartesianY = rxy
			* sin(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);

	// compute unit vectors
	double rr = sqrt(
			m_dCartesianX * m_dCartesianX + m_dCartesianY * m_dCartesianY
					+ m_dCartesianZ * m_dCartesianZ);
	m_dUnitVectorX = m_dCartesianX / rr;
	m_dUnitVectorY = m_dCartesianY / rr;
	m_dUnitVectorZ = m_dCartesianZ / rr;
}

// Initialize geocentric coordinates
void Geo::setGeocentric(double lat, double lon, double r) {
	m_dGeocentricLatitude = lat;

	// longitude wrap check
	if (lon > k_MaximumLongitude) {
		// dLon is greater than 180
		m_dGeocentricLongitude = lon - k_LongitudeWrap;
	} else if (lon < k_MinimumLongitude) {
		// dLon is less than -180
		m_dGeocentricLongitude = lon + k_LongitudeWrap;
	} else {
		m_dGeocentricLongitude = lon;
	}

	m_dGeocentricRadius = r;

	// compute Cartesian coordinates
	m_dCartesianZ = r
			* sin(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude);
	double rxy = r * cos(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude);
	m_dCartesianX = rxy
			* cos(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);
	m_dCartesianY = rxy
			* sin(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);

	// compute unit vectors
	double rr = sqrt(
			m_dCartesianX * m_dCartesianX + m_dCartesianY * m_dCartesianY
					+ m_dCartesianZ * m_dCartesianZ);
	m_dUnitVectorX = m_dCartesianX / rr;
	m_dUnitVectorY = m_dCartesianY / rr;
	m_dUnitVectorZ = m_dCartesianZ / rr;
}

void Geo::setCartesian(double x, double y, double z) {
	m_dCartesianX = x;
	m_dCartesianY = y;
	m_dCartesianZ = z;

	// computer geocentric coordinates
	m_dGeocentricRadius = sqrt(x * x + y * y + z * z);
	double rxy = sqrt(x * x + y * y);
	m_dGeocentricLatitude = GlassMath::k_RadiansToDegrees * atan2(z, rxy);
	m_dGeocentricLongitude = GlassMath::k_RadiansToDegrees * atan2(y, x);

	// compute unit vectors
	double rr = sqrt(
			m_dCartesianX * m_dCartesianX + m_dCartesianY * m_dCartesianY
					+ m_dCartesianZ * m_dCartesianZ);
	m_dUnitVectorX = m_dCartesianX / rr;
	m_dUnitVectorY = m_dCartesianY / rr;
	m_dUnitVectorZ = m_dCartesianZ / rr;
}

void Geo::getGeographic(double *lat, double *lon, double *r) {
	// convert latitude
	*lat = GlassMath::k_RadiansToDegrees
			* atan(tan(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude)
					/ k_GeographicToGeocentric);

	// longitude wrap check
	if (m_dGeocentricLongitude > k_MaximumLongitude) {
		// dLon is greater than 180
		*lon = m_dGeocentricLongitude - k_LongitudeWrap;
	} else if (m_dGeocentricLongitude < k_MinimumLongitude) {
		// dLon is less than -180
		*lon = m_dGeocentricLongitude + k_LongitudeWrap;
	} else {
		*lon = m_dGeocentricLongitude;
	}

	// r unchanged
	*r = m_dGeocentricRadius;
}

void Geo::getGeocentric(double *lat, double *lon, double *r) {
	*lat = m_dGeocentricLatitude;

	// longitude wrap check
	if (m_dGeocentricLongitude > k_MaximumLongitude) {
		// dLon is greater than 180
		*lon = m_dGeocentricLongitude - k_LongitudeWrap;
	} else if (m_dGeocentricLongitude < k_MinimumLongitude) {
		// dLon is less than -180
		*lon = m_dGeocentricLongitude + k_LongitudeWrap;
	} else {
		*lon = m_dGeocentricLongitude;
	}

	*r = m_dGeocentricRadius;
}

// Calculate the distance in radians to a given geographic object
double Geo::delta(Geo *geo) {
	// compute dot product
	double dot = m_dUnitVectorX * geo->m_dUnitVectorX
			+ m_dUnitVectorY * geo->m_dUnitVectorY
			+ m_dUnitVectorZ * geo->m_dUnitVectorZ;

	// use dot product to compute distance
	double dlt;
	if (dot < 1.0) {
		dlt = acos(dot);
	} else {
		dlt = 0.0;
	}
	return (dlt);
}

// Calculate the azimuth in radians to a given geographic object
double Geo::azimuth(Geo *geo) {
	// Station radial normal vector
	double sx = cos(GlassMath::k_DegreesToRadians * geo->m_dGeocentricLatitude)
			* cos(GlassMath::k_DegreesToRadians * geo->m_dGeocentricLongitude);
	double sy = cos(GlassMath::k_DegreesToRadians * geo->m_dGeocentricLatitude)
			* sin(GlassMath::k_DegreesToRadians * geo->m_dGeocentricLongitude);
	double sz = sin(GlassMath::k_DegreesToRadians * geo->m_dGeocentricLatitude);

	// Quake radial normal vector
	double qx = cos(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude)
			* cos(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);
	double qy = cos(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude)
			* sin(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);
	double qz = sin(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude);

	// Normal to great circle
	double qsx = qy * sz - sy * qz;
	double qsy = qz * sx - sz * qx;
	double qsz = qx * sy - sx * qy;

	// Vector points along great circle
	double ax = qsy * qz - qy * qsz;
	double ay = qsz * qx - qz * qsx;
	double az = qsx * qy - qx * qsy;
	double r = sqrt(ax * ax + ay * ay + az * az);
	ax /= r;
	ay /= r;
	az /= r;

	// North tangent vector
	double nx = -sin(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude)
			* cos(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);
	double ny = -sin(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude)
			* sin(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);
	double nz = cos(GlassMath::k_DegreesToRadians * m_dGeocentricLatitude);

	// East tangent vector
	double ex = -sin(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);
	double ey = cos(GlassMath::k_DegreesToRadians * m_dGeocentricLongitude);
	double ez = 0.0;
	double n = ax * nx + ay * ny + az * nz;
	double e = ax * ex + ay * ey + az * ez;

	// compute azimuth
	double azm = atan2(e, n);
	if (azm < 0.0) {
		azm += GlassMath::k_TwoPi;
	}
	return (azm);
}
}  // namespace util
}  // namespace glass3
