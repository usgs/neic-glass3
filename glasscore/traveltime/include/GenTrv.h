/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef GENTRV_H
#define GENTRV_H

#include <json.h>
#include <vector>
#include <memory>
#include <string>

namespace traveltime {

class CTerra;
class CRay;
class CTimeWarp;

/**
 * \brief traveltime branch generator
 *
 * The CGenTrv class is used to generate the travel time table files used
 * by CTrv to rapidly calculate travel times using cubic interpolation
 * a N x M grid given a json configuration of the phase/branches to generate.
 */
class CGenTrv {
 public:
	/**
	 * \brief CGenTrv constructor
	 */
	CGenTrv();

	/**
	 * \brief CGenTrv constructor
	 *
	 * The constructor for the CGenTrv class.
	 * Initializes members to provided values.
	 *
	 * \param modelFile - A std::string containing the model file name
	 * \param outputPath - A std::string containing the output path
	 * \param fileExtension - A std::string containing the output file extension
	 */
	CGenTrv(std::string modelFile, std::string outputPath,
			std::string fileExtension);

	/**
	 * \brief CGenTrv destructor
	 */
	~CGenTrv();

	/**
	 * \brief CGenTrv clear function
	 */
	void clear();

	/**
	 * \brief CGenTrv setup function
	 *
	 * The setup function for the CGenTrv class.
	 * Initializes members to provided values.
	 *
	 * \param modelFile - A std::string containing the model file name
	 * \param outputPath - A std::string containing the output path
	 * \param fileExtension - A std::string containing the output file extension
	 * \return returns true if successful, false otherwise
	 */
	bool setup(std::string modelFile, std::string outputPath,
				std::string fileExtension);

	/**
	 * \brief Generate travel time file
	 *
	 * Generate a travel time file based on the provided json configuration
	 *
	 * \param com - A pointer to a json object containing the configuration
	 * for the travel time file
	 * \return Returns true if the file was generated, false otherwise.
	 */
	bool generate(json::Object *com);

	/**
	 * \brief Generate depth row of the travel time interpolation grid
	 *
	 * Generate A row of travel times and a row distances indexed by the
	 * given depth.
	 *
	 * \param iDepth - An integer containing the index of the depth row being
	 * generated
	 * \param travelTimeArray - A pointer to an array of double values
	 * filled in with the travel times for the given depth index
	 * \param depthDistanceArray - A pointer to an array of double values
	 * filled in with the distances for the given depth index
	 * \param phaseArray - A pointer to an array of characters containing the
	 * filled in with the phases for the given depth index
	 * \return Returns the number of holes (discontinuities?) patched during
	 * generation.
	 */
	int Row(int iDepth, double *travelTimeArray, double *depthDistanceArray,
			char *phaseArray);

	/**
	 * \brief Compute travel time
	 *
	 * Compute the traveltime from the given phase, distance, and depth
	 *
	 * This routine uses reciprocity to calculate travel times when source is
	 * above station
	 *
	 * \param phase - A std::string containing the phase to use
	 * \param delta - A double value containing the distance to use
	 * \param depth - A double value containing the depth to use
	 * \return Returns true if successful, false otherwise
	 */
	bool T(std::string phase, double delta, double depth);

	/**
	 * \brief A string containing the output path
	 */
	std::string m_OutputPath;

	/**
	 * \brief A string containing the output file extension
	 */
	std::string m_FileExtension;

	/**
	 * \brief A boolean flag indicating whether CGenTrav has been set up
	 */
	bool bSetup = false;

	/**
	 * \brief An integer variable containing the number of rays to generate
	 */
	int nRays;

	/**
	 * \brief An integer variable containing the grid index for the distance
	 * warp
	 */
	int nDistanceWarp;

	/**
	 * \brief An integer variable containing the grid index for the depth
	 * warp
	 */
	int nDepthWarp;

	/**
	 * \brief A double variable containing the most recent travel time value
	 * calculated by T()
	 */
	double dTravelTime;

	/**
	 * \brief A double variable containing the most recent depth distance value
	 * calculated by T()
	 */
	double dDepthDistance;

	/**
	 * \brief A double variable containing the most recent ray parameter value
	 * calculated by T()
	 */
	double rayParameter;

	/**
	 * \brief A string containing the most recent phase calculated by T()
	 */
	std::string Phase;

	/**
	 * \brief A vector of strings containing the names of rays to generate
	 */
	std::vector<std::string> vRays;

	/**
	 * \brief A pointer to the earth model object used in generation
	 */
	CTerra *pTerra;

	/**
	 * \brief A pointer to the travel time ray parameters object used in
	 * generation
	 */
	CRay *pRay;

	/**
	 * \brief A pointer to the distance warp object used in generation
	 */
	CTimeWarp *pDistanceWarp;

	/**
	 * \brief A pointer to the depth warp object used in generation
	 */
	CTimeWarp *pDepthWarp;
};
}  // namespace traveltime
#endif  // GENTRV_H
