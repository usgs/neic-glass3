#include <logger.h>
#include <glassmath.h>
#include <random>
#include <cmath>

namespace glass3 {
namespace util {

bool GlassMath::m_bInitialized = false;
std::default_random_engine GlassMath::m_RandomGenerator;

// constants
constexpr double GlassMath::k_RadiansToDegrees;
constexpr double GlassMath::k_DegreesToRadians;
constexpr double GlassMath::k_Pi;
constexpr double GlassMath::k_TwoPi;

// ---------------------------------------------------------Sig
// Calculate the significance function, which is just
// the bell shaped curve with Sig(0, x) pinned to 1.
// It is used for pruning and association, and is roughly
// analogous to residual pruning in least squares approaches
double GlassMath::sig(double x, double sigma) {
	return (exp(-0.5 * x * x / sigma / sigma));
}

// ---------------------------------------------------------Sig
// Calculate the laplacian significance function, which is just
// It is used for pruning and association, and is roughly
// analogous to residual pruning in L1 approach.
double GlassMath::sig_laplace_pdf(double x, double sigma) {
	if (x > 0) {
		return ((1. / (2. * sigma)) * exp(-1. * x / sigma));
	} else {
		return ((1. / (2. * sigma)) * exp(x / sigma));
	}
}

// ---------------------------------------------------------Rand
double GlassMath::random(double x, double y) {
	initializeRandom();

	std::uniform_real_distribution<double> distribution(x, y);
	double number = distribution(m_RandomGenerator);
	return (number);
}

// ---------------------------------------------------------gauss
// generate Gaussian pseudo-random number using the
// polar form of the Box-Muller method
double GlassMath::gauss(double avg, double std) {
	double rsq = 0;
	double v1 = 0;

	do {
		v1 = random(-1.0, 1.0);
		double v2 = random(-1.0, 1.0);
		rsq = v1 * v1 + v2 * v2;
	} while (rsq >= 1.0);

	double fac = sqrt(-2.0 * log(rsq) / rsq);
	double x = std * fac * v1 + avg;

	// return random normal deviate
	return (x);
}

// ---------------------------------------------------------initializeRandom
void GlassMath::initializeRandom() {
	if (m_bInitialized == false) {
		// seed the random number generator
		std::random_device randomDevice;
		m_RandomGenerator.seed(randomDevice());
		m_bInitialized = true;
	}
}

/**
 * Get the minimum angle (degrees) between two angles
 */
double GlassMath::angleDifference(double angle1, double angle2) {
	double difference = fmod(abs(angle1 - angle2), 360.);
	if (difference > 360.) {
		difference = 360. - difference;
	}
	return difference;
}

}  // namespace util
}  // namespace glass3
