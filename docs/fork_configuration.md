# Personal fork configuration

## Separate instances

Run the same executable with a different configuration root and base port:

```bash
sunshine --config-dir /absolute/path/player-2 port=48989
# Equivalent directory selection:
SUNSHINE_CONFIG_DIR=/absolute/path/player-2 sunshine port=48989
```

The CLI directory takes precedence over the environment. Both `--config-dir <path>` and
`--config-dir=<path>` are accepted. A positional configuration filename remains explicit.
Windows uses `sunshine.conf`; Linux uses `vibepollo.conf`. Each directory contains its own
apps.json, credentials, state, and logs. Existing absolute paths inside configuration files
remain explicit; remove them or update them when copying a configuration to another instance.
Assign each instance a unique host name, ports, and client pairings.

From an elevated Windows prompt, register a second service using the scripts shipped under `scripts`:

```bat
install-sunshine-instance.bat --name ApolloServiceP2 --config-dir "C:\Vibepollo\player-2" --port 48989 --firewall
uninstall-sunshine-instance.bat --name ApolloServiceP2 --firewall
```

Pass `--svc "C:\Program Files\Vibepollo\tools\sunshinesvc.exe"` if automatic discovery fails.
The scripts require the patched service host built from this fork. They do not reinstall the
primary application. Use `--no-start` to register without starting the new service.

## Capture crops

Choose a physical display in the existing application display selector. Under Display,
set **Capture Crop Rect** to `x,y,width,height`, for example `3840,0,3840,2160`.
Application JSON uses `"capture-crop": "3840,0,3840,2160"`. General per-app
`"config-overrides": {"capture_crop": "3840,0,3840,2160"}` and the instance-level
`capture_crop` config key are also supported. Existing runtime config overrides manage
launch, resume, and restoration. An explicit general `capture_crop` override takes precedence.

The rectangle must fit completely inside the source. Width and height must be positive;
coordinates must be nonnegative. Empty input captures the full output. Malformed or out-of-bounds
rectangles, and cropping on rotated displays, are ignored by native capture.

Cropping is supported on Windows DDA and service-mode WGC with RAM or VRAM encoding.
Source-size change detection still checks the complete output. Captured buffers use the crop's
size, cursor rendering is shifted, and absolute mouse input targets the cropped desktop region.
Linux and macOS retain full-output capture. Use even dimensions for encoder compatibility.

## Existing Vibepollo equivalents

Use Vibepollo's existing per-app display routing and controller selection. Per-app
`config-overrides` already supports `keyboard`, `mouse`, `controller`, and `gamepad`, including
restoration on application exit. For example:

```json
{
  "name": "Player 2",
  "display-output": "\\\\.\\DISPLAY1",
  "capture-crop": "3840,0,3840,2160",
  "gamepad": "ds4",
  "config-overrides": {"keyboard": "disabled", "mouse": "disabled"}
}
```

For split-screen games, launch one game across a combined NVIDIA Surround display and stream a
different crop from each instance. Use the [Surround tool](../contrib/nvidia-surround-toggle/README.md)
in preparation commands. Display helper pipes, mutexes, cleanup, and snapshots are isolated per host instance.
Windows display topology and physical input remain shared: let one
instance manage the topology, and disable display automation in the other instances. Avoid
independent virtual-display creation or layout restoration while another instance is streaming.

Vibepollo's MSI packaging is its primary Windows distribution. Sunshine's optional-WiX fallback
was intentionally not ported: silently skipping WiX would remove Vibepollo's main installer.
Vibepollo also already provides session log management; its logger is preserved.
