#include <gtest/gtest.h>

int add(int x, int y) {
    return x + y;
}

TEST(AddTest, Add1) {
    EXPECT_EQ(1, add(1, 0));
    EXPECT_EQ(1, add(0, 1));
    EXPECT_EQ(2, add(1, 1));
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}