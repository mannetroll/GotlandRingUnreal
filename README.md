# GotlandRingUnreal

Full-circuit UE 5.8.4 migration, imported and rendered on EXCAVATOR: 7,214.397 m of the existing GotlandRing road with 3 m gravel aprons. Includes Unity’s existing meadow, quarry piles, woodland, building, barriers, turbines, markings and signs. Surrounding ground is an authored approximation, not surveyed terrain.

Build the C++ editor target, then open `GotlandRingUnreal.uproject` with UE 5.8; `/Game/Migration/Prototype` is the startup level. Saved Unreal assets are included under `Content/Migration`.

To rebuild:

1. Keep `GotlandRingUnity` beside this directory and run `python3 Scripts/export_scene.py` followed by `python3 Scripts/export_scenery.py` and `python3 Scripts/export_vehicle.py` here. Run `python3 Scripts/export_cockpit.py` to regenerate the compiled racing line and source-controller fixtures. The scenery exporter uses the existing Unity editor in an isolated copy.
2. In Unreal's Python console run `exec(open(unreal.Paths.project_dir() + 'Scripts/import_scene.py').read())`.
3. For an automated 2560×1440 capture, launch the Editor with `-ExecutePythonScript=<absolute-project-path>/Scripts/render_scene.py`. Output: `Saved/GotlandRing.png`.

The importer replaces its generated level and assets. EXCAVATOR engine: `D:/UE_5.8`; project: `D:/Dev/GotlandRing/GotlandRingUnreal`.

See [MIGRATION_REPORT.md](MIGRATION_REPORT.md) for coordinates, commands, attribution and limitations, [validation](Validation/import_validation.json) for measured bounds, and [render](Renders/GotlandRing.png) for the actual Unreal image.

For a close-up of the saved scenery, use `Scripts/render_detail.py` as the ExecutePythonScript target. See [detail render](Renders/GotlandRingDetail.png) and [scenery validation](Validation/scenery_validation.json).

## Driving

Press Play in `/Game/Migration/Prototype`, then click the viewport, or run `Scripts/Drive.ps1` on EXCAVATOR. W/Up accelerates, S/Down brakes, X brakes forward travel then reverses, A/D or Left/Right steers, Space applies the rear handbrake, R recovers to the nearest road section, Home restarts at start/finish, M mutes all audio, and T toggles tyre squeal (off by default). C cycles cockpit, bonnet and chase views. Mouse motion looks around; U switches to mouse steering and right-click recentres. P alone toggles autopilot; manual driving inputs do not cancel it. Escape freezes/resumes the car, and F2 saves a screenshot under Saved. Forward throttle takes priority over X. The HUD shows speed, engine RPM, gear, tire contacts and body sideslip.

The native vehicle ports Unity’s independent flywheel, torque curve, automatic clutch, five forward ratios, reverse, shift delay/cooldown and throttle blips. It also ports the 50:50 AWD torque budget, traction control, ABS, combined-grip reduction, powered rear slip curve, doubled lift-off grip, off-road multiplier, anti-roll, drag and rolling resistance. Four rotating raycast tires apply these forces to Chaos. Tire contact/rotation is an Unreal solver implementation, not Unity’s proprietary WheelCollider solver; matching tuning does not establish identical handling or real-car fidelity. No detailed differential, tire temperature or wear model exists in the Unity source.

Engine audio uses the three existing owner-recorded Impreza cycles with the ported phase-locked mixer, RPM pitch, load crossfade/filtering, lift-off air, DC filtering and mute fades. The audio renderer owns its playback state; the game thread supplies engine RPM, load and speed. The cockpit uses the original camera anchors, hides the driver head, rotates the steering wheel, applies acceleration lean and road vibration, and displays live RPM/speed/gear on the source LCD shader port. Autopilot follows Unity’s exported racing line and speed plan through ordinary steering/throttle/brake inputs, including its AWD sideslip/yaw correction. Unity’s load-weighted asphalt tyre squeal is included in the audio mixer, with its original pitch wander, noise filters, attack/release and low-speed gate. Airborne and loose-surface wheels contribute no squeal. Animated driver-hand IK, distinct kerb vibration and menu settings remain outside this migration.

Build on EXCAVATOR:

```powershell
& D:/UE_5.8/Engine/Build/BatchFiles/Build.bat GotlandRingUnrealEditor Win64 Development -Project=D:/Dev/GotlandRing/GotlandRingUnreal/GotlandRingUnreal.uproject -WaitMutex
```

Validation:

- `Scripts/ValidatePilot.ps1`: full-circuit autopilot run and cockpit/bonnet/chase captures; writes `Saved/pilot_validation.json` and telemetry. Source-controller parity: compile `Scripts/validate_pilot.cpp` with C++17 and run it with `Validation/pilot_reference.csv`.
- `Scripts/ValidateDriving.ps1`: rendered circuit collision, acceleration, braking, steering, reset and live audio check.
- `Scripts/ValidateDynamics.ps1`: temporary flat-pad acceleration/gear changes, braking torque cut, powered steering reversals, lift recovery and reverse. The pad exists only during the test and is not saved into the circuit.
- `Scripts/TestPortable.cmd`: MSVC tests against the original Unity engine/audio outputs, tire budgets, TC/ABS, downshift and reverse. It also compiles the sibling Unity project’s unmodified `ImprezaEngineMixer.cs` and compares tyre audio at 44.1, 48 and 96 kHz, including mute and surface/contact gates. Requires the sibling Unity checkout and Visual Studio C++ Build Tools. On macOS use `clang++ -std=c++17 -O2 -ffp-contract=off -I Source/GotlandRingUnreal Tests/dynamics.cpp -o /tmp/ring-dynamics-test`, then `/tmp/ring-dynamics-test Validation/UnityReference`.
- `Scripts/export_dynamics_reference.py`: regenerate reference outputs with the original Unity classes in the isolated staging project. `Scripts/export_engine_audio.py` regenerates compiled PCM cycles from the three original WAV files.

[Cockpit](Renders/GotlandRingCockpit.png) · [Bonnet](Renders/GotlandRingBonnet.png) · [Chase](Renders/GotlandRingChase.png) · [Autopilot lap](Validation/pilot_validation.json) · [Driving render](Renders/GotlandRingDriving.png) · [Engine audio capture](Renders/GotlandRingEngine.wav) · [Circuit check](Validation/driving_validation.json) · [Dynamics check](Validation/dynamics_validation.json) · [Source parity](Validation/portable_validation.txt)

The Subaru and reused cockpit/driver assets are **CC BY-NC 4.0**, separate from the code’s MIT license. Preserve [vehicle credits](VEHICLE_ASSET_CREDITS.md). The owner-recorded engine cycles are MIT-licensed; see [audio credits](ENGINE_AUDIO_CREDITS.md).
