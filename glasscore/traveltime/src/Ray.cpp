#include "Ray.h"
#include <geo.h>
#include <logger.h>
#include <cmath>
#include <cstring>
#include "Terra.h"

namespace traveltime {

// ------------------------------------------------------------------CRay
CRay::CRay() {
	clear();
}

// ------------------------------------------------------------------CRay
CRay::CRay(CTerra *terra) {
	clear();

	initialize(terra);
}

// ------------------------------------------------------------------~CRay
CRay::~CRay() {
	clear();
}

// ---------------------------------------------------------initialize
void CRay::initialize(CTerra *terra) {
	pTerra = terra;
}

// ---------------------------------------------------------clear
void CRay::clear() {
	pTerra = NULL;

	dMinimumRayParam = 0;
	dMaximumRayParam = 0;
	dEarthRadius = 0;
	iPhaseIndex = -1;
}

// ------------------------------------------------------------------setPhase
int CRay::setPhase(const char *phase) {
	// look for phase in phase list
	for (int i = 0; i < nPhase; i++) {
		// is this it
		if (strcmp(phase, PhaseIndexesValues[i]) != 0) {
			continue;
		}

		// found it
		iPhaseIndex = i;
		return (iPhaseIndex);
	}

	// no matching phase found
	return (-1);
}

// --------------------------------------------------------------setupRayParam
void CRay::setupRayParam() {
	// nullcheck
	if (pTerra == NULL) {
		return;
	}

	int layerIndex;

	// init to defaults
	dMinimumRayParam = 0.0;
	dMaximumRayParam = 1.0;

	// based on the phase we're set up for, setup
	// the ray parameters
	switch (iPhaseIndex) {
		case RAY_Pup:
			dMinimumRayParam = 0.0;
			dMaximumRayParam = dEarthRadius / pTerra->P(dEarthRadius);
			break;

		case RAY_P:
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			dMaximumRayParam = dEarthRadius / pTerra->P(dEarthRadius);
			break;

		case RAY_Pdiff:
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			dMaximumRayParam = dMinimumRayParam;
			break;

		case RAY_PP:
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			dMaximumRayParam = dEarthRadius / pTerra->P(dEarthRadius);
			break;

		case RAY_PPP:
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			dMaximumRayParam = dEarthRadius / pTerra->P(dEarthRadius);
			break;

		case RAY_PKP:
		case RAY_PKPab:
		case RAY_PKPbc:
			layerIndex = pTerra->iInnerDiscontinuity + 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMaximumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			break;

		case RAY_PKIKP:
		case RAY_PKPdf:
			layerIndex = 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			layerIndex = pTerra->iInnerDiscontinuity;
			dMaximumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			break;

		case RAY_PcP:
			layerIndex = 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMaximumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerPVel[layerIndex];
			break;

		case RAY_Sup:
			dMinimumRayParam = 0.0;
			dMaximumRayParam = dEarthRadius / pTerra->S(dEarthRadius);
			break;

		case RAY_S:
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerSVel[layerIndex];
			dMaximumRayParam = dEarthRadius / pTerra->S(dEarthRadius);
			break;

		case RAY_Sdiff:
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerSVel[layerIndex];
			dMaximumRayParam = dMinimumRayParam;
			break;

		case RAY_SS:
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerSVel[layerIndex];
			dMaximumRayParam = dEarthRadius / pTerra->S(dEarthRadius);
			break;

		case RAY_SSS:
			layerIndex = pTerra->iOuterDiscontinuity + 1;
			dMinimumRayParam = pTerra->dLayerRadii[layerIndex]
					/ pTerra->dLayerSVel[layerIndex];
			dMaximumRayParam = dEarthRadius / pTerra->S(dEarthRadius);
			break;

		default:
			break;
	}
	// printf("Setup phaseIndex %d dMinimumRayParam:%.6f dMaximumRayParam:%.6f\n",
	// iPhaseIndex, dMinimumRayParam, dMaximumRayParam);
}

// ------------------------------------------------------------------setDepth
void CRay::setDepth(double depth) {
	// use earthmodel radius if available
	if (pTerra) {
		dEarthRadius = pTerra->dEarthRadius - depth;
	} else {
		dEarthRadius = 6371.0 - depth;
	}
}

// ------------------------------------------------------------------travel
double CRay::travel(double delta) {
	return (travel(delta, pTerra->dEarthRadius));
}

// ------------------------------------------------------------------travel
double CRay::travel(double delta, double earthRadius) {
	double rayParam;

	return (travel(delta, earthRadius, &rayParam));
}

// ------------------------------------------------------------------travel
double CRay::travel(double delta, double earthRadius, double *rayParam) {
	// Calculate minimum travel time from Tau curve for current
	// branch. Returns least travel time as well as ray parameter
	// at minimum.
	// nullcheck
	if (pTerra == NULL) {
		return (-1);
	}

	int ip;

	double calcRayParam;
	double pinc;
	double x[3];
	double y1, y2;
	double time;
	double d;
	double trav;
	double rturn;
	int nbranch;

	glass3::util::Logger::log(
			"debug",
			"CRay::travel: delta:" + std::to_string(delta)
					+ " earthRadius:" + std::to_string(earthRadius));

	trav = -10.0;  // Indicates no arrivals detected

	double funFac = 1.0;

	switch (iPhaseIndex) {
		case RAY_Pdiff:
			rturn = pTerra->dLayerRadii[pTerra->iOuterDiscontinuity + 1];
			d = 2.0
					* pTerra->integrateRaySegment(FUN_P_DELTA, rturn,
													dEarthRadius,
													dMinimumRayParam)
					+ pTerra->integrateRaySegment(FUN_P_DELTA, dEarthRadius,
													earthRadius,
													dMinimumRayParam);
			if (delta < d) {
				return (trav);
			}
			time = 2.0
					* pTerra->integrateRaySegment(FUN_P_TIME, rturn,
													dEarthRadius,
													dMinimumRayParam)
					+ pTerra->integrateRaySegment(FUN_P_TIME, dEarthRadius,
													earthRadius,
													dMinimumRayParam);
			trav =
					time
							+ RAD2DEG * (delta - d)
									* pTerra->dLayerRadii[pTerra
											->iOuterDiscontinuity + 1] * DEG2KM
									/ earthRadius
									/ pTerra->dLayerPVel[pTerra
											->iOuterDiscontinuity + 1];
			*rayParam = dMinimumRayParam;
			return trav;
		case RAY_Sdiff:
			rturn = pTerra->dLayerRadii[pTerra->iOuterDiscontinuity + 1];
			d = 2.0
					* pTerra->integrateRaySegment(FUN_S_DELTA, rturn,
													dEarthRadius,
													dMinimumRayParam)
					+ pTerra->integrateRaySegment(FUN_S_DELTA, dEarthRadius,
													earthRadius,
													dMinimumRayParam);
			if (delta < d)
				return (trav);
			time = 2.0
					* pTerra->integrateRaySegment(FUN_S_TIME, rturn,
													dEarthRadius,
													dMinimumRayParam)
					+ pTerra->integrateRaySegment(FUN_S_TIME, dEarthRadius,
													earthRadius,
													dMinimumRayParam);
			trav =
					time
							+ RAD2DEG * (delta - d)
									* pTerra->dLayerRadii[pTerra
											->iOuterDiscontinuity + 1] * DEG2KM
									/ earthRadius
									/ pTerra->dLayerSVel[pTerra
											->iOuterDiscontinuity + 1];
			*rayParam = dMinimumRayParam;
			return trav;
	}
	double p1, p2;
	double pincmax = 2.0;
	double pincmin = 1.0e-8;
	pinc = pincmin;
	p2 = dMaximumRayParam + pinc;
	ip = 0;
	nbranch = 0;  // Branch index to resolve multiple phases
	p1 = y1 = y2 = 0.0;
	time = -5.0;

	if (false) {
		double qinc = 1.0;
		for (double calcRayParam = dMinimumRayParam;
				calcRayParam < dMaximumRayParam + qinc; calcRayParam += qinc) {
			double q = calcRayParam;
			if (q > dMaximumRayParam) {
				q = dMaximumRayParam;
			}
			// printf("%.4f %.6f\n", q,
			// calculateThetaFunction(q, funFac, delta, earthRadius));
		}
		return (-13.0);
	}

	while (p2 > dMinimumRayParam) {
		// printf("%d p2:%.4f dMinimumRayParam:%.4f\n", ip, p2, dMinimumRayParam);
		double p0 = p1;
		p1 = p2;
		p2 -= pinc;
		calcRayParam = p2;
		if (calcRayParam < dMinimumRayParam) {
			calcRayParam = dMinimumRayParam;
		}
		double y0 = y1;
		y1 = y2;
		y2 = calculateThetaFunction(calcRayParam, funFac, delta, earthRadius);

		// Skip first two calculations
		if (ip++ < 2) {
			continue;
		}

		if (y1 < y0 && y1 < y2) {
			funFac = 1.0;
			pinc = pincmin;
		} else if (y1 > y0 && y1 > y2) {
			funFac = -1.0;
			pinc = pincmin;
		} else {
			// Accelerate search by expanding search mesh
			pinc *= 2.0;
			if (pinc > pincmax) {
				pinc = pincmax;
			}
			continue;
		}
		x[0] = calcRayParam;
		x[1] = p1;
		x[2] = p0;
		time = funFac
				* brentMinimization(x, 2.0e-3, &calcRayParam, funFac, delta,
									earthRadius);
		nbranch++;
		switch (iPhaseIndex) {
			case RAY_PKPab:
				// First extrema in rayParam order
				if (nbranch == 1) {
					trav = time;
					*rayParam = calcRayParam;
				}
				break;
			case RAY_PKPbc:
				// Second extrema in rayParam order
				if (nbranch == 2) {
					trav = time;
					*rayParam = calcRayParam;
				}
				break;
			default:
				// For all other multiple branches accept the earliest arrival
				if (trav < 0.0 || time < trav) {
					trav = time;
					*rayParam = calcRayParam;
				}
				break;
		}
		funFac = 1.0;
	}
	switch (iPhaseIndex) {
		case RAY_PKPbc:
			// For PKPbc to exist, PKPab must also, otherwise phase is PKPab
			if (nbranch < 2) {
				trav = -12.0;
			}
			break;
	}
	return (trav);
}

// ------------------------------------------------------------------travelBasic
// travelBasic: Calculate travel time (seconds) for given distance (radians)
double CRay::travelBasic(double delta) {
	double x[6];
	double trav;
	double xmin;
	int i;

	x[1] = dMinimumRayParam;
	x[2] = dMinimumRayParam + 1.0;

	double funFac = 1.0;

	if (calculateThetaFunction(x[2], funFac, delta, dEarthRadius)
			> calculateThetaFunction(x[1], funFac, delta, dEarthRadius)) {
		funFac = -1.0;
	}

	for (i = 0; i < 2; i++) {
		x[0] = x[1];
		x[1] = x[2];
		calculateBracketMinima(x, funFac, delta, dEarthRadius);
		double time = funFac
				* brentMinimization(x, 2.0e-3, &xmin, funFac, delta,
									dEarthRadius);
		if (i == 0) {
			trav = time;
		}
		if (time < trav) {
			trav = time;
		}
		if (x[2] >= dMaximumRayParam) {
			break;
		}
		x[1] = xmin;
		funFac = -funFac;
	}
	return (trav);
}

// ------------------------------------------------------------------delta
double CRay::delta(double time, double *pret) {
	// Find the minimum delta given a travel-time
	// Returns -1.0 for no such arrival
	// nullcheck
	if (pTerra == NULL) {
		return (-1);
	}

	double dmax = -1.0;
	double pmax;
	int n = 1000;

	for (int i = 0; i < n; i++) {
		double pdel = (dMaximumRayParam - dMinimumRayParam) / (n - 1);
		double t1 = 0;
		double p2 = dMinimumRayParam + i * pdel;
		double t2 = integrateFunction(FUN_P_TIME, p2, pTerra->dEarthRadius);

		if (!i) {
			t1 = t2;
			continue;
		}

		if ((t1 < time && t2 > time) || (t2 < time && t1 > time)) {
			double dcal = -1.0;
			double p1 = p2 - pdel;
			double t2save = t2;
			double rayParam = 0;
			for (int iref = 0; iref < 5; iref++) {
				rayParam = p1 + (time - t1) * (p2 - p1) / (t2 - t1);
				double tcal = integrateFunction(FUN_P_TIME, rayParam,
											pTerra->dEarthRadius);
				dcal = integrateFunction(FUN_P_DELTA, rayParam,
											pTerra->dEarthRadius);

				if (fabs(tcal - time) < 0.5) {
					break;
				}

				if (t2 > t1) {
					if (tcal > time) {
						p2 = rayParam;
						t2 = tcal;
					} else {
						p1 = rayParam;
						t1 = tcal;
					}
				} else {
					if (tcal > time) {
						p1 = rayParam;
						t1 = tcal;
					} else {
						p2 = rayParam;
						t2 = tcal;
					}
				}
			}

			t2 = t2save;

			if (dcal > dmax) {
				dmax = dcal;
				pmax = rayParam;
			}
		}
		t1 = t2;
	}
	*pret = pmax;
	return (dmax);
}

// ------------------------------------------------------------------T
double CRay::T(double rayParam, double earthRadius) {
	// Calculate travel time as a function of ray parameter.
	return (integrateFunction(FUN_P_TIME, rayParam, earthRadius));
}

// ------------------------------------------------------------------D
double CRay::D(double rayParam, double earthRadius) {
	// Calculate distance (radians) as a function of ray parameter
	if (rayParam < 0.0001) {
		return (0.0);
	}
	return (integrateFunction(FUN_P_DELTA, rayParam, earthRadius));
}

// ------------------------------------------------------------------tau
double CRay::tau(double rayParam, double earthRadius) {
	// tau(rayParam): Calculate Tau as a function of ray parameter
	return (integrateFunction(FUN_P_TAU, rayParam, earthRadius));
}

// -----------------------------------------------------------integrateFunction
double CRay::integrateFunction(int functionIndex, double rayParam,
								double earthRadius) {
	// integrateFunction(functionIndex, rayParam): Time, range, and tau
	// integrals over depth. This is an internal routine.
	// nullcheck
	if (pTerra == NULL) {
		return (-1);
	}

	double rturn;  // Turning radius (if applicable)
	int ilay1;  // Innermost model index
	int ilay2;  // Outermost model index
	double val;  // Integral accumulator

	if (rayParam < dMinimumRayParam || rayParam > dMaximumRayParam) {
		return -100.0;
	}

	val = 0.0;
	switch (iPhaseIndex) {
		case RAY_Pup:
			val = pTerra->integrateRaySegment(functionIndex, dEarthRadius,
												earthRadius, rayParam);
			break;
		case RAY_P:
			ilay1 = pTerra->iOuterDiscontinuity + 1;
			ilay2 = pTerra->nLayer - 1;
			rturn = pTerra->calculateTurnRadius(ilay1, ilay2,
												pTerra->dLayerPVel, rayParam);
			val = 2.0
					* pTerra->integrateRaySegment(functionIndex, rturn,
													dEarthRadius, rayParam)
					+ pTerra->integrateRaySegment(functionIndex, dEarthRadius,
													earthRadius, rayParam);
			break;
		case RAY_Pdiff:
			val = 0.0;
			break;
		case RAY_PP:
			ilay1 = pTerra->iOuterDiscontinuity + 1;
			ilay2 = pTerra->nLayer - 1;
			rturn = pTerra->calculateTurnRadius(ilay1, ilay2,
												pTerra->dLayerPVel, rayParam);
			val = 4.0
					* pTerra->integrateRaySegment(functionIndex, rturn,
													dEarthRadius, rayParam)
					+ 3.0
							* pTerra->integrateRaySegment(functionIndex,
															dEarthRadius,
															earthRadius,
															rayParam);
			break;
		case RAY_PPP:
			ilay1 = pTerra->iOuterDiscontinuity + 1;
			ilay2 = pTerra->nLayer - 1;
			rturn = pTerra->calculateTurnRadius(ilay1, ilay2,
												pTerra->dLayerPVel, rayParam);
			val = 6.0
					* pTerra->integrateRaySegment(functionIndex, rturn,
													dEarthRadius, rayParam)
					+ 5.0
							* pTerra->integrateRaySegment(functionIndex,
															dEarthRadius,
															earthRadius,
															rayParam);
			break;
		case RAY_PKP:
		case RAY_PKPab:
		case RAY_PKPbc:
			ilay1 = pTerra->iInnerDiscontinuity + 1;
			ilay2 = pTerra->iOuterDiscontinuity;
			rturn = pTerra->calculateTurnRadius(ilay1, ilay2,
												pTerra->dLayerPVel, rayParam);
			val = 2.0
					* pTerra->integrateRaySegment(functionIndex, rturn,
													dEarthRadius, rayParam)
					+ pTerra->integrateRaySegment(functionIndex, dEarthRadius,
													earthRadius, rayParam);
			break;
		case RAY_PKIKP:
		case RAY_PKPdf:
			ilay1 = 1;
			ilay2 = pTerra->iInnerDiscontinuity;
			rturn = pTerra->calculateTurnRadius(ilay1, ilay2,
												pTerra->dLayerPVel, rayParam);
			val = 2.0
					* pTerra->integrateRaySegment(functionIndex, rturn,
													dEarthRadius, rayParam)
					+ pTerra->integrateRaySegment(functionIndex, dEarthRadius,
													earthRadius, rayParam);
			break;
		case RAY_PcP:
			rturn = pTerra->dLayerRadii[pTerra->iOuterDiscontinuity + 1];
			val = 2.0
					* pTerra->integrateRaySegment(functionIndex, rturn,
													dEarthRadius, rayParam)
					+ pTerra->integrateRaySegment(functionIndex, dEarthRadius,
													earthRadius, rayParam);

			break;
		case RAY_Sup:
			val = pTerra->integrateRaySegment(functionIndex + 1, dEarthRadius,
												earthRadius, rayParam);
			break;
		case RAY_S:
			ilay1 = pTerra->iOuterDiscontinuity + 1;
			ilay2 = pTerra->nLayer - 1;
			rturn = pTerra->calculateTurnRadius(ilay1, ilay2,
												pTerra->dLayerSVel, rayParam);
			val = 2.0
					* pTerra->integrateRaySegment(functionIndex + 1, rturn,
													dEarthRadius, rayParam)
					+ pTerra->integrateRaySegment(functionIndex + 1,
													dEarthRadius, earthRadius,
													rayParam);
			break;
		case RAY_Sdiff:
			return 0.0;
		case RAY_SS:
			ilay1 = pTerra->iOuterDiscontinuity + 1;
			ilay2 = pTerra->nLayer - 1;
			rturn = pTerra->calculateTurnRadius(ilay1, ilay2,
												pTerra->dLayerSVel, rayParam);
			val = 4.0
					* pTerra->integrateRaySegment(functionIndex + 1, rturn,
													dEarthRadius, rayParam)
					+ 3.0
							* pTerra->integrateRaySegment(functionIndex + 1,
															dEarthRadius,
															earthRadius,
															rayParam);
			break;
		case RAY_SSS:
			ilay1 = pTerra->iOuterDiscontinuity + 1;
			ilay2 = pTerra->nLayer - 1;
			rturn = pTerra->calculateTurnRadius(ilay1, ilay2,
												pTerra->dLayerSVel, rayParam);
			val = 6.0
					* pTerra->integrateRaySegment(functionIndex + 1, rturn,
													dEarthRadius, rayParam)
					+ 5.0
							* pTerra->integrateRaySegment(functionIndex + 1,
															dEarthRadius,
															earthRadius,
															rayParam);
			break;
		default:
			break;
	}
	return val;
}

// ------------------------------------------------------calculateThetaFunction
double CRay::calculateThetaFunction(double rayParam, double dFunFac,
									double dDelta, double dRcvr) {
	// Calculates theta function after Buland and Chapman(1983)
	// Assumes x in radians
	// Reflect at dMaximumRayParam
	if (rayParam > dMaximumRayParam) {
		rayParam = 2.0 * dMaximumRayParam - rayParam;
	}

	double theta = tau(rayParam, dRcvr) + rayParam * dDelta;
	double result = dFunFac * theta;

	return (result);
}

// ------------------------------------------------------calculateBracketMinima
static double dArg1;
static double dArg2;
#define ITMAX 50
#define SHFT(a, b, c, d) (a)=(b); (b)=(c); (c)=(d);
#define GOLD 1.618034
#define GLIMIT 100.0
#define TINY 1.0e-20
#define SIGN(a, b) ((b) > 0.0 ? fabs(a) : -fabs(a))
#define FMAX(a, b) (dArg1 = (a), dArg2 = (b), (dArg1 > dArg2) ? dArg1 : dArg2)
void CRay::calculateBracketMinima(double *x, double dFunFac, double dDelta,
									double dRcvr) {
	// calculateBracketMinima: Bracket function minima (1-dimensional)
	double ulim;
	double u;
	double r;
	double q;
	double fu;
	double dum;
	double ax = x[0];
	double bx = x[1];
	double cx = x[2];
	double fa = calculateThetaFunction(ax, dFunFac, dDelta, dRcvr);  // x[3];
	double fb = calculateThetaFunction(bx, dFunFac, dDelta, dRcvr);  // x[4];
	double fc = x[5];

	if (fb > fa) {
		SHFT(dum, ax, bx, dum)
		SHFT(dum, fb, fa, dum)
	}
	cx = bx + GOLD * (bx - ax);
	fc = calculateThetaFunction(cx, dFunFac, dDelta, dRcvr);
	while (fb > fc) {
		r = (bx - ax) * (fb - fc);
		q = (bx - cx) * (fb - fa);
		u = bx
				- ((bx - cx) * q - (bx - ax) * r)
						/ (2.0 * SIGN(FMAX(fabs(q-r), TINY), q - r));
		ulim = bx + GLIMIT * (cx - bx);
		if ((bx - u) * (u - cx) > 0.0) {
			fu = calculateThetaFunction(u, dFunFac, dDelta, dRcvr);
			if (fu < fc) {
				ax = bx;
				bx = u;
				fa = fb;
				fb = fu;
				goto pau;
			} else if (fu > fb) {
				cx = u;
				fc = fu;
				goto pau;
			}
			u = cx + GOLD * (cx - bx);
			fu = calculateThetaFunction(u, dFunFac, dDelta, dRcvr);
		} else if ((cx - u) * (u - ulim) > 0.0) {
			fu = calculateThetaFunction(u, dFunFac, dDelta, dRcvr);
			if (fu < fc) {
				SHFT(bx, cx, u, cx+GOLD*(cx - bx))
				SHFT(fb, fc, fu,
						calculateThetaFunction(u, dFunFac, dDelta, dRcvr))
			}
		} else if ((u - ulim) * (ulim - cx) >= 0.0) {
			u = ulim;
			fu = calculateThetaFunction(u, dFunFac, dDelta, dRcvr);
		} else {
			u = cx + GOLD * (cx - bx);
			fu = calculateThetaFunction(u, dFunFac, dDelta, dRcvr);
		}
		SHFT(ax, bx, cx, u);
		SHFT(fa, fb, fc, fu);
	}

	pau: x[0] = ax;
	x[1] = bx;
	x[2] = cx;
	x[3] = fa;
	x[4] = fb;
	x[5] = fc;
	return;
}

// -----------------------------------------------------------brentMinimization
#define CGOLD 0.3819660
#define ZEPS 1.0e-10
double CRay::brentMinimization(double *xx, double tol, double *xmin,
								double dFunFac, double dDelta, double dRcvr) {
	int iter;
	double ax = xx[0];
	double bx = xx[1];
	double cx = xx[2];
	double a, b, d;
	double etemp;
	double fv, fw, fx;
	double p, q, r;
	double u, v, w, x;
	double e = 0.0;

	a = (ax < cx ? ax : cx);
	b = (ax > cx ? ax : cx);
	x = w = v = bx;
	fw = fv = fx = calculateThetaFunction(x, dFunFac, dDelta, dRcvr);
	for (iter = 1; iter <= ITMAX; iter++) {
		double xm = 0.5 * (a + b);
		double tol1 = tol * fabs(x) + ZEPS;
		double tol2 = 2.0 * tol1;
		if (fabs(x - xm) <= (tol2 - 0.5 * (b - a))) {
			*xmin = x;
			return fx;
		}
		if (fabs(e) > tol1) {
			r = (x - w) * (fx - fv);
			q = (x - v) * (fx - fw);
			p = (x - v) * q - (x - w) * r;
			q = 2.0 * (q - r);
			if (q > 0.0)
				p = -p;
			q = fabs(q);
			etemp = e;
			e = d;
			if (fabs(p) >= fabs(0.5 * q * etemp) || p <= q * (a - x)
					|| p >= q * (b - x)) {
				d = CGOLD * (e = (x >= xm ? a - x : b - x));
			} else {
				d = p / q;
				u = x + d;
				if (u - a < tol2 || b - u < tol2)
					d = SIGN(tol1, xm - x);
			}
		} else {
			d = CGOLD * (e = (x >= xm ? a - x : b - x));
		}
		u = (fabs(d) >= tol1 ? x + d : x + SIGN(tol1, d));
		double fu = calculateThetaFunction(u, dFunFac, dDelta, dRcvr);
		if (fu <= fx) {
			if (u >= x)
				a = x;
			else
				b = x;
			SHFT(v, w, x, u)
			SHFT(fv, fw, fx, fu)
		} else {
			if (u < x)
				a = u;
			else
				b = u;
			if (fu <= fw || w == x) {
				v = w;
				w = u;
				fv = fw;
				fw = fu;
			} else if (fu <= fv || v == x || v == w) {
				v = u;
				fv = fu;
			}
		}
	}
	*xmin = x;
	return fx;
}
}  // namespace traveltime
