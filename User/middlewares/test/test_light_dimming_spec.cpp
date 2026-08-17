/**
 * @file test_light_dimming_spec.cpp
 * @brief Host 单测：LightDimmingSpec（无硬件 / 无 Matter）
 * @note CI 扫描 User 下各模块 test 目录中的 cpp 自动发现。
 */
#include "LightDimmingSpec.h"

#include <cstdio>
#include <cstdlib>

static int g_failures = 0;

static void ExpectTrue(bool cond, const char* expr)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", expr);
        g_failures++;
    }
}

int main()
{
    using namespace LightDimmingSpec;

    ExpectTrue(MatterLevelToDriverBrightness(0) == 0U, "level 0 -> 0");
    ExpectTrue(MatterLevelToDriverBrightness(254) == 255U, "level 254 -> 255");
    ExpectTrue(ClampPhysicalBrightness(0) == 0U, "clamp 0");
    ExpectTrue(ClampPhysicalBrightness(1) == kMinPhysicalBrightness255,
               "clamp 1 -> min");
    ExpectTrue(ClampPhysicalBrightness(255) == 255U, "clamp 255");
    ExpectTrue(MapBrightnessToDutyScale(0) == 0U, "duty 0");
    ExpectTrue(MapBrightnessToDutyScale(255) == 255U, "duty 255");
    ExpectTrue(MapBrightnessToDutyScale(kMinPhysicalBrightness255) ==
                   kMinPhysicalDutyScale255,
               "duty at min bri");

    if (g_failures != 0)
    {
        std::fprintf(stderr, "%d assertion(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }

    std::puts("LightDimmingSpec host tests OK");
    return EXIT_SUCCESS;
}
