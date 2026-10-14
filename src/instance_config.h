/**
 * @file src/instance_config.h
 * @brief Process-wide configuration directory selection for independent host instances.
 */
#pragma once
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace platf {
  /**
   * @brief Access the process-wide override for the app-data (config/state) directory.
   * @details Empty means "use the platform default". When non-empty, appdata() returns this path
   *          instead, letting multiple Sunshine instances share one executable while each uses its
   *          own config/apps.json/credentials/state/log directory. The slot is seeded once from the
   *          `SUNSHINE_CONFIG_DIR` environment variable on first access, and may be overwritten via
   *          set_appdata_dir() (e.g. from the `--config-dir` CLI flag). Defined inline so a single
   *          instance is shared across all translation units.
   * @return Reference to the mutable override slot.
   */
  inline std::filesystem::path &appdata_override() {
    static std::filesystem::path override_path = []() -> std::filesystem::path {
      if (const char *env = std::getenv("SUNSHINE_CONFIG_DIR"); env && *env) {
        return std::filesystem::path {env};
      }
      return {};
    }();
    return override_path;
  }

  /**
   * @brief Set the app-data directory override consulted by appdata().
   * @param dir Directory that should hold this instance's config/state, or empty to reset to default.
   */
  inline void set_appdata_dir(const std::filesystem::path &dir) {
    appdata_override() = dir;
  }

  /**
   * @brief Result of recognizing one instance-directory command-line option.
   */
  struct config_directory_argument_t {
    bool matched;  ///< Whether the argument is an instance-directory option.
    bool consumes_next;  ///< Whether the next command-line argument supplies the value.
    std::optional<std::string_view> directory;  ///< Nonempty value, or no value for an invalid option.
  };

  /**
   * @brief Recognize both supported config-directory CLI forms before subcommand parsing.
   * @param argument Current command-line token.
   * @param next Next token, or empty when no next token exists.
   * @return Match status, token consumption, and the directory value.
   */
  inline config_directory_argument_t parse_config_directory_argument(std::string_view argument, std::string_view next = {}) {
    constexpr std::string_view prefix {"--config-dir="};
    if (argument.starts_with(prefix)) {
      const auto value = argument.substr(prefix.size());
      return {true, false, value.empty() ? std::nullopt : std::optional {value}};
    }
    if (argument == "--config-dir") {
      return {true, true, next.empty() ? std::nullopt : std::optional {next}};
    }
    return {false, false, std::nullopt};
  }

  /**
   * @brief Process namespace forwarded to display helpers; empty retains primary names.
   */
  inline std::string instance_resource_scope;

  /**
   * @brief Apply the directory and decimal IPC namespace passed from a host to its helper.
   * @param argc Number of command-line arguments.
   * @param argv Command-line tokens supplied to either display helper engine.
   */
  inline void configure_helper_instance(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
      const std::string_view argument {argv[i]};
      if (const auto option = parse_config_directory_argument(argument, i + 1 < argc ? argv[i + 1] : ""); option.matched) {
        if (option.directory) {
          set_appdata_dir(std::filesystem::absolute(std::string {*option.directory}));
          if (option.consumes_next) {
            ++i;
          }
        }
      } else if (argument.starts_with("--instance-id=")) {
        const auto id = argument.substr(std::string_view {"--instance-id="}.size());
        if (!id.empty() && id.find_first_not_of("0123456789") == std::string_view::npos) {
          instance_resource_scope = id;
        }
      }
    }
  }

  /**
   * @brief Add this host's resource namespace to a named IPC resource.
   * @param base Existing primary resource name.
   * @return Primary name, or a name isolated to this host process.
   */
  inline std::string instance_resource_name(std::string_view base) {
    return instance_resource_scope.empty() ? std::string(base) :
                                             std::string(base) + "-" + instance_resource_scope;
  }

  /**
   * @brief Restrict helper cleanup to processes created by the current host.
   * @param parent Process ID of the helper's parent.
   * @param host Process ID of the host performing cleanup.
   * @return Whether the helper belongs to this host.
   */
  inline bool owns_instance_helper(unsigned long parent, unsigned long host) {
    return parent == host;
  }
}  // namespace platf
