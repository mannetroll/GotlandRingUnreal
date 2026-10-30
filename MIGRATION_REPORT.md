# GotlandRing migration report

## Status

Unreal Engine 5.8.4 is installed on EXCAVATOR at D:/UE_5.8. The full circuit was imported, saved and rendered with the RTX 3090 on 2026-10-30. Content/Migration contains the saved level, three meshes, two textures and three materials; Renders/GotlandRing.png is the actual 2560×1440 Unreal capture. Imported bounds are validated. The original Unity checkout remains clean.

The available source supports a 7,214.397 m closed road and adjacent 3 m aprons, not a complete terrain tile. The requested real surrounding landscape remains unavailable in the repository; none was invented or downloaded.

## Inventory and architecture

Inspected CODEX_LOG.md, DATA_LICENSES.md, THIRD_PARTY_NOTICES.md, TRACK.md, track/SURFACE_README.md, TrackData.cs, QuarryTerrain.cs and RingDrive scene construction. Unity generates a single circuit at runtime from Assets/Resources/Track/Surface.csv. TrackData builds 16 road subdivisions, 3 m aprons and approximate shoulders extending 42 m from the reference line. The larger ground grid derives nearest-road elevations minus 3 m; quarry stockpiles use visual estimates and noise. These are not actual terrain tiles. No GIS raster, RAW heightmap or GeoJSON was found in the source-file inventory; no terrain streaming system is used by this scene construction.

Reusable assets are the surface and centerline CSVs, their coordinate metadata, and Asphalt/Gravel textures. Existing C# surface mathematics is reused directly in the Python bridge. Existing Python scripts concern audio and car preparation, not a reusable GIS acquisition pipeline. The standalone track/Unity6 importer provides another record of surface construction. Unity materials, meshes created at runtime, lighting, controllers and scenery require engine replacement. Vehicles, physics, quarry approximations are outside this migration.

The selected area is the complete circuit: all 2,406 rows, including the duplicated closing cross-section, chainage 0–7,214.397 m. It keeps all supplied samples and sixteen transverse road subdivisions. Only the source-derived adjacent aprons are exported; there is no fabricated landscape filling missing data.

## Files and reconstruction

Scripts/export_scene.py reads the sibling Unity CSV without modifying it and writes Export/{Road,LeftApron,RightApron}.obj, scene.json and validation.json. JSON retains double-precision geometry and source provenance; OBJ positions use nine decimal places in centimetres. No heightmap quantization or spatial resampling occurs. The exporter packages only the existing Asphalt.jpg and Gravel.jpg in Export/Textures (about 7.5 MB total); Scripts/import_scene.py reads those files so the Unreal import can run independently on EXCAVATOR. Generated Unreal assets belong under Content/Migration.

The road cross-section is y + slope*d + curve*d², with normalized horizontal right vector (forward_z, -forward_x). Ground aprons share road-edge vertices and interpolate the existing apron height difference over 3 m. This reproduces TrackData's geometry in Python double precision, not Unity float rounding. UVs preserve offset/4 and chainage/4. No banking rotation is added.

## Coordinates

Unity local metres (x east, y up, z north) become Unreal centimetres (X north, Y east, Z up) using (100z,100x,100y). The inverse is (Y/100,Z/100,X/100). This cyclic axis permutation preserves handedness in the canonical JSON geometry. OBJ interchange uses (X,-Y,Z) and reversed triangle order to account for Unreal 5.8’s unconditional OBJ Y reflection; imported actor transforms remain identity. Left apron ordering is reversed because its columns progress outward to the left.

The source local system is WGS84 azimuthal equidistant at 57.835756925442 N, 18.830856092125 E. Height origin is 38.507350922 m RH2000. SWEREF99 TM reference origin is E=727401.318349, N=6416863.884251 m (EPSG:3006); vertical reference EPSG:5613. SWEREF offsets are not interchangeable with the local AEQD frame. For global reprojection use the documented AEQD inverse followed by EPSG:3006; no reprojection is needed for this migration. Large projected positions never enter the engine geometry.

## Validation actually performed

Export executed successfully with 2,406 cross-sections and 86,580 triangles. Every triangle has positive projected area. Shared road/apron boundary positions match exactly: zero seam gap. The first and last cross-sections also match exactly across all three meshes: zero circuit closure gap. Reconstructed road edges differ from the independently stored rounded CSV edge columns by at most 0.001145244 m. This is a numerical comparison, not survey accuracy. The source CSV SHA256 is 778bba6066860608dbc3414143773115c783a22824da9617d8ff5062bbefdf4d and remains unchanged across export.

Bounds in local metres: north −420.412004 to 699.295567; east −790.632942 to 671.390872; height relative to origin −19.696659 to 4.068903. There are no rectangular terrain tile boundaries to validate. Unreal bounds match the canonical export within 0.003699236 cm (about 0.037 mm), including elevation bounds. Top faces and both textured aprons are visible in the actual capture. UV0 uses the exported coordinates; no independent imagery orientation reference exists. Collision and driving behavior were not tested. Source elevations and widths are processed estimates; this is not a surveyed circuit.

## Remote validation

EXCAVATOR imported all three full-circuit meshes and reused the existing Asphalt and Gravel textures. Validation/import_validation.json records the circuit chainage, cross-section and triangle counts, zero export closure gap, and per-mesh imported bounds. The source CSV and Unity project remain unchanged.

## Unreal procedure and verification

The official Epic Launcher installed UE 5.8.4 after the user accepted its licensing declaration and EULA. The project is at D:/Dev/GotlandRing/GotlandRingUnreal. Existing UE4 was left untouched. Engine Python is 3.11.8.

Open GotlandRingUnreal.uproject with UE 5.8. The saved startup level is /Game/Migration/Prototype. To rebuild and capture on EXCAVATOR:

```powershell
& 'D:/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' 'D:/Dev/GotlandRing/GotlandRingUnreal/GotlandRingUnreal.uproject' '-ExecutePythonScript=D:/Dev/GotlandRing/GotlandRingUnreal/Scripts/render_scene.py' -unattended -NoSplash
```

The importer loads or creates its generated level, clears its actors, imports the three OBJ meshes and packaged textures, constructs materials, and adds sunlight, skylight, atmosphere and a camera. It asserts each mesh's bounds and that the level saved successfully. It replaces generated assets; keep custom work elsewhere. Camera and light Rotator fields use explicit keywords. The overview camera frames the complete circuit from its exported bounds, looking down at 75 degrees. It uses fixed manual exposure, with a 10-lux directional light and 0.5 skylight intensity. The screenshot is Saved/GotlandRing.png. The render runner keeps the Editor alive for its asynchronous screenshot; reopen normally afterward for interactive editing.

Validation/import_validation.json records the engine version and all mesh bounds. The full-circuit import completed without Python or level-save errors. The screenshot confirms top-facing road and apron surfaces and visible Asphalt/Gravel textures. Numerical bounds checks caught and corrected the OBJ axis reflection before accepting this result. Generated assets and evidence were copied back to the macOS project.

No full DTM, terrain imagery tile, rectangular terrain boundary, or surveyed accuracy can be validated from the available source. The view intentionally has no surrounding ground. Next iteration should locate an existing real terrain dataset and its provenance before extending the ground area; collision and a close-range material inspection can follow separately.

## Attribution

© OpenStreetMap contributors — ODbL 1.0, https://www.openstreetmap.org/copyright . Datakälla: Lantmäteriets Min karta, © Lantmäteriet, CC BY 4.0; information has been processed, https://creativecommons.org/licenses/by/4.0/ . Preserve derivative-database obligations. Asphalt (Aerial Asphalt 01) and Gravel (Gravel Floor) are Poly Haven CC0 assets according to the source THIRD_PARTY_NOTICES.md and docs/ASSET-CREDITS.md. DATA_LICENSES.md is preserved verbatim from the Unity repository; its relative source paths refer to that repository. Source-derived code retains the supplied MIT LICENSE.
