/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef WEBLIST_H
#define WEBLIST_H

#include <json.h>
#include <vector>
#include <memory>

namespace glasscore {

// forward declarations
class CGlass;
class CPick;
class CSite;
class CWeb;

/**
 * \brief glasscore detection node class
 * The detection fabric consists of a collection of nets which
 * in turn contain a list of nodes, each of which is capable of
 * detecting and nucleating a new event. The individual nets are
 * added to the detection fabric (CWeb) by such commands as 'Grid'
 * or 'Global'.
 *
 * CWebList uses smart pointers (std::shared_ptr).
 */
class CWebList {
 public:
	/**
	 * \brief CWebList constructor
	 */
	explicit CWebList(bool useBackgroundThreads = true);

	/**
	 * \brief CWebList destructor
	 */
	~CWebList();

	/**
	 * \brief CWebList clear function
	 *
	 * The clear function for the CWebList class.
	 */
	void clear();

	/**
	 * \brief CWeb communication recieveing function
	 *
	 * The function used by CWeb to recieve communication
	 * (such as configuration or input data), from outside the
	 * glasscore library, or it's parent CGlass.
	 *
	 * Supports Global (genrate global grid) Shell (generate grid at
	 * single depth), Grid (generate local grid), Single (generate
	 * single node), GetWeb (generate output message detailing detection
	 * graph database), and ClearGlass (clear all Node data) inputs.
	 *
	 * \param com - A pointer to a json::object containing the
	 * communication.
	 * \return Returns true if the communication was handled by CWeb,
	 * false otherwise
	 */
	bool dispatch(json::Object *com);

	/**
	 * \brief Add subnet web ('Grid', etc) from detection fabric
	 *
	 * This method creates a new web component, handling common
	 * parameters and then generating the particular flavor by passing
	 * command to the 'dispatch' message in the generated CWeb instance
	 *
	 * \param com - A pointer to a json::object containing name of the node
	 * to be removed.
	 *
	 * \return Always returns true
	 */
	bool addWeb(json::Object *com);

	/**
	 * \brief Remove subnet compoent ('Grid', etc) from detection fabric
	 *
	 * This method removes a previously named subnet component from the
	 * detection fabric by scanning all of the nodes in the current fabric,
	 * and deleting all those that match the name of the grid to be removed.
	 * When a node is deleted, all station linkages are reset for that node.
	 *
	 * \param com - A pointer to a json::object containing name of the web
	 * component to be removed.
	 *
	 * \return Always returns true
	 */
	bool removeWeb(json::Object *com);

	/**
	 * \brief Add a site to the webs
	 * This function adds the given site to all appropriate web detection arrays
	 * in the list of wweb detection arrays
	 *
	 * \param site - A shared_ptr to a CSite object containing the site to add
	 */
	void addSite(std::shared_ptr<CSite> site);

	/**
	 * \brief Remove Site from the webs
	 * This function removes the given site from all web detection arrays in the
	 * list of web detection arrays
	 *
	 * \param site - A shared_ptr to a CSite object containing the site to remove
	 */
	void remSite(std::shared_ptr<CSite> site);

	/**
	 * \brief check to see if each web thread is still functional
	 *
	 * Checks each web thread to see if it is still responsive.
	 */
	bool statusCheck();

	/**
	 * \brief A pointer to the main CGlass class, used to send output,
	 * get sites (stations), encode/decode time, and get debug flags
	 */
	CGlass *pGlass;

	/**
	 * \brief A vector of shared pointers to all currently defined
	 * web detection arrays.
	 */
	std::vector<std::shared_ptr<CWeb>> vWeb;

	/**
	 * \brief A boolean flag indicating whether to perform web updates on
	 * a background thread.
	 */
	bool m_bUseBackgroundThreads;
};
}  // namespace glasscore
#endif  // WEBLIST_H
