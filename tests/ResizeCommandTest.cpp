#include "core/command/entity/CadObject.h"
#include "core/command/resize/ResizeCommand.h"
#include "core/command/resize/ResizeFactors.h"
#include "core/command/resize/ResizeService.h"

#include "CubeFixture.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cmath>

namespace cmd = semanticad::core::command;
using cmd::CadObject;
using cmd::resize::ResizeCommand;
using cmd::resize::ResizeFactors;
using cmd::resize::ResizeService;
using semanticad::test::boundsOf;
using semanticad::test::makeCube;

// --- value object ---------------------------------------------------------

TEST(ResizeFactors, RejectsZeroNegativeNonFinite) {
    EXPECT_FALSE(ResizeFactors::create(0.0, 1.0, 1.0).has_value());
    EXPECT_FALSE(ResizeFactors::create(1.0, -2.0, 1.0).has_value());
    EXPECT_FALSE(ResizeFactors::create(1.0, 1.0, INFINITY).has_value());
    EXPECT_TRUE(ResizeFactors::create(1.05, 0.5, 2.0).has_value());
}

// --- service (real geometry) ----------------------------------------------

TEST(ResizeService, ScalesGeometry) {
    auto data = makeCube();  // bounds [-0.5, 0.5]^3
    CadObject object(data);
    ResizeService::apply(*ResizeFactors::create(2.0, 2.0, 2.0), object);
    const auto b = boundsOf(data);
    EXPECT_NEAR(b[0], -1.0, 1e-6);
    EXPECT_NEAR(b[1], 1.0, 1e-6);
    EXPECT_NEAR(b[3], 1.0, 1e-6);
    EXPECT_NEAR(b[5], 1.0, 1e-6);
}

// --- command (JSON parsing + validation) ----------------------------------

TEST(ResizeCommand, HasName) { EXPECT_EQ(ResizeCommand().name(), "resize"); }

TEST(ResizeCommand, ParsesParamsAndScales) {
    auto data = makeCube();
    CadObject object(data);
    nlohmann::ordered_json params;
    params["x"] = 2.0;
    params["y"] = 2.0;
    params["z"] = 2.0;
    EXPECT_TRUE(ResizeCommand().execute(params, object).ok);
    EXPECT_NEAR(boundsOf(data)[1], 1.0, 1e-6);
}

TEST(ResizeCommand, MissingAxesDefaultToOne) {
    auto data = makeCube();
    CadObject object(data);
    EXPECT_TRUE(ResizeCommand().execute(nlohmann::ordered_json::object(), object).ok);
    EXPECT_NEAR(boundsOf(data)[1], 0.5, 1e-6);  // unchanged
}

TEST(ResizeCommand, RejectsZeroFactorWithoutTouchingGeometry) {
    auto data = makeCube();
    CadObject object(data);
    nlohmann::ordered_json params;
    params["x"] = 0.0;
    EXPECT_FALSE(ResizeCommand().execute(params, object).ok);
    EXPECT_NEAR(boundsOf(data)[1], 0.5, 1e-6);  // untouched
}

TEST(ResizeCommand, RejectsNonNumericParam) {
    auto data = makeCube();
    CadObject object(data);
    nlohmann::ordered_json params;
    params["x"] = "big";
    EXPECT_FALSE(ResizeCommand().execute(params, object).ok);
    EXPECT_NEAR(boundsOf(data)[1], 0.5, 1e-6);
}
