#include <geo.h>

#include <cmath>

namespace glass3 {
namespace util {

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
	m_dGeocentricLatitude = RAD2DEG * atan(0.993277 * tan(DEG2RAD * lat));

	// longitude wrap check
	if (lon > 180.0) {
		// dLon is greater than 180
		m_dGeocentricLongitude = lon - 360.0;
	} else if (lon < -180.0) {
		// dLon is less than -180
		m_dGeocentricLongitude = lon + 360.0;
	} else {
		m_dGeocentricLongitude = lon;
	}

	m_dGeocentricRadius = r;

	// compute Cartesian coordinates
	m_dCartesianZ = r * sin(DEG2RAD * m_dGeocentricLatitude);
	double rxy = r * cos(DEG2RAD * m_dGeocentricLatitude);
	m_dCartesianX = rxy * cos(DEG2RAD * m_dGeocentricLongitude);
	m_dCartesianY = rxy * sin(DEG2RAD * m_dGeocentricLongitude);

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
	if (lon > 180.0) {
		// dLon is greater than 180
		m_dGeocentricLongitude = lon - 360.0;
	} else if (lon < -180.0) {
		// dLon is less than -180
		m_dGeocentricLongitude = lon + 360.0;
	} else {
		m_dGeocentricLongitude = lon;
	}

	m_dGeocentricRadius = r;

	// compute Cartesian coordinates
	m_dCartesianZ = r * sin(DEG2RAD * m_dGeocentricLatitude);
	double rxy = r * cos(DEG2RAD * m_dGeocentricLatitude);
	m_dCartesianX = rxy * cos(DEG2RAD * m_dGeocentricLongitude);
	m_dCartesianY = rxy * sin(DEG2RAD * m_dGeocentricLongitude);

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
	m_dGeocentricLatitude = RAD2DEG * atan2(z, rxy);
	m_dGeocentricLongitude = RAD2DEG * atan2(y, x);

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
	*lat = RAD2DEG * atan(tan(DEG2RAD * m_dGeocentricLatitude) / 0.993277);

	// longitude wrap check
	if (m_dGeocentricLongitude > 180.0) {
		// dLon is greater than 180
		*lon = m_dGeocentricLongitude - 360.0;
	} else if (m_dGeocentricLongitude < -180.0) {
		// dLon is less than -180
		*lon = m_dGeocentricLongitude + 360.0;
	} else {
		*lon = m_dGeocentricLongitude;
	}

	// r unchanged
	*r = m_dGeocentricRadius;
}

void Geo::getGeocentric(double *lat, double *lon, double *r) {
	*lat = m_dGeocentricLatitude;

	// longitude wrap check
	if (m_dGeocentricLongitude > 180.0) {
		// dLon is greater than 180
		*lon = m_dGeocentricLongitude - 360.0;
	} else if (m_dGeocentricLongitude < -180.0) {
		// dLon is less than -180
		*lon = m_dGeocentricLongitude + 360.0;
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
	double sx = cos(DEG2RAD * geo->m_dGeocentricLatitude)
			* cos(DEG2RAD * geo->m_dGeocentricLongitude);
	double sy = cos(DEG2RAD * geo->m_dGeocentricLatitude)
			* sin(DEG2RAD * geo->m_dGeocentricLongitude);
	double sz = sin(DEG2RAD * geo->m_dGeocentricLatitude);

	// Quake radial normal vector
	double qx = cos(DEG2RAD * m_dGeocentricLatitude)
			* cos(DEG2RAD * m_dGeocentricLongitude);
	double qy = cos(DEG2RAD * m_dGeocentricLatitude)
			* sin(DEG2RAD * m_dGeocentricLongitude);
	double qz = sin(DEG2RAD * m_dGeocentricLatitude);

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
	double nx = -sin(DEG2RAD * m_dGeocentricLatitude)
			* cos(DEG2RAD * m_dGeocentricLongitude);
	double ny = -sin(DEG2RAD * m_dGeocentricLatitude)
			* sin(DEG2RAD * m_dGeocentricLongitude);
	double nz = cos(DEG2RAD * m_dGeocentricLatitude);

	// East tangent vector
	double ex = -sin(DEG2RAD * m_dGeocentricLongitude);
	double ey = cos(DEG2RAD * m_dGeocentricLongitude);
	double ez = 0.0;
	double n = ax * nx + ay * ny + az * nz;
	double e = ax * ex + ay * ey + az * ez;

	// compute azimuth
	double azm = atan2(e, n);
	if (azm < 0.0) {
		azm += TWOPI;
	}
	return (azm);
}
}  // namespace util
}  // namespace glass3
