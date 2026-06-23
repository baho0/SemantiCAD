#include "core/command/entity/CadObject.h"
#include "core/command/translate/TranslateCommand.h"
#include "core/command/translate/TranslateService.h"
#include "core/command/translate/Translation.h"

#include "CubeFixture.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cmath>

namespace cmd = semanticad::core::command;
using cmd::CadObject;
using cmd::translate::TranslateCommand;
using cmd::translate::TranslateService;
using cmd::translate::Translation;
using semanticad::test::boundsOf;
using semanticad::test::makeCube;

TEST(Translation, RejectsNonFinite) {
    EXPECT_FALSE(Translation::create(NAN, 0.0, 0.0).has_value());
    EXPECT_TRUE(Translation::create(-20.0, 0.0, 5.0).has_value());
}

TEST(TranslateService, MovesGeometry) {
    auto data = makeCube();
    CadObject object(data);
    TranslateService::apply(*Translation::create(10.0, -20.0, 0.0), object);
    const auto b = boundsOf(data);
    EXPECT_NEAR(b[0], 9.5, 1e-6);
    EXPECT_NEAR(b[1], 10.5, 1e-6);
    EXPECT_NEAR(b[2], -20.5, 1e-6);
    EXPECT_NEAR(b[3], -19.5, 1e-6);
}

TEST(TranslateCommand, HasName) { EXPECT_EQ(TranslateCommand().name(), "translate"); }

TEST(TranslateCommand, ParsesDeltas) {
    auto data = makeCube();
    CadObject object(data);
    nlohmann::ordered_json params;
    params["dx"] = 10.0;
    EXPECT_TRUE(TranslateCommand().execute(params, object).ok);
    EXPECT_NEAR(boundsOf(data)[1], 10.5, 1e-6);
}

TEST(TranslateCommand, MissingDeltasDefaultToZero) {
    auto data = makeCube();
    CadObject object(data);
    EXPECT_TRUE(TranslateCommand().execute(nlohmann::ordered_json::object(), object).ok);
    EXPECT_NEAR(boundsOf(data)[1], 0.5, 1e-6);  // unchanged
}
