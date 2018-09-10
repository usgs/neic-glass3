#include <gtest/gtest.h>

#include <string>
#include "GlassMath.h"
#include "Logit.h"

#define SIG_RESULT 0.60653065971263342
#define SIG_LAPLACE_RESULT 0.036787944117144235

TEST(GlassMathTest, CombinedTest) {
	glassutil::CLogit::disable();

	ASSERT_NEAR(SIG_RESULT, glassutil::GlassMath::sig(5,5), 0.0001)<< "sig";
	ASSERT_NEAR(SIG_LAPLACE_RESULT, glassutil::GlassMath::sig_laplace_pdf(5,5), 0.0001)<< "sig_laplace_pdf";  // NOLINT

	// don't really know how to test gauss or random, since they're randomized
	// so just print it out
	for (double sg = 0.0; sg < 5.0; sg += 1.0) {
		printf("[ gauss    ] %.2f %.4f\n", sg,
				glassutil::GlassMath::gauss(sg, 1.0));
	}
}
