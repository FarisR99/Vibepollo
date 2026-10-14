#!/usr/bin/env python3
"""Compile the real service argument functions against a deterministic command-line provider."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[4]
source = (root / "tools/sunshinesvc.cpp").read_text()
start = source.index("std::wstring GetForwardedArguments()")
end = source.index("DWORD WINAPI HandlerEx", start)
production = source[start:end]
fakes = r'''
#include <cassert>
#include <string>
std::wstring command_line;
std::wstring g_child_command_line = L"Sunshine.exe";
std::wstring g_log_file_leaf = L"sunshine.log";
const wchar_t *GetCommandLineW() { return command_line.c_str(); }
'''
main = r'''
int main() {
  command_line = L"sunshinesvc.exe";
  assert(GetForwardedArguments().empty());
  BuildChildInvocation();
  assert(g_child_command_line == L"Sunshine.exe");
  assert(g_log_file_leaf == L"sunshine.log");

  command_line = L"\"C:\\Program Files\\Vibepollo\\tools\\sunshinesvc.exe\"  \t--config-dir \"C:\\Players\\Player Two\" port=48989";
  const auto forwarded = GetForwardedArguments();
  assert(forwarded == L"--config-dir \"C:\\Players\\Player Two\" port=48989");
  BuildChildInvocation();
  assert(g_child_command_line == L"Sunshine.exe " + forwarded);
  assert(g_log_file_leaf == L"sunshine-Player Two.log");

  command_line = L"sunshinesvc.exe --config-dir=player-two port=48989";
  BuildChildInvocation();
  assert(g_log_file_leaf == L"sunshine-player-two.log");
  assert(ConfigDirLeaf(L"--config-dir=C:/Players/two/") == L"two");
  assert(ConfigDirLeaf(L"--config-dir C:\\Players\\two\\") == L"two");
  assert(ConfigDirLeaf(L"port=48989").empty());
  assert(ConfigDirLeaf(L"--config-dir ").empty());
  assert(ConfigDirLeaf(L"--config-dir=\"C:/Players/two\"") == L"two");
}
'''
with tempfile.TemporaryDirectory(prefix="cmake-build-service-instance-") as directory:
    cpp = Path(directory) / "service_args.cpp"
    executable = Path(directory) / "service_args"
    cpp.write_text(fakes + production + main)
    subprocess.run(["g++", "-std=c++23", str(cpp), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("Service argument forwarding and per-instance log naming passed")
