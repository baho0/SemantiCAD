#include "core/command/CommandDispatcher.h"
#include "core/command/entity/CadObject.h"

#include "CubeFixture.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace cmd = semanticad::core::command;
using cmd::CadObject;
using cmd::CommandDispatcher;
using semanticad::test::boundsOf;
using semanticad::test::makeCube;

TEST(CommandDispatcher, RegistersAllBuiltins) {
    CommandDispatcher dispatcher;
    EXPECT_TRUE(dispatcher.has("resize"));
    EXPECT_TRUE(dispatcher.has("translate"));
    EXPECT_TRUE(dispatcher.has("rotate"));
    EXPECT_TRUE(dispatcher.has("mirror"));
    const std::vector<std::string> expected{"mirror", "resize", "rotate", "translate"};
    EXPECT_EQ(dispatcher.names(), expected);
}

TEST(CommandDispatcher, RoutesToTheNamedCommand) {
    CommandDispatcher dispatcher;
    auto data = makeCube();
    CadObject object(data);
    nlohmann::ordered_json params;
    params["dx"] = 7.0;
    EXPECT_TRUE(dispatcher.dispatch("translate", params, object).ok);
    const auto b = boundsOf(data);
    EXPECT_NEAR(b[0], 6.5, 1e-6);
    EXPECT_NEAR(b[1], 7.5, 1e-6);
}

TEST(CommandDispatcher, UnknownNameFailsAndDoesNothing) {
    CommandDispatcher dispatcher;
    auto data = makeCube();
    CadObject object(data);
    for (const char* name : {"none", "error", "explode"}) {
        EXPECT_FALSE(dispatcher.dispatch(name, nlohmann::ordered_json::object(), object).ok)
            << name;
    }
    EXPECT_NEAR(boundsOf(data)[1], 0.5, 1e-6);  // geometry untouched
}
