#!/usr/bin/env python3
"""Exercise production crop initialization and texture copies without a GPU."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[4]
source = (root / "src/platform/windows/display_base.cpp").read_text()
start = source.index("  void display_base_t::copy_capture_region(")
end = source.index("  int display_base_t::init(", start)
production = source[start:end]
init = source[end:source.index("  std::optional<adapter_id_t> display_base_t::capture_adapter_id()", end)]
assert init.index("output_monitor =") < init.index("apply_capture_crop();")
probe = source[source.index("  bool test_dxgi_duplication("):start]
assert "full_width =" not in probe
assert "apply_capture_crop()" not in probe
fakes = r'''
#include <cassert>
#include <sstream>
#include <string>
#include "src/capture_crop.h"
#define BOOST_LOG(level) std::ostringstream()
constexpr int DXGI_MODE_ROTATION_UNSPECIFIED = 0;
constexpr int DXGI_MODE_ROTATION_IDENTITY = 1;
namespace config {
  struct { std::string capture_crop; } video;
}
using UINT = unsigned int;
struct ID3D11Texture2D {};
struct D3D11_BOX { UINT left, top, front, right, bottom, back; };
struct recording_context {
  ID3D11Texture2D *expected_source, *expected_destination;
  int full_copies = 0, region_copies = 0;
  D3D11_BOX region {};
  void CopyResource(ID3D11Texture2D *destination, ID3D11Texture2D *source) {
    assert(destination == expected_destination && source == expected_source);
    ++full_copies;
  }
  void CopySubresourceRegion(ID3D11Texture2D *destination, UINT subresource, UINT x, UINT y, UINT z,
                            ID3D11Texture2D *source, UINT source_subresource, const D3D11_BOX *box) {
    assert(destination == expected_destination && source == expected_source);
    assert(subresource == 0 && source_subresource == 0 && x == 0 && y == 0 && z == 0);
    region = *box;
    ++region_copies;
  }
};
struct display_base_t {
  bool cropped = false;
  int crop_offset_x = 0, crop_offset_y = 0, width = 7680, height = 2160;
  int full_width = 0, full_height = 0, offset_x = 100, offset_y = 200;
  int display_rotation = DXGI_MODE_ROTATION_IDENTITY;
  recording_context *device_ctx;
  void apply_capture_crop();
  void copy_capture_region(ID3D11Texture2D *, ID3D11Texture2D *);
};
'''
main = r'''
int main() {
  ID3D11Texture2D source, destination;
  recording_context context {&source, &destination};
  display_base_t display;
  display.device_ctx = &context;
  display.copy_capture_region(&destination, &source);
  assert(context.full_copies == 1 && context.region_copies == 0);
  display.apply_capture_crop();
  assert(!display.cropped && display.full_width == 7680 && display.full_height == 2160);
  assert(display.offset_x == 100 && display.offset_y == 200);
  config::video.capture_crop = "3840,20,3840,2140";
  display.apply_capture_crop();
  assert(display.cropped && display.width == 3840 && display.height == 2140);
  assert(display.full_width == 7680 && display.full_height == 2160);
  assert(display.offset_x == 3940 && display.offset_y == 220);
  display.copy_capture_region(&destination, &source);
  assert(context.full_copies == 1 && context.region_copies == 1);
  assert(context.region.left == 3840 && context.region.top == 20 && context.region.front == 0);
  assert(context.region.right == 7680 && context.region.bottom == 2160 && context.region.back == 1);
  for (int rotation : {DXGI_MODE_ROTATION_UNSPECIFIED, DXGI_MODE_ROTATION_IDENTITY, 2, 3, 4}) {
    for (const auto *crop : {"", "invalid", "3840,20,3840,2140", "7680,0,1,1", "0,0,2147483647,1"}) {
      display_base_t candidate;
      candidate.display_rotation = rotation;
      candidate.cropped = true;
      candidate.crop_offset_x = 50;
      candidate.crop_offset_y = 60;
      config::video.capture_crop = crop;
      candidate.apply_capture_crop();
      const bool expected = rotation <= DXGI_MODE_ROTATION_IDENTITY &&
        config::video.capture_crop == "3840,20,3840,2140";
      assert(candidate.cropped == expected);
      assert(candidate.full_width == 7680 && candidate.full_height == 2160);
      assert(candidate.width == (expected ? 3840 : 7680));
      assert(candidate.height == (expected ? 2140 : 2160));
      assert(candidate.crop_offset_x == (expected ? 3840 : 0));
      assert(candidate.crop_offset_y == (expected ? 20 : 0));
      assert(candidate.offset_x == (expected ? 3940 : 100));
      assert(candidate.offset_y == (expected ? 220 : 200));
    }
  }
}
'''
with tempfile.TemporaryDirectory(prefix="cmake-build-capture-crop-") as directory:
    cpp = Path(directory) / "capture_copy.cpp"
    executable = Path(directory) / "capture_copy"
    cpp.write_text(fakes + production + main)
    subprocess.run(["g++", "-std=c++23", "-I", str(root), str(cpp), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("Crop initialization, rotation/bounds rejection, and D3D copy calls passed")
