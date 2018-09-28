#include <gtest/gtest.h>

#include <string>
#include <glassmath.h>
#include <logger.h>

#define SIG_RESULT 0.60653065971263342
#define SIG_LAPLACE_RESULT 0.036787944117144235

TEST(GlassMathTest, CombinedTest) {
	glass3::util::Logger::disable();

	ASSERT_NEAR(SIG_RESULT, glass3::util::GlassMath::sig(5,5), 0.0001)<< "sig";
	ASSERT_NEAR(SIG_LAPLACE_RESULT, glass3::util::GlassMath::sig_laplace_pdf(5,5), 0.0001)<< "sig_laplace_pdf";  // NOLINT

	// don't really know how to test gauss or random, since they're randomized
	// so just print it out
	for (double sg = 0.0; sg < 5.0; sg += 1.0) {
		printf("[ gauss    ] %.2f %.4f\n", sg,
				glass3::util::GlassMath::gauss(sg, 1.0));
	}
}
