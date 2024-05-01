#include <unity.h>

#include "encoder.h"
#include "encoder.cpp"


void test_encoder_distance()
{
    encoder_position_t start = { 100, 200 };
    TEST_ASSERT_EQUAL_INT32(100, encoder_distance_pulses(start, {200,300}));
    TEST_ASSERT_EQUAL_INT32(0, encoder_distance_pulses(start, {200,100}));
    TEST_ASSERT_EQUAL_INT32(-100, encoder_distance_pulses(start, {0,100}));
    TEST_ASSERT_EQUAL_INT32(0, encoder_distance_pulses(start, {0,300}));
    TEST_ASSERT_EQUAL_INT32(-300, encoder_distance_pulses(start, {UINT32_MAX-199,UINT32_MAX-99}));

    start = { UINT32_MAX-99, 100 };
    TEST_ASSERT_EQUAL_INT32(200, encoder_distance_pulses(start, {100,300}));
    TEST_ASSERT_EQUAL_INT32(150, encoder_distance_pulses(start, {100,200}));

    TEST_ASSERT_EQUAL_INT32(50, encoder_distance_pulses({UINT32_MAX-100, UINT32_MAX-1000}, {UINT32_MAX-50,UINT32_MAX-950}));
    TEST_ASSERT_EQUAL_INT32(1000, encoder_distance_pulses({UINT32_MAX-949, UINT32_MAX-1049}, {0,0}));

    // long distance
    uint32_t dist = UINT32_MAX/4;
    TEST_ASSERT_EQUAL_INT32(dist, encoder_distance_pulses({0, 0}, {dist, dist}));
    dist = UINT32_MAX/2-10;
    TEST_ASSERT_EQUAL_INT32(dist, encoder_distance_pulses({0, 0}, {dist, dist}));
    dist = UINT32_MAX/2;
    TEST_ASSERT_EQUAL_INT32(dist, encoder_distance_pulses({0, 0}, {dist, dist}));
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_encoder_distance);
    return UNITY_END();
}


int main(void)
{
    return runUnityTests();
}
