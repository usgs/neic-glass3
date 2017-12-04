#include <cmath>
#include "Geo.h"

namespace glassutil {

CGeo::CGeo() {
	clear();
}

CGeo::CGeo(double lat, double lon, double rad, double cartX, double cartY,
			double cartZ, double cartT, int tag, double unitX, double unitY,
			double unitZ) {
	initialize(lat, lon, rad, cartX, cartY, cartZ, cartT, tag, unitX, unitY,
				unitZ);
}

CGeo::CGeo(CGeo *geo) {
	clear();

	clone(geo);
}

CGeo::~CGeo() {
}

void CGeo::clone(CGeo *geo) {
	initialize(geo->dLat, geo->dLon, geo->dRad, geo->dX, geo->dY, geo->dZ,
				geo->dT, geo->iTag, geo->uX, geo->uY, geo->uZ);
}

void CGeo::clear() {
	dLat = 0;
	dLon = 0;
	dRad = 0;
	dX = 0;
	dY = 0;
	dZ = 0;
	dT = 0;
	iTag = 0;
	uX = 0;
	uY = 0;
	uZ = 0;
}

void CGeo::initialize(double lat, double lon, double rad, double cartX,
						double cartY, double cartZ, double cartT, int tag,
						double unitX, double unitY, double unitZ) {
	dLat = lat;
	dLon = lon;
	dRad = rad;
	dX = cartX;
	dY = cartY;
	dZ = cartZ;
	dT = cartT;
	iTag = tag;
	uX = unitX;
	uY = unitY;
	uZ = unitZ;
}

// Converts geographic latitude into geocentric latitude.
void CGeo::setGeographic(double lat, double lon, double r) {
	// convert latitude
	dLat = RAD2DEG * atan(0.993277 * tan(DEG2RAD * lat));
	dLon = lon;
	dRad = r;

	// compute Cartesian coordinates
	dZ = r * sin(DEG2RAD * dLat);
	double rxy = r * cos(DEG2RAD * dLat);
	dX = rxy * cos(DEG2RAD * dLon);
	dY = rxy * sin(DEG2RAD * dLon);

	// compute unit vectors
	double rr = sqrt(dX * dX + dY * dY + dZ * dZ);
	uX = dX / rr;
	uY = dY / rr;
	uZ = dZ / rr;
}

// Initialize geocentric coordinates
void CGeo::setGeocentric(double lat, double lon, double r) {
	dLat = lat;
	dLon = lon;
	dRad = r;

	// compute Cartesian coordinates
	dZ = r * sin(DEG2RAD * dLat);
	double rxy = r * cos(DEG2RAD * dLat);
	dX = rxy * cos(DEG2RAD * dLon);
	dY = rxy * sin(DEG2RAD * dLon);

	// compute unit vectors
	double rr = sqrt(dX * dX + dY * dY + dZ * dZ);
	uX = dX / rr;
	uY = dY / rr;
	uZ = dZ / rr;
}

void CGeo::setCart(double x, double y, double z) {
	dX = x;
	dY = y;
	dZ = z;

	// computer geocentric coordinates
	dRad = sqrt(x * x + y * y + z * z);
	double rxy = sqrt(x * x + y * y);
	dLat = RAD2DEG * atan2(z, rxy);
	dLon = RAD2DEG * atan2(y, x);

	// compute unit vectors
	double rr = sqrt(dX * dX + dY * dY + dZ * dZ);
	uX = dX / rr;
	uY = dY / rr;
	uZ = dZ / rr;
}

void CGeo::getGeographic(double *lat, double *lon, double *r) {
	// convert latitude
	*lat = RAD2DEG * atan(tan(DEG2RAD * dLat) / 0.993277);

	// longitude wrap check
	if (dLon > 180.0) {
		// dLon is greater than 180
		*lon = dLon - 360.0;
	} else if (dLon < -180.0) {
		// dLon is less than -180
		*lon = dLon + 360.0;
	} else {
		*lon = dLon;
	}

	// r unchanged
	*r = dRad;
}

void CGeo::getGeocentric(double *lat, double *lon, double *r) {
	*lat = dLat;
	*lon = dLon;
	*r = dRad;
}

// Calculate the distance in radians to a given geographic object
double CGeo::delta(CGeo *geo) {
	// compute dot product
	double dot = uX * geo->uX + uY * geo->uY + uZ * geo->uZ;

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
double CGeo::azimuth(CGeo *geo) {
	// Station radial normal vector
	double sx = cos(DEG2RAD * geo->dLat) * cos(DEG2RAD * geo->dLon);
	double sy = cos(DEG2RAD * geo->dLat) * sin(DEG2RAD * geo->dLon);
	double sz = sin(DEG2RAD * geo->dLat);

	// Quake radial normal vector
	double qx = cos(DEG2RAD * dLat) * cos(DEG2RAD * dLon);
	double qy = cos(DEG2RAD * dLat) * sin(DEG2RAD * dLon);
	double qz = sin(DEG2RAD * dLat);

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
	double nx = -sin(DEG2RAD * dLat) * cos(DEG2RAD * dLon);
	double ny = -sin(DEG2RAD * dLat) * sin(DEG2RAD * dLon);
	double nz = cos(DEG2RAD * dLat);

	// East tangent vector
	double ex = -sin(DEG2RAD * dLon);
	double ey = cos(DEG2RAD * dLon);
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
}  // namespace glassutil
