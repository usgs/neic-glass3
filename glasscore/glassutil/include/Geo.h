/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef GEO_H
#define GEO_H

/**
 * \namespace glassutil
 * \brief glassutil namespace general glass utility functions
 *
 * The glassutil namespace contains various classes and functions used
 * in both the traveltime and glasslib libraries
 */
namespace glassutil {

// mathmatical defines
#define RAD2DEG 57.29577951308
#define DEG2RAD	0.01745329251994
#define TWOPI 6.283185307179586
#define PI 3.14159265359

// geographic defines
#define EARTHRADIUSKM 6371.0  // average
#define DEG2KM 111.19  // based on 6371 as the average radius
#define KM2DEG 0.00899  // based on 6371 as the average radius
/**
 * \brief glassutil geographic coordinate conversion class
 *
 * The glassutil CGeo class is a class that encapsulates the
 * logic necessary to convert geographic, geocentric,
 * and Cartesian coordinates to geographic and geocentric
 * coordinates.  The class also computes the distance and
 * azimuth between coordinates, and intersections between
 * geographic (or geocentric) spheres.
 */
class CGeo {
 public:
	/**
	 * \brief CGeo constructor
	 *
	 * The constructor for the CGeo class.
	 */
	CGeo();

	/**
	 * \brief CGeo alternate constructor
	 *
	 * The atlernate constructor for the CGeo class.
	 *
	 * \param lat - The geocentric latitude in degrees
	 * \param lon - The geocentric longitude in degrees
	 * \param rad - The geocentric radius in kilometers
	 * \param cartX - The Cartesian x coordinate.
	 * \param cartY - The Cartesian y coordinate.
	 * \param cartZ - The Cartesian z coordinate.
	 * \param cartT - The associated time to the Cartesian point (if CGeo
	 * represents a space-time point) in decimal seconds.
	 * \param tag - an Optional user information tag
	 * \param unitX - The x unit vector
	 * \param unitY - The y unit vector
	 * \param unitZ - The z unit vector
	 */
	CGeo(double lat, double lon, double rad, double cartX, double cartY,
			double cartZ, double cartT, int tag, double unitX, double unitY,
			double unitZ);

	/**
	 * \brief CGeo copy constructor
	 *
	 * The copy constructor for the CGeo class.
	 *
	 * \param geo - A pointer to a CGeo object to copy from
	 */
	explicit CGeo(CGeo *geo);

	/**
	 * \brief CGeo destructor
	 *
	 * The destructor for the CGeo class.
	 */
	virtual ~CGeo();

	/**
	 * \brief CGeo init method
	 *
	 * The initialize method for the CGeo class.
	 *
	 * \param lat - The geocentric latitude in degrees
	 * \param lon - The geocentric longitude in degrees
	 * \param rad - The geocentric radius in kilometers
	 * \param cartX - The Cartesian x coordinate.
	 * \param cartY - The Cartesian y coordinate.
	 * \param cartZ - The Cartesian z coordinate.
	 * \param cartT - The associated time to the Cartesian point (if CGeo
	 * represents a space-time point) in decimal seconds.
	 * \param tag - an Optional user information tag
	 * \param unitX - The x unit vector
	 * \param unitY - The y unit vector
	 * \param unitZ - The z unit vector
	 */
	void initialize(double lat, double lon, double rad, double cartX,
					double cartY, double cartZ, double cartT, int tag,
					double unitX, double unitY, double unitZ);

	/**
	 * \brief CGeo clone method
	 *
	 * The clone method for the CGeo class.
	 *
	 * \param geo - A pointer to a CGeo object to clone from
	 */
	virtual void clone(CGeo *geo);

	/**
	 * \brief CGeo clear function
	 */
	void clear();

	/**
	 * \brief CGeo geographic coordinate set function
	 *
	 * Converts geographic coordinates into geocentric coordinates and
	 * sets CGeo to those geocentric coordinates.  Computes the
	 * Cartesian coordinates and unit vectors.
	 *
	 * \param lat - The geographic latitude in degrees
	 * \param lon - The geographic longitude in degrees
	 * \param r - The geographic radius in kilometers
	 */
	virtual void setGeographic(double lat, double lon, double r);

	/**
	 * \brief CGeo geocentric coordinate set function
	 *
	 * Sets CGeo to the provided geocentric coordinates.
	 * Computes the Cartesian coordinates and unit vectors.
	 *
	 * \param lat - The geocentric latitude in degrees
	 * \param lon - The geocentric longitude in degrees
	 * \param r - The geocentric radius in kilometers
	 */
	virtual void setGeocentric(double lat, double lon, double r);

	/**
	 * \brief CGeo geographic coordinate get function
	 *
	 * Gets the current geographic coordinates from CGeo
	 *
	 * \param lat - A pointer to the geographic latitude variable to populate in
	 * degrees
	 * \param lon - A pointer to the geographic longitude variable to populate
	 * in degrees
	 * \param r - A pointer to the geographic radius variable to populate in
	 * kilometers
	 */
	virtual void getGeographic(double *lat, double *lon, double *r);

	/**
	 * \brief CGeo geographic coordinate get function
	 *
	 * Gets the current geographic coordinates from CGeo.
	 * NOTE: Does not appear to be called by glassutil or glasscore!
	 *
	 * \param lat - A pointer to the geographic latitude variable to populate in
	 * degrees
	 * \param lon - A pointer to the geographic longitude variable to populate
	 * in degrees
	 * \param r - A pointer to the geographic radius variable to populate in
	 * kilometers
	 */
	virtual void getGeocentric(double *lat, double *lon, double *r);

	/**
	 * \brief CGeo Cartesian coordinate set function
	 *
	 * Sets CGeo to the provided Cartesian coordinates.
	 * Computes the geocentric coordinates and unit vectors.
	 * NOTE: Does not appear to be called by glassutil or glasscore!
	 *
	 * \param x - The Cartesian x coordinate
	 * \param y - The Cartesian y coordinate
	 * \param z - The Cartesian z coordinate
	 */
	virtual void setCart(double x, double y, double z);

	/**
	 * \brief Calculate the distance to a given CGeo object
	 *
	 * Computes the distance in radians from this object to a given
	 * CGeo object
	 *
	 * \param geo - A pointer to the CGeo object to calculate distance to
	 * \return Returns the distance in radians between the two CGeo objects.
	 */
	virtual double delta(CGeo *geo);

	/**
	 * \brief Calculate the azimuth to a given CGeo object
	 *
	 * Computes the azimuth in radians from this object to a given
	 * CGeo object
	 *
	 * \param geo - A pointer to the CGeo object to calculate azimuth to
	 * \return Returns the azimuth in radians between the two CGeo objects.
	 */
	virtual double azimuth(CGeo *geo);

	/**
	 * \brief the double value containing the geocentric latitude
	 * in degrees.
	 */
	double dLat;

	/**
	 * \brief the double value containing the geocentric longitude
	 * in degrees.
	 */
	double dLon;

	/**
	 * \brief the double value containing the geocentric radius
	 * in kilometers.
	 */
	double dRad;

	/**
	 * \brief the double value containing the Cartesian x
	 * coordinate.
	 */
	double dX;

	/**
	 * \brief the double value containing the Cartesian y
	 * coordinate.
	 */
	double dY;

	/**
	 * \brief the double value containing the Cartesian z
	 * coordinate.
	 */
	double dZ;

	/**
	 * \brief the double value containing the associated time
	 * (if CGeo represents a space-time point) in decimal seconds.
	 */
	double dT;

	/**
	 * \brief the double value containing the x unit vector
	 * used in delta calculations
	 */
	double uX;

	/**
	 * \brief the double value containing the y unit vector
	 * used in delta calculations
	 */
	double uY;

	/**
	 * \brief the double value containing the z unit vector
	 * used in delta calculations
	 */
	double uZ;

	/**
	 * \brief Optional user information tag.
	 */
	int iTag;
};
}  // namespace glassutil
#endif  // GEO_H
