/**
 * @file src/capture_crop.h
 * @brief Parsing and bounds checks for physical-pixel capture rectangles.
 */
#pragma once
#include <array>
#include <charconv>
#include <optional>
#include <string_view>

namespace config {
  /**
   * @brief Capture rectangle relative to an unrotated display's top-left corner.
   */
  struct capture_crop_t {
    int x;  ///< Horizontal source offset in pixels.
    int y;  ///< Vertical source offset in pixels.
    int width;  ///< Captured width in pixels.
    int height;  ///< Captured height in pixels.
  };

  /**
   * @brief Parse four comma-separated nonnegative integers with positive dimensions.
   * @param value Rectangle text; empty or whitespace-only text disables cropping.
   * @return Parsed rectangle, or no value for empty or malformed text.
   */
  inline std::optional<capture_crop_t> parse_capture_crop(std::string_view value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string_view::npos) {
      return std::nullopt;
    }
    value = value.substr(begin, value.find_last_not_of(" \t\r\n") - begin + 1);
    std::array<int, 4> parts {};
    for (std::size_t field = 0; field < parts.size(); ++field) {
      const auto comma = value.find(',');
      const auto token = value.substr(0, comma);
      const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), parts[field]);
      if (error != std::errc {} || end != token.data() + token.size() || parts[field] < 0) {
        return std::nullopt;
      }
      if (field == parts.size() - 1) {
        if (comma != std::string_view::npos) {
          return std::nullopt;
        }
      } else {
        if (comma == std::string_view::npos) {
          return std::nullopt;
        }
        value.remove_prefix(comma + 1);
      }
    }
    if (parts[2] == 0 || parts[3] == 0) {
      return std::nullopt;
    }
    return capture_crop_t {parts[0], parts[1], parts[2], parts[3]};
  }

  /**
   * @brief Check source bounds using subtraction to avoid coordinate-sum overflow.
   * @param crop Requested capture rectangle.
   * @param width Full source width in pixels.
   * @param height Full source height in pixels.
   * @return Whether the entire nonempty rectangle fits in the source.
   */
  inline bool capture_crop_fits(const capture_crop_t &crop, int width, int height) {
    return crop.x >= 0 && crop.y >= 0 && crop.width > 0 && crop.height > 0 &&
           crop.x <= width && crop.y <= height &&
           crop.width <= width - crop.x && crop.height <= height - crop.y;
  }
}  // namespace config
