#!/usr/bin/env python3
"""Compile the real crop copy method against a recording D3D context without a GPU."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[4]
source = (root / "src/platform/windows/display_base.cpp").read_text()
start = source.index("  void display_base_t::copy_capture_region(")
end = source.index("  int display_base_t::init(", start)
production = source[start:end]
fakes = r'''
#include <cassert>
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
  recording_context *device_ctx;
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
  display.cropped = true;
  display.crop_offset_x = 3840;
  display.crop_offset_y = 20;
  display.width = 3840;
  display.height = 2140;
  display.copy_capture_region(&destination, &source);
  assert(context.full_copies == 1 && context.region_copies == 1);
  assert(context.region.left == 3840 && context.region.top == 20 && context.region.front == 0);
  assert(context.region.right == 7680 && context.region.bottom == 2160 && context.region.back == 1);
}
'''
with tempfile.TemporaryDirectory(prefix="cmake-build-capture-crop-") as directory:
    cpp = Path(directory) / "capture_copy.cpp"
    executable = Path(directory) / "capture_copy"
    cpp.write_text(fakes + production + main)
    subprocess.run(["g++", "-std=c++23", str(cpp), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("Full-frame and cropped D3D copy calls passed")
