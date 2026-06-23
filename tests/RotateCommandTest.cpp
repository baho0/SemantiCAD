#include "core/command/entity/CadObject.h"
#include "core/command/rotate/RotateCommand.h"
#include "core/command/rotate/RotateService.h"
#include "core/command/rotate/Rotation.h"

#include "CubeFixture.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cmath>

namespace cmd = semanticad::core::command;
using cmd::CadObject;
using cmd::rotate::Axis;
using cmd::rotate::RotateCommand;
using cmd::rotate::RotateService;
using cmd::rotate::Rotation;
using semanticad::test::boundsOf;
using semanticad::test::makeCube;

// Half-extent of a unit cube after a 45° rotation: 0.5*(|cos45|+|sin45|).
static const double kRot45HalfExtent = 0.5 * std::sqrt(2.0);

TEST(Rotation, ParsesKnownAxesAndRejectsOthers) {
    EXPECT_EQ(Rotation::parseAxis("x"), Axis::X);
    EXPECT_EQ(Rotation::parseAxis("y"), Axis::Y);
    EXPECT_EQ(Rotation::parseAxis("z"), Axis::Z);
    EXPECT_FALSE(Rotation::parseAxis("w").has_value());
    EXPECT_FALSE(Rotation::parseAxis("X").has_value());
}

TEST(Rotation, RejectsNonFiniteAngle) {
    EXPECT_FALSE(Rotation::create(Axis::Z, INFINITY).has_value());
}

TEST(RotateService, RotatesAboutTheGivenAxis) {
    {  // X axis: y and z grow, x is unchanged
        auto data = makeCube();
        CadObject object(data);
        RotateService::apply(*Rotation::create(Axis::X, 45.0), object);
        const auto b = boundsOf(data);
        EXPECT_NEAR(b[1], 0.5, 1e-6);
        EXPECT_NEAR(b[3], kRot45HalfExtent, 1e-6);
        EXPECT_NEAR(b[5], kRot45HalfExtent, 1e-6);
    }
    {  // Z axis: x and y grow, z is unchanged
        auto data = makeCube();
        CadObject object(data);
        RotateService::apply(*Rotation::create(Axis::Z, 45.0), object);
        const auto b = boundsOf(data);
        EXPECT_NEAR(b[1], kRot45HalfExtent, 1e-6);
        EXPECT_NEAR(b[3], kRot45HalfExtent, 1e-6);
        EXPECT_NEAR(b[5], 0.5, 1e-6);
    }
}

TEST(RotateCommand, HasName) { EXPECT_EQ(RotateCommand().name(), "rotate"); }

TEST(RotateCommand, ParsesAxisAndAngle) {
    auto data = makeCube();
    CadObject object(data);
    nlohmann::ordered_json params;
    params["axis"] = "x";
    params["angle_deg"] = 45.0;
    EXPECT_TRUE(RotateCommand().execute(params, object).ok);
    const auto b = boundsOf(data);
    EXPECT_NEAR(b[1], 0.5, 1e-6);                  // x unchanged -> rotated about x
    EXPECT_NEAR(b[3], kRot45HalfExtent, 1e-6);
}

TEST(RotateCommand, DefaultsToZAxis) {
    auto data = makeCube();
    CadObject object(data);
    nlohmann::ordered_json params;
    params["angle_deg"] = 45.0;
    EXPECT_TRUE(RotateCommand().execute(params, object).ok);
    const auto b = boundsOf(data);
    EXPECT_NEAR(b[5], 0.5, 1e-6);                  // z unchanged -> rotated about z
    EXPECT_NEAR(b[1], kRot45HalfExtent, 1e-6);
}

TEST(RotateCommand, RejectsUnknownAxis) {
    auto data = makeCube();
    CadObject object(data);
    nlohmann::ordered_json params;
    params["axis"] = "w";
    EXPECT_FALSE(RotateCommand().execute(params, object).ok);
    EXPECT_NEAR(boundsOf(data)[1], 0.5, 1e-6);     // untouched
}
