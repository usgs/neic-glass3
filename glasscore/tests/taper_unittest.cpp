#include <gtest/gtest.h>

#include "Taper.h"
#include "Logit.h"

#define TESTPATH "testdata"
#define PHASE "P"
#define PHASEFILENAME "P.trv"

#define NDISTANCEWARP 550
#define NDEPTHWARP 105

#define X1 0.0
#define X2 0.25
#define X3 0.25
#define X4 1.0

#define VAL1 0.6545
#define VAL2 0.9567

// tests to see if the taper can be constructed
TEST(TaperTest, Construction) {
	glassutil::CLogit::disable();

	glassutil::CTaper taper;

	// dX1
	ASSERT_NEAR(0.0, taper.dX1, 0.001)<< "dX1 Default Check";

	// dX2
	ASSERT_NEAR(0.5, taper.dX2, 0.001)<< "dX2 Default Check";

	// dX3
	ASSERT_NEAR(0.5, taper.dX3, 0.001)<< "dX3 Default Check";

	// dX4
	ASSERT_NEAR(1.0, taper.dX4, 0.001)<< "dX4 Default Check";

	glassutil::CTaper taper2(X1, X2, X3, X4);

	// dX1
	ASSERT_NEAR(X1, taper2.dX1, 0.001)<< "dX1 Check";

	// dX2
	ASSERT_NEAR(X2, taper2.dX2, 0.001)<< "dX2 Check";

	// dX3
	ASSERT_NEAR(X3, taper2.dX3, 0.001)<< "dX3 Check";

	// dX4
	ASSERT_NEAR(X4, taper2.dX4, 0.001)<< "dX4 Check";
}

// tests to see if the taper can be constructed
TEST(TaperTest, Copy) {
	glassutil::CLogit::disable();

	glassutil::CTaper taper2(X1, X2, X3, X4);

	glassutil::CTaper taper(taper2);

	// dX1
	ASSERT_NEAR(X1, taper.dX1, 0.001)<< "dX1 Check";

	// dX2
	ASSERT_NEAR(X2, taper.dX2, 0.001)<< "dX2 Check";

	// dX3
	ASSERT_NEAR(X3, taper.dX3, 0.001)<< "dX3 Check";

	// dX4
	ASSERT_NEAR(X4, taper.dX4, 0.001)<< "dX4 Check";
}

TEST(TaperTest, Val) {
	glassutil::CLogit::disable();

	glassutil::CTaper taper(X1, X2, X3, X4);

	ASSERT_EQ(0, taper.Val(X1 - 1))<< "Val X less than X1";

	ASSERT_EQ(0, taper.Val(X4 + 1))<< "Val X greater than X4";

	ASSERT_NEAR(1, taper.Val(X2), 0.001)<< "Val X at X2";

	ASSERT_NEAR(VAL1, taper.Val(X2 - 0.1), 0.001)<< "Val X between X1 and X2";

	ASSERT_NEAR(VAL2, taper.Val(X2 + 0.1), 0.001)<< "Val X between X3 and X4";
}
