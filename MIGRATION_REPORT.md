# GotlandRing migration report

## Status

Unreal Engine 5.8.4 is installed on EXCAVATOR at D:/UE_5.8. The full circuit was imported, saved and rendered with the RTX 3090 on 2026-10-30. Content/Migration contains the full road, aprons, and saved scene; Content/Migration/Scenery contains 18 scenery mesh/material groups and the reused textures; Renders/GotlandRing.png is the actual 2560×1440 Unreal capture. Imported bounds are validated. The original Unity checkout remains clean.

The scene includes the 7,214.397 m closed road, 3 m aprons, and the existing Unity surrounding ground and scenery. The ground is Unity’s authored approximation, not surveyed DTM. No new geographic data was acquired or invented.

## Inventory and architecture

Inspected CODEX_LOG.md, DATA_LICENSES.md, THIRD_PARTY_NOTICES.md, TRACK.md, track/SURFACE_README.md, TrackData.cs, QuarryTerrain.cs and RingDrive scene construction. Unity generates a single circuit at runtime from Assets/Resources/Track/Surface.csv. TrackData builds 16 road subdivisions, 3 m aprons and approximate shoulders extending 42 m from the reference line. The larger ground grid derives nearest-road elevations minus 3 m; quarry stockpiles use visual estimates and noise. These are not actual terrain tiles. No GIS raster, RAW heightmap or GeoJSON was found in the source-file inventory; no terrain streaming system is used by this scene construction.

Reusable assets are the surface and centerline CSVs, their coordinate metadata, and Asphalt/Gravel textures. Existing C# surface mathematics is reused directly in the Python bridge. Existing Python scripts concern audio and car preparation, not a reusable GIS acquisition pipeline. The standalone track/Unity6 importer provides another record of surface construction. Unity materials, meshes created at runtime, lighting, controllers and scenery require engine replacement. The Subaru visual prefab and a native Unreal driving pawn are included; turbine rotation remains static.

The selected area is the complete circuit: all 2,406 rows, including the duplicated closing cross-section, chainage 0–7,214.397 m. It keeps all supplied samples and sixteen transverse road subdivisions. Road-adjacent shoulders and the surrounding meadow and quarry meshes are exported directly from Unity’s existing construction code.

## Files and reconstruction

Scripts/export_scene.py reads the sibling Unity CSV without modifying it and writes Export/{Road,LeftApron,RightApron}.obj, scene.json and validation.json. JSON retains double-precision geometry and source provenance; OBJ positions use nine decimal places in centimetres. No heightmap quantization or spatial resampling occurs. The exporter packages only the existing Asphalt.jpg and Gravel.jpg in Export/Textures (about 7.5 MB total); Scripts/import_scene.py reads those files so the Unreal import can run independently on EXCAVATOR. Generated Unreal assets belong under Content/Migration.

The road cross-section is y + slope*d + curve*d², with normalized horizontal right vector (forward_z, -forward_x). Ground aprons share road-edge vertices and interpolate the existing apron height difference over 3 m. This reproduces TrackData's geometry in Python double precision, not Unity float rounding. UVs preserve offset/4 and chainage/4. No banking rotation is added.

## Unity scenery export

`Scripts/export_scenery.py` copies Assets, Packages and ProjectSettings into the ignored `Export/UnitySceneryStage` directory. It runs the existing Unity 6000.3.25f1 installation with `Scripts/Unity/ExportScenery.cs`. Only the copy is changed: scene construction stops before batching, cars and physics; tree mesh CPU data remains readable. The original Unity project is read-only throughout.

Unity constructs the actual runtime meadow grid, 42 m shoulders, three quarry piles, road paint, kerbs, safety barriers, main building and forecourt, turbine geometry and ground transitions, rocks, signs and woodland. The exporter groups transformed world-space meshes by material and retains vertex positions, normals and UVs. Road and apron submeshes are excluded because the validated road bridge already supplies them. OBJ and `scenery.json` are written under `Export/Scenery`, with existing Asphalt, Gravel, Grass and Pine textures. The imported scenery contains 299,942 triangles across 18 groups, 10,595 mapped trees, and 85 text labels. Combined with the road and aprons, the scene has 386,522 mesh triangles, excluding font-generated text geometry.

`Scripts/import_scenery.py` imports those meshes at identity transforms, recreates base color, roughness and metallic materials, and validates each group's bounds within 0.1 cm. The first capture and the detail capture confirm the saved scenery is rendered in Unreal. `Validation/scenery_validation.json` records the actual per-group errors, counts and labels. `Renders/GotlandRingDetail.png` shows the main building and pit area.

Material and animation limitations: Unity's shader-driven tree billboards become two crossed, alpha-masked cards at each original tree position and dimensions; per-tree shade variation is not transferred. Quarry triplanar shading is approximated using its existing mesh UVs and texture. Normal maps and Unity fog are not transferred in this pass. Sign text is recreated with Unreal's default font, rather than the Unity font atlas. Turbine rotors retain their original initial phase but are static. These are rendering differences, not changes to the source scenery placement. Road and solid scenery use complex triangle collision; tree cards have collision disabled.

## Coordinates

Unity local metres (x east, y up, z north) become Unreal centimetres (X north, Y east, Z up) using (100z,100x,100y). The inverse is (Y/100,Z/100,X/100). This cyclic axis permutation preserves handedness in the canonical JSON geometry. OBJ interchange uses (X,-Y,Z) and reversed triangle order to account for Unreal 5.8’s unconditional OBJ Y reflection; imported actor transforms remain identity. Left apron ordering is reversed because its columns progress outward to the left.

The source local system is WGS84 azimuthal equidistant at 57.835756925442 N, 18.830856092125 E. Height origin is 38.507350922 m RH2000. SWEREF99 TM reference origin is E=727401.318349, N=6416863.884251 m (EPSG:3006); vertical reference EPSG:5613. SWEREF offsets are not interchangeable with the local AEQD frame. For global reprojection use the documented AEQD inverse followed by EPSG:3006; no reprojection is needed for this migration. Large projected positions never enter the engine geometry.

## Validation actually performed

Export executed successfully with 2,406 cross-sections and 86,580 triangles. Every triangle has positive projected area. Shared road/apron boundary positions match exactly: zero seam gap. The first and last cross-sections also match exactly across all three meshes: zero circuit closure gap. Reconstructed road edges differ from the independently stored rounded CSV edge columns by at most 0.001145244 m. This is a numerical comparison, not survey accuracy. The source CSV SHA256 is 778bba6066860608dbc3414143773115c783a22824da9617d8ff5062bbefdf4d and remains unchanged across export.

Bounds in local metres: north −420.412004 to 699.295567; east −790.632942 to 671.390872; height relative to origin −19.696659 to 4.068903. The road validation is independent of the approximate scenery-ground extents. Unreal bounds match the canonical export within 0.003699236 cm (about 0.037 mm), including elevation bounds. Top faces and both textured aprons are visible in the actual capture. UV0 uses the exported coordinates; no independent imagery orientation reference exists. The standalone driving check produces separate collision and vehicle reports. Source elevations and widths are processed estimates; this is not a surveyed circuit.

## Remote validation

EXCAVATOR imported all three full-circuit meshes and reused the existing Asphalt and Gravel textures. Validation/import_validation.json records the circuit chainage, cross-section and triangle counts, zero export closure gap, and per-mesh imported bounds. The source CSV and Unity project remain unchanged.

## Unreal procedure and verification

The official Epic Launcher installed UE 5.8.4 after the user accepted its licensing declaration and EULA. The project is at D:/Dev/GotlandRing/GotlandRingUnreal. Existing UE4 was left untouched. Engine Python is 3.11.8.

Open GotlandRingUnreal.uproject with UE 5.8. The saved startup level is /Game/Migration/Prototype. To regenerate source exports on macOS, run `python3 Scripts/export_scene.py` then `python3 Scripts/export_scenery.py`. The second requires the existing Unity installation under the sibling Unity project’s Tools directory. Copy `Export/Scenery` and the road exports to EXCAVATOR. To rebuild and capture there:

```powershell
& 'D:/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' 'D:/Dev/GotlandRing/GotlandRingUnreal/GotlandRingUnreal.uproject' '-ExecutePythonScript=D:/Dev/GotlandRing/GotlandRingUnreal/Scripts/render_scene.py' -unattended -NoSplash
```

The importer loads or creates its generated level, clears its actors, imports the three road/apron OBJ meshes plus the 18 scenery groups and packaged textures, constructs materials, and adds sunlight, skylight, atmosphere and a camera. It asserts each mesh's bounds and that the level saved successfully. It replaces generated assets; keep custom work elsewhere. Camera and light Rotator fields use explicit keywords. The overview camera frames the complete circuit from its exported bounds, looking down at 75 degrees. It uses fixed manual exposure, with a 10-lux directional light and 0.5 skylight intensity. The screenshot is Saved/GotlandRing.png. The render runner keeps the Editor alive for its asynchronous screenshot; reopen normally afterward for interactive editing.

Validation/import_validation.json records the engine version and all mesh bounds. The full-circuit import completed without Python or level-save errors. The screenshot confirms top-facing road and apron surfaces and visible Asphalt/Gravel textures. Numerical bounds checks caught and corrected the OBJ axis reflection before accepting this result. Generated assets and evidence were copied back to the macOS project.

No full DTM, terrain imagery tile, rectangular terrain boundary, or surveyed accuracy can be validated from the available source. The surrounding ground reproduces the authored Unity approximation. A surveyed replacement requires a real terrain dataset and provenance. Further work can address tree billboarding, shader parity, turbine animation, and detailed vehicle dynamics.

## Attribution

© OpenStreetMap contributors — ODbL 1.0, https://www.openstreetmap.org/copyright . Datakälla: Lantmäteriets Min karta, © Lantmäteriet, CC BY 4.0; information has been processed, https://creativecommons.org/licenses/by/4.0/ . Preserve derivative-database obligations. Asphalt (Aerial Asphalt 01), Gravel (Gravel Floor), and Grass (Withered Grass) are Poly Haven CC0 assets according to the source THIRD_PARTY_NOTICES.md and docs/ASSET-CREDITS.md. DATA_LICENSES.md is preserved verbatim from the Unity repository; its relative source paths refer to that repository. Source-derived code retains the supplied MIT LICENSE.

The Pine cutout is AI-generated in the source project. Forest distribution derives from the supplied Lantmäteriet aerial crops: © Lantmäteriet, CC BY 4.0, processed information. The existing C# scenery construction and shaders retain the supplied MIT license.

For the close-up, run the same Editor command with `Scripts/render_detail.py`; it loads the saved level without rebuilding it and writes `Saved/GotlandRingDetail.png`.

## Vehicle implementation

`Scripts/export_vehicle.py` and `Scripts/Unity/ExportVehicle.cs` read the existing SubaruImpreza prefab from the isolated Unity copy. They bake body/interior meshes into local car coordinates and export spinning wheel groups around their original pivots. The source prefab supplies a 33.8657 cm visual wheel radius and wheel centers; no new vehicle model or texture is generated. Skinned driver geometry is baked into its initial pose. `Scripts/import_vehicle.py` imports the 22 mesh/material groups and existing textures, places the native `ARingCar` at source start/finish point 284, and assigns the default game mode.

The chassis uses the source 1,360 kg loaded mass and nominal COM at 48 cm height, shifted 20.16 cm forward for the 58% front weight distribution. Physical wheels use the source 2.52 m wheelbase, 1.46 m track and 30.57 cm radius; the imported visual wheel hubs retain their original positions. Suspension uses 20 cm travel, front/rear 34,000/29,000 N/m springs and 3,800/3,200 N·s/m dampers. Front/rear anti-roll rates are 4,500/3,200 N/m. Collision and suspension are supplied by Unreal raycasts and Chaos, not Unity WheelCollider.

`RingDynamics.h` ports SubaruEngine and SubaruAwdController’s engine, clutch, gear-selection and tire-budget formulas. The torque curve uses the source keyframes with their zero-tangent interpolation. Engine inertia is 0.28 kg·m²; the clutch targets 4,500 RPM with 360 Nm capacity; the limiter approaches 6,800 RPM. Five ratios, final drive, reverse, 0.22-second shift clutch opening, 0.8-second shift cooldown and downshift blips follow the source. Engine RPM is integrated independently of chassis speed. Brake input cuts drive; traction-control torque reductions also reduce clutch reaction on the engine.

The tire implementation integrates four 18 kg rotating wheels with an implicit motor/brake/road-reaction solve. Longitudinal peak/asymptotic slip is 0.22/0.65 with 1.0/0.75 coefficients. Lateral peak/asymptotic slip is 0.20/0.60; front/coasting rear retain 0.98, while driven rear tires reduce toward 0.50 under longitudinal demand. Front/rear lateral stiffness is 1.05/1.20, and combined demand caps at 0.9 of the longitudinal budget. Source TC and ABS reduce torque above 0.22 forward slip. Lifting doubles grip and moves tire-force application closer to the COM. Off-road grip is multiplied by 0.48, classified against the actual road mesh footprint rather than scenery textures. Kerbs/paint do not hide the underlying road classification. Wheel angular speed drives the visible wheel spin and gearbox shaft RPM.

The exposed curves and controller formulas are preserved, but the contact solver’s normalized slip, low-speed force limiting and integration are explicit Unreal implementations. Unity’s internal WheelCollider friction/suspension solver is not reproduced bit-for-bit. Engine/wheel state takes steps no larger than 2 ms; averaged forces act on the Chaos chassis. This is a prototype handling model, not measured real-car fidelity or complete Unity handling equivalence.

`RingEngineMixer.h` ports ImprezaEngineMixer sample processing. `RingAudio` uses UE’s ISoundGenerator on the audio thread, shared atomic controls, and compiled PCM cycles generated from the original WAV files. No sound asset streaming or callback allocation is required during playback. Output follows the actual independent engine RPM, including clutch loading, rev limiting and shifts. Mute uses the source fade. The separate Unity surface-sound mixer is not included.

## Drivetrain and audio validation

The isolated Unity project executed the original SubaruEngine and ImprezaEngineMixer to produce reference data. The same C++ port passed on both Clang/macOS and MSVC/EXCAVATOR: 4,000 engine steps differed by at most 0.000976563 RPM and 0.000274659 Nm. The 12-second stereo audio sequence differed by at most 2.98023e-7 per sample, with 4.08259e-7 relative RMS error; the muted tail is zero. Audio checks also cover finite, unclipped output at 44.1, 48 and 96 kHz. Source hashes are recorded in Validation/dynamics_sources.json and Validation/engine_audio_sources.json. Reference inputs are under Validation/UnityReference.

Portable tests exercise acceleration through fourth gear, braking, reverse drive at all four wheels, torque distribution, multi-gear downshift with open clutch, TC/ABS reduction, combined-grip budgets and lift/off-road multipliers. `Validation/portable_validation.txt` records the EXCAVATOR run. This does not substitute for a full driving lap or compare the proprietary Unity wheel solver.

Rendered Unreal checks use the real pawn and force path. The circuit check covers all 7,218 center/edge collision probes, short acceleration/braking, steering and reset. The longer test creates a temporary 1 km flat collision pad and measures gear changes, higher-speed braking, powered steering reversals beyond the lateral peak, lift recovery and reverse. The pad is never saved into the circuit. Reports and per-frame telemetry are under Validation; the latest actual game image and audio-generator capture are under Renders. The audio capture verifies the generator signal, not a calibrated loudspeaker measurement.

Vehicle artwork remains CC BY-NC 4.0; see VEHICLE_ASSET_CREDITS.md. The owner-recorded engine cycles and source-derived driving/audio code retain MIT licensing; see ENGINE_AUDIO_CREDITS.md.
