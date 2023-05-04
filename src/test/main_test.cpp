#include <gtest/gtest.h>
#include "buzz.h"

TEST(ToCountTest, ToCount) {
    EXPECT_EQ(uint32_t(10000), to_count(0));
    EXPECT_EQ(uint32_t(12476), to_count(100));
    EXPECT_EQ(uint32_t(1333333), to_count(500));
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
