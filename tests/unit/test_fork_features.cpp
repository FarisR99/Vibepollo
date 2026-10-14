/**
 * @file tests/unit/test_fork_features.cpp
 * @brief Hardware-independent tests for crop selection and host instance isolation.
 */
#include <climits>
#include <gtest/gtest.h>
#include <src/capture_crop.h>
#include <src/instance_config.h>

TEST(CaptureCrop, AcceptsFullOffsetWhitespaceAndIntegerBoundaryRectangles) {
  for (const auto *value : {"0,0,3840,2160", "3840,0,3840,2160", " \t0,0,1,1\r\n", "2147483647,0,1,1"}) {
    EXPECT_TRUE(config::parse_capture_crop(value)) << value;
  }
  const auto crop = config::parse_capture_crop("3840,20,3840,2160");
  ASSERT_TRUE(crop);
  EXPECT_EQ(crop->x, 3840);
  EXPECT_EQ(crop->y, 20);
  EXPECT_EQ(crop->width, 3840);
  EXPECT_EQ(crop->height, 2160);
}

TEST(CaptureCrop, RejectsDisabledMalformedNegativeOverflowAndZeroAreaRectangles) {
  for (const auto *value : {"", " \t\r\n", "0", "0,0,1", "0,0,1,1,2", "0,0,1,1,", "0,0,0,1", "0,0,1,0", "-1,0,1,1", "0,-1,1,1", "0,0,-1,1", "+1,0,1,1", "0, 0,1,1", "0,0,1.5,1", "0,0,1,", ",0,1,1", "2147483648,0,1,1"}) {
    EXPECT_FALSE(config::parse_capture_crop(value)) << value;
  }
}

TEST(CaptureCrop, ChecksEveryBoundaryWithoutCoordinateSumOverflow) {
  EXPECT_TRUE(config::capture_crop_fits({0, 0, 7680, 2160}, 7680, 2160));
  EXPECT_TRUE(config::capture_crop_fits({3840, 0, 3840, 2160}, 7680, 2160));
  EXPECT_TRUE(config::capture_crop_fits({INT_MAX - 1, INT_MAX - 1, 1, 1}, INT_MAX, INT_MAX));
  for (const auto crop : {config::capture_crop_t {-1, 0, 1, 1}, {0, -1, 1, 1}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 0, -1, 1}, {0, 0, 1, -1}, {11, 0, 1, 1}, {0, 11, 1, 1}, {10, 0, 1, 1}, {0, 10, 1, 1}, {0, 0, 11, 1}, {0, 0, 1, 11}, {INT_MAX, 0, INT_MAX, 1}, {0, INT_MAX, 1, INT_MAX}}) {
    EXPECT_FALSE(config::capture_crop_fits(crop, 10, 10));
  }
}

TEST(InstanceDirectory, RecognizesEqualsAndSeparateArgumentsAndRejectsMissingValues) {
  const auto separate = platf::parse_config_directory_argument("--config-dir", "player two");
  EXPECT_TRUE(separate.matched);
  EXPECT_TRUE(separate.consumes_next);
  EXPECT_EQ(separate.directory, "player two");
  const auto equal = platf::parse_config_directory_argument("--config-dir=player two");
  EXPECT_TRUE(equal.matched);
  EXPECT_FALSE(equal.consumes_next);
  EXPECT_EQ(equal.directory, "player two");
  for (const auto *value : {"--config-dir", "--config-dir="}) {
    const auto option = platf::parse_config_directory_argument(value);
    EXPECT_TRUE(option.matched);
    EXPECT_FALSE(option.directory);
  }
  const auto other = platf::parse_config_directory_argument("--creds", "user");
  EXPECT_FALSE(other.matched);
  EXPECT_FALSE(other.consumes_next);
  EXPECT_FALSE(other.directory);
}

TEST(InstanceDirectory, ExplicitSelectionOverridesEnvironmentAndCanReset) {
  const auto original = platf::appdata_override();
  // The test runner supplies this variable before the override's first access.
  if (const auto *environment = std::getenv("SUNSHINE_CONFIG_DIR"); environment && *environment) {
    EXPECT_EQ(original, std::filesystem::path {environment});
  } else {
    EXPECT_TRUE(original.empty());
  }
  platf::set_appdata_dir("player-two");
  EXPECT_EQ(platf::appdata_override(), "player-two");
  platf::set_appdata_dir({});
  EXPECT_TRUE(platf::appdata_override().empty());
  platf::set_appdata_dir(original);
}

TEST(InstanceResources, PreservePrimaryNamesAndIsolateHelpersOfDifferentHosts) {
  const auto original = platf::instance_resource_scope;
  platf::instance_resource_scope.clear();
  EXPECT_EQ(platf::instance_resource_name("sunshine_display_helper"), "sunshine_display_helper");
  platf::instance_resource_scope = "101";
  EXPECT_EQ(platf::instance_resource_name("sunshine_display_helper"), "sunshine_display_helper-101");
  platf::instance_resource_scope = "202";
  EXPECT_EQ(platf::instance_resource_name("sunshine_display_helper"), "sunshine_display_helper-202");
  EXPECT_TRUE(platf::owns_instance_helper(101, 101));
  EXPECT_FALSE(platf::owns_instance_helper(101, 202));
  platf::instance_resource_scope = original;
}

TEST(InstanceResources, HelperArgumentsSelectTheHostDirectoryAndRejectUnsafeNamespaces) {
  const auto original_directory = platf::appdata_override();
  const auto original_scope = platf::instance_resource_scope;
  char program[] = "helper";
  char option[] = "--config-dir";
  char directory[] = "player two";
  char id[] = "--instance-id=101";
  char other[] = "--engine=v2";
  char *arguments[] = {program, option, directory, id, other};
  platf::configure_helper_instance(5, arguments);
  EXPECT_EQ(platf::appdata_override(), std::filesystem::absolute(directory));
  EXPECT_EQ(platf::instance_resource_scope, "101");
  char equals[] = "--config-dir=player three";
  char empty[] = "--instance-id=";
  char unsafe[] = "--instance-id=101/other";
  char missing[] = "--config-dir=";
  char *invalid[] = {program, equals, empty, unsafe, missing, option};
  platf::configure_helper_instance(6, invalid);
  EXPECT_EQ(platf::appdata_override(), std::filesystem::absolute("player three"));
  EXPECT_EQ(platf::instance_resource_scope, "101");
  platf::set_appdata_dir(original_directory);
  platf::instance_resource_scope = original_scope;
}
