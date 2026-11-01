# GotlandRingUnreal

Full-circuit UE 5.8.4 migration, imported and rendered on EXCAVATOR: 7,214.397 m of the existing GotlandRing road with 3 m gravel aprons. Includes Unity’s existing meadow, quarry piles, woodland, building, barriers, turbines, markings and signs. Surrounding ground is an authored approximation, not surveyed terrain.

Build the C++ editor target, then open `GotlandRingUnreal.uproject` with UE 5.8; `/Game/Migration/Prototype` is the startup level. Saved Unreal assets are included under `Content/Migration`.

To rebuild:

1. Keep `GotlandRingUnity` beside this directory and run `python3 Scripts/export_scene.py` followed by `python3 Scripts/export_scenery.py` and `python3 Scripts/export_vehicle.py` here. Run `python3 Scripts/export_instruments.py` to export the original analogue cluster and artwork, and `python3 Scripts/export_cockpit.py` to regenerate the compiled racing line and source-controller fixtures. The scenery exporter uses the existing Unity editor in an isolated copy.
2. In Unreal's Python console run `exec(open(unreal.Paths.project_dir() + 'Scripts/import_scene.py').read())`.
3. For an automated 2560×1440 capture, launch the Editor with `-ExecutePythonScript=<absolute-project-path>/Scripts/render_scene.py`. Output: `Saved/GotlandRing.png`.

The importer replaces its generated level and assets. EXCAVATOR engine: `D:/UE_5.8`; project: `D:/Dev/GotlandRing/GotlandRingUnreal`.

See [MIGRATION_REPORT.md](MIGRATION_REPORT.md) for coordinates, commands, attribution and limitations, [validation](Validation/import_validation.json) for measured bounds, and [render](Renders/GotlandRing.png) for the actual Unreal image.

For a close-up of the saved scenery, use `Scripts/render_detail.py` as the ExecutePythonScript target. See [detail render](Renders/GotlandRingDetail.png) and [scenery validation](Validation/scenery_validation.json).

## Driving

Press Play in `/Game/Migration/Prototype`, then click the viewport, or run `Scripts/Drive.ps1` on EXCAVATOR. W/Up accelerates, S/Down brakes, X brakes forward travel then reverses, A/D or Left/Right steers, Space applies the rear handbrake, R recovers to the nearest road section, Home restarts at start/finish, M mutes all audio, and T toggles and saves tyre squeal (off until enabled). F1 or F3 opens the driving setup menu. C cycles cockpit, bonnet, chase and drone views. Mouse motion looks around; U switches to mouse steering and right-click recentres. P alone toggles autopilot; manual driving inputs do not cancel it. Escape opens the pause menu; F switches fullscreen/windowed, J hides the HUD, and F2 saves a screenshot under Saved. Forward throttle takes priority over X. The HUD shows speed, engine RPM, boost, gear, body sideslip, lap timing and a banking-coloured minimap. The original analogue cluster also appears in exterior camera views.

The native vehicle ports Unity’s independent flywheel, torque curve, automatic clutch, five forward ratios, reverse, shift delay/cooldown and throttle blips. It also ports the 50:50 AWD torque budget, traction control, ABS, combined-grip reduction, powered rear slip curve, doubled lift-off grip, off-road multiplier, anti-roll, drag and rolling resistance. Four rotating raycast tires apply these forces to Chaos. Tire contact/rotation is an Unreal solver implementation, not Unity’s proprietary WheelCollider solver; matching tuning does not establish identical handling or real-car fidelity. No detailed differential, tire temperature or wear model exists in the Unity source.

Engine audio uses the three existing owner-recorded Impreza cycles with the ported phase-locked mixer, RPM pitch, load crossfade/filtering, lift-off air, DC filtering and mute fades. The audio renderer owns its playback state; the game thread supplies engine RPM, load and speed. The cockpit uses the original camera anchors, hides the driver head, rotates the steering wheel, applies acceleration lean and road vibration, and displays live analogue RPM/speed needles, gear and trip distance on Unity’s Plesse cluster artwork. Autopilot follows Unity’s exported racing line and speed plan through ordinary steering/throttle/brake inputs, including its AWD sideslip/yaw correction. Unity’s load-weighted asphalt tyre squeal is included in the audio mixer, with its original pitch wander, noise filters, attack/release and low-speed gate. Airborne and loose-surface wheels contribute no squeal. The AWD setup menu includes grip, front torque share, low/high-speed steering angles, steering rate, traction control and tyre squeal. Apply saves to `Saved/DrivingSettings.json`; Cancel discards edits; Restore defaults changes the draft until Apply is pressed. The menu restores the prior pause state when closed. Resets and road recovery preserve the applied setup. Animated driver-hand IK, distinct kerb vibration and the arcade comparison controller remain outside this migration.

Build on EXCAVATOR:

```powershell
& D:/UE_5.8/Engine/Build/BatchFiles/Build.bat GotlandRingUnrealEditor Win64 Development -Project=D:/Dev/GotlandRing/GotlandRingUnreal/GotlandRingUnreal.uproject -WaitMutex
```

Validation:

- `Scripts/ValidateInstruments.ps1`: GPU pixel checks for idle/fast speed and RPM needles, full/low fuel, reverse gear and trip state; saves a cockpit capture plus 1200×400 cluster renders under `Saved`.

- `Scripts/ValidateSettings.ps1`: checks cancel/apply/defaults persistence, reset behavior, pause restoration, world-level drone positioning and disabled cockpit movement, and captures `Saved/GotlandRingSettings.png` and `Saved/GotlandRingDrone.png`. Uses a separate temporary settings file. Defaults to fractal’s `C:\UE_5.8`; pass `-EngineRoot D:\UE_5.8` on EXCAVATOR.

- `Scripts/ValidatePilot.ps1`: full-circuit autopilot run and cockpit/bonnet/chase captures; writes `Saved/pilot_validation.json` and telemetry. Source-controller parity: compile `Scripts/validate_pilot.cpp` with C++17 and run it with `Validation/pilot_reference.csv`.
- `Scripts/ValidateDriving.ps1`: rendered circuit collision, acceleration, braking, steering, reset and live audio check.
- `Scripts/ValidateDynamics.ps1`: temporary flat-pad acceleration/gear changes, braking torque cut, powered steering reversals, lift recovery and reverse. The pad exists only during the test and is not saved into the circuit.
- `Scripts/TestPortable.cmd`: MSVC tests against the original Unity engine/audio outputs, tire budgets, TC/ABS, downshift and reverse. It also compiles the sibling Unity project’s unmodified `ImprezaEngineMixer.cs` and compares tyre audio at 44.1, 48 and 96 kHz, including mute and surface/contact gates. Requires the sibling Unity checkout and Visual Studio C++ Build Tools. On macOS use `clang++ -std=c++17 -O2 -ffp-contract=off -I Source/GotlandRingUnreal Tests/dynamics.cpp -o /tmp/ring-dynamics-test`, then `/tmp/ring-dynamics-test Validation/UnityReference`.
- `Scripts/export_dynamics_reference.py`: regenerate reference outputs with the original Unity classes in the isolated staging project. `Scripts/export_engine_audio.py` regenerates compiled PCM cycles from the three original WAV files.

[Cockpit](Renders/GotlandRingCockpit.png) · [Bonnet](Renders/GotlandRingBonnet.png) · [Chase](Renders/GotlandRingChase.png) · [Autopilot lap](Validation/pilot_validation.json) · [Driving render](Renders/GotlandRingDriving.png) · [Engine audio capture](Renders/GotlandRingEngine.wav) · [Circuit check](Validation/driving_validation.json) · [Dynamics check](Validation/dynamics_validation.json) · [Source parity](Validation/portable_validation.txt)

The Subaru and reused cockpit/driver assets are **CC BY-NC 4.0**, separate from the code’s MIT license. Preserve [vehicle credits](VEHICLE_ASSET_CREDITS.md). The owner-recorded engine cycles are MIT-licensed; see [audio credits](ENGINE_AUDIO_CREDITS.md).

The drone camera uses Unity v0.3.4’s world-up follow pose and half-angle downward pitch. F1 camera settings save height (5–60 m, default 30), follow distance (5–120 m, default 40) and cockpit movement (0–1, default 0.5). Right-click recentres the view. These settings share Apply/Cancel/Restore defaults with the driving setup.

The analogue cluster uses the existing Unity mounting mesh and face texture. Fuel and coolant show Unity’s default values (full and midpoint); fuel consumption and coolant simulation are not implemented. Trip distance accumulates while driving for the current session. To update only the instruments, run `Scripts/export_instruments.py`, then run `Scripts/import_instruments.py` through Unreal Editor Python.

## Signs, minimap and menus

The 42 numbered Unity signs retain their original exported anchors, names and navy panels. At runtime, each of the 84 name/number labels is fitted to its Unity panel area. `Scripts/update_sign_material.py` updates the saved navy material and is also called by the scenery importer.

The minimap uses all 2,405 road sections and Unity’s surface banking formula, including width asymmetry. It displays the same coolwarm scale (±5.8°), start/finish checker and live car position. Lap timing uses ordered quarter-circuit gates; reversing, recovering and skipping gates do not award a completed lap. Restart clears the current timer and preserves the session best.

Escape opens Resume, Restart lap, Driving dynamics, View & sound, Drone camera, Keyboard help & setup, and Quit. Keyboard setup supports primary/alternate keys, conflict checks, unassigned keys, defaults, cancel and save to `Saved/KeyboardBindings.json`. Escape remains reserved for menu/back. Autopilot defaults to **P**. Driving, view/sound and drone settings share Apply/Cancel and persist across launches. The menu exposes the migrated features; Unity’s training, replay, paint selection and arcade mode are not yet included.

Regenerate the presentation data from the unchanged Unity checkout and existing scenery export:

```sh
python3 Scripts/export_presentation.py --scenery Export/Scenery/scenery.json
clang++ -std=c++17 -I Source/GotlandRingUnreal Tests/lap.cpp -o /tmp/ring-lap-test
/tmp/ring-lap-test
```

Run `Scripts/ValidatePresentation.ps1` on Windows, or launch the macOS game with `-RingPresentationTest -windowed -ResX=1600 -ResY=900`. It checks all sign bounds, minimap bounds, reserved/conflicting key assignments, persistence and pause-menu navigation, and writes `Saved/presentation_validation.json` plus HUD/menu/keyboard screenshots. `Scripts/ValidateSettings.ps1` remains the settings regression check.

## macOS build

Use `Scripts/PackageMac.sh <output-directory>` on Apple Silicon with UE 5.8.4 and Xcode installed. Set `UE_MAC_ROOT` if Unreal is outside `~/Epic Games/UE_5.8`. The script builds, cooks and packages the complete app, including its game data, and copies the asset credits. Launch `Mac/GotlandRingUnreal-Mac-Shipping.app` from the output directory. The local build targets macOS 14 or newer; signing/notarization for public distribution is separate.
