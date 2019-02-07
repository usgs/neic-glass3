/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef LINK_H
#define LINK_H

#include <tuple>
#include <memory>

namespace glasscore {

// forward declarations
class CNode;
class CSite;

// defines indexes for links
#define LINK_PTR 0   // Node or site pointer
#define LINK_TT1 1   // First travel time in seconds
#define LINK_PHS1 2  // First travel time phase code
#define LINK_TT2 3   // Second travel time in seconds
#define LINK_PHS2 4  // Second travel time phase code
#define LINK_DIST 5  // Distance in degrees between Node and Site

/**
 * \brief Typedef to simplify use of a site-node link.  Contains a CNode
 * pointer to the Node that it's linked to, and then 3 doubles:
 * Nucleation Traveltime 1 (sec), Nucleation Traveltime 2 (sec),
 * and Distance (between Node and Site) in Deg.  Uses weak_ptr to
 * prevent a cyclical reference
 */
typedef std::tuple<std::weak_ptr<CNode>, double, std::string, double, std::string, double> NodeLink; // NOLINT

/**
 * \brief Typedef to simplify use of a node-site link.  Contains a CSite
 * pointer to the Site that it's linked to, and then 3 doubles:
 * Nucleation Traveltime 1 (sec), Nucleation Traveltime 2 (sec),
 * and Distance (between Node and Site) in Deg
 */
typedef std::tuple<std::shared_ptr<CSite>, double, std::string, double, std::string, double> SiteLink; // NOLINT
}  // namespace glasscore
#endif  // LINK_H
