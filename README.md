# GotlandRingUnreal

Full-circuit UE 5.8.4 migration, imported and rendered on EXCAVATOR: 7,214.397 m of the existing GotlandRing road with 3 m gravel aprons. Includes Unity’s existing meadow, quarry piles, woodland, building, barriers, turbines, markings and signs. Surrounding ground is an authored approximation, not surveyed terrain.

Build the C++ editor target, then open `GotlandRingUnreal.uproject` with UE 5.8; `/Game/Migration/Prototype` is the startup level. Saved Unreal assets are included under `Content/Migration`.

To rebuild:

1. Keep `GotlandRingUnity` beside this directory and run `python3 Scripts/export_scene.py` followed by `python3 Scripts/export_scenery.py` and `python3 Scripts/export_vehicle.py` here. The scenery exporter uses the existing Unity editor in an isolated copy.
2. In Unreal's Python console run `exec(open(unreal.Paths.project_dir() + 'Scripts/import_scene.py').read())`.
3. For an automated 2560×1440 capture, launch the Editor with `-ExecutePythonScript=<absolute-project-path>/Scripts/render_scene.py`. Output: `Saved/GotlandRing.png`.

The importer replaces its generated level and assets. EXCAVATOR engine: `D:/UE_5.8`; project: `D:/Dev/GotlandRing/GotlandRingUnreal`.

See [MIGRATION_REPORT.md](MIGRATION_REPORT.md) for coordinates, commands, attribution and limitations, [validation](Validation/import_validation.json) for measured bounds, and [render](Renders/GotlandRing.png) for the actual Unreal image.

For a close-up of the saved scenery, use `Scripts/render_detail.py` as the ExecutePythonScript target. See [detail render](Renders/GotlandRingDetail.png) and [scenery validation](Validation/scenery_validation.json).

## Driving

Press Play in `/Game/Migration/Prototype`, then click the viewport, or run `Scripts/Drive.ps1` on EXCAVATOR. W/Up accelerates, S/Down brakes, X brakes forward travel then reverses, A/D steers, Space applies the rear handbrake, R resets to start/finish, and M mutes engine audio. Forward throttle takes priority over X. The HUD shows speed, engine RPM, gear, tire contacts and body sideslip.

The native vehicle ports Unity’s independent flywheel, torque curve, automatic clutch, five forward ratios, reverse, shift delay/cooldown and throttle blips. It also ports the 50:50 AWD torque budget, traction control, ABS, combined-grip reduction, powered rear slip curve, doubled lift-off grip, off-road multiplier, anti-roll, drag and rolling resistance. Four rotating raycast tires apply these forces to Chaos. Tire contact/rotation is an Unreal solver implementation, not Unity’s proprietary WheelCollider solver; matching tuning does not establish identical handling or real-car fidelity. No detailed differential, tire temperature or wear model exists in the Unity source.

Engine audio uses the three existing owner-recorded Impreza cycles with the ported phase-locked mixer, RPM pitch, load crossfade/filtering, lift-off air, DC filtering and mute fades. The audio renderer owns its playback state; the game thread supplies engine RPM, load and speed. This covers engine audio; Unity’s separate surface-sound mixer, cockpit controls, autopilot and menu settings remain outside this migration.

Build on EXCAVATOR:

```powershell
& D:/UE_5.8/Engine/Build/BatchFiles/Build.bat GotlandRingUnrealEditor Win64 Development -Project=D:/Dev/GotlandRing/GotlandRingUnreal/GotlandRingUnreal.uproject -WaitMutex
```

Validation:

- `Scripts/ValidateDriving.ps1`: rendered circuit collision, acceleration, braking, steering, reset and live audio check.
- `Scripts/ValidateDynamics.ps1`: temporary flat-pad acceleration/gear changes, braking torque cut, powered steering reversals, lift recovery and reverse. The pad exists only during the test and is not saved into the circuit.
- `Scripts/TestPortable.cmd`: MSVC tests against the original Unity engine/audio outputs, tire budgets, TC/ABS, downshift and reverse. On macOS use `clang++ -std=c++17 -O2 -ffp-contract=off -I Source/GotlandRingUnreal Tests/dynamics.cpp -o /tmp/ring-dynamics-test`, then `/tmp/ring-dynamics-test Validation/UnityReference`.
- `Scripts/export_dynamics_reference.py`: regenerate reference outputs with the original Unity classes in the isolated staging project. `Scripts/export_engine_audio.py` regenerates compiled PCM cycles from the three original WAV files.

[Driving render](Renders/GotlandRingDriving.png) · [Engine audio capture](Renders/GotlandRingEngine.wav) · [Circuit check](Validation/driving_validation.json) · [Dynamics check](Validation/dynamics_validation.json) · [Source parity](Validation/portable_validation.txt)

The Subaru and reused cockpit/driver assets are **CC BY-NC 4.0**, separate from the code’s MIT license. Preserve [vehicle credits](VEHICLE_ASSET_CREDITS.md). The owner-recorded engine cycles are MIT-licensed; see [audio credits](ENGINE_AUDIO_CREDITS.md).
