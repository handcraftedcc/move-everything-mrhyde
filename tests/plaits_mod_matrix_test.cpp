#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "plaits_move_engine.h"

static void fail(const char *msg) {
    std::fprintf(stderr, "FAIL: %s\n", msg);
    std::exit(1);
}

static void expect_near(float got, float want, float eps, const char *msg) {
    if (std::fabs(got - want) > eps) {
        std::fprintf(stderr, "FAIL: %s (got=%f want=%f)\n", msg, got, want);
        std::exit(1);
    }
}

int main() {
    ppf_mod_sources_t src{};
    src.lfo = 0.65f;
    src.env = 0.40f;
    src.cycle_env = -0.35f;
    src.random = 0.25f;
    src.velocity = 0.90f;
    src.poly_aftertouch = 0.50f;

    ppf_mod_amounts_t amt{};
    float base = 0.42f;

    float zero_mod = ppf_apply_destination_modulation(base, src, amt, 0.0f, 1.0f);
    expect_near(zero_mod, base, 1e-6f, "zero amounts must be strict no-op");

    amt.lfo = 0.5f;
    amt.env = -0.2f;
    amt.cycle_env = 0.75f;
    amt.random = 0.25f;
    amt.velocity = 0.3f;
    amt.poly_aftertouch = -0.4f;

    float enabled = ppf_apply_destination_modulation(base, src, amt, 0.0f, 1.0f);
    if (enabled == base) {
        fail("enabled modulation should alter value when amounts are non-zero");
    }
    if (enabled < 0.0f || enabled > 1.0f) {
        fail("enabled modulation should respect clamp bounds");
    }

    float linear = ppf_apply_bipolar_curve(0.25f, 0.0f);
    expect_near(linear, 0.25f, 1e-6f, "bipolar curve at 0 must be linear");

    float exp_shape = ppf_apply_bipolar_curve(0.25f, -1.0f);
    if (!(exp_shape < linear)) {
        fail("negative bipolar curve should produce exponential response");
    }

    float inv_exp_shape = ppf_apply_bipolar_curve(0.25f, 1.0f);
    if (!(inv_exp_shape > linear)) {
        fail("positive bipolar curve should produce inverse exponential response");
    }

    expect_near(ppf_apply_bipolar_curve(0.0f, -1.0f), 0.0f, 1e-6f, "curve should keep 0 endpoint");
    expect_near(ppf_apply_bipolar_curve(1.0f, 1.0f), 1.0f, 1e-6f, "curve should keep 1 endpoint");

    std::printf("PASS: plaits modulation matrix behavior\n");
    return 0;
}
