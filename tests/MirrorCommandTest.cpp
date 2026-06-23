#include "core/command/entity/CadObject.h"
#include "core/command/mirror/MirrorCommand.h"
#include "core/command/mirror/MirrorPlane.h"
#include "core/command/mirror/MirrorService.h"

#include "CubeFixture.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace cmd = semanticad::core::command;
using cmd::CadObject;
using cmd::mirror::MirrorCommand;
using cmd::mirror::MirrorPlane;
using cmd::mirror::MirrorService;
using cmd::mirror::Plane;
using semanticad::test::boundsOf;
using semanticad::test::makeCube;

TEST(MirrorPlane, ParsesKnownPlanesAndRejectsOthers) {
    EXPECT_EQ(MirrorPlane::create("xy")->plane(), Plane::XY);
    EXPECT_EQ(MirrorPlane::create("yz")->plane(), Plane::YZ);
    EXPECT_EQ(MirrorPlane::create("xz")->plane(), Plane::XZ);
    EXPECT_FALSE(MirrorPlane::create("zz").has_value());
}

TEST(MirrorService, ReflectsAcrossThePlane) {
    {  // yz flips x: a cube at x=+10 lands at x=-10
        auto data = makeCube(10.0, 0.0, 0.0);  // x bounds [9.5, 10.5]
        CadObject object(data);
        MirrorService::apply(*MirrorPlane::create("yz"), object);
        const auto b = boundsOf(data);
        EXPECT_NEAR(b[0], -10.5, 1e-6);
        EXPECT_NEAR(b[1], -9.5, 1e-6);
    }
    {  // xy flips z
        auto data = makeCube(0.0, 0.0, 10.0);  // z bounds [9.5, 10.5]
        CadObject object(data);
        MirrorService::apply(*MirrorPlane::create("xy"), object);
        const auto b = boundsOf(data);
        EXPECT_NEAR(b[4], -10.5, 1e-6);
        EXPECT_NEAR(b[5], -9.5, 1e-6);
    }
}

TEST(MirrorCommand, HasName) { EXPECT_EQ(MirrorCommand().name(), "mirror"); }

TEST(MirrorCommand, ParsesPlane) {
    auto data = makeCube(10.0, 0.0, 0.0);
    CadObject object(data);
    nlohmann::ordered_json params;
    params["plane"] = "yz";
    EXPECT_TRUE(MirrorCommand().execute(params, object).ok);
    EXPECT_NEAR(boundsOf(data)[0], -10.5, 1e-6);
}

TEST(MirrorCommand, DefaultsToXyPlane) {
    auto data = makeCube(0.0, 0.0, 10.0);
    CadObject object(data);
    EXPECT_TRUE(MirrorCommand().execute(nlohmann::ordered_json::object(), object).ok);
    EXPECT_NEAR(boundsOf(data)[4], -10.5, 1e-6);
}

TEST(MirrorCommand, RejectsUnknownPlane) {
    auto data = makeCube(10.0, 0.0, 0.0);
    CadObject object(data);
    nlohmann::ordered_json params;
    params["plane"] = "zz";
    EXPECT_FALSE(MirrorCommand().execute(params, object).ok);
    EXPECT_NEAR(boundsOf(data)[1], 10.5, 1e-6);  // untouched
}
