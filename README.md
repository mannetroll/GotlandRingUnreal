# GotlandRingUnreal

Full-circuit UE 5.8.4 migration, imported and rendered on EXCAVATOR: 7,214.397 m of the existing GotlandRing road with 3 m gravel aprons. No real surrounding terrain raster exists in the source project.

Open `GotlandRingUnreal.uproject` with UE 5.8; `/Game/Migration/Prototype` is the startup level. Saved Unreal assets are included under `Content/Migration`.

To rebuild:

1. Keep `GotlandRingUnity` beside this directory and run `python3 Scripts/export_scene.py` here.
2. In Unreal's Python console run `exec(open(unreal.Paths.project_dir() + 'Scripts/import_scene.py').read())`.
3. For an automated 2560×1440 capture, launch the Editor with `-ExecutePythonScript=<absolute-project-path>/Scripts/render_scene.py`. Output: `Saved/GotlandRing.png`.

The importer replaces its generated level and assets. EXCAVATOR engine: `D:/UE_5.8`; project: `D:/Dev/GotlandRing/GotlandRingUnreal`.

See [MIGRATION_REPORT.md](MIGRATION_REPORT.md) for coordinates, commands, attribution and limitations, [validation](Validation/import_validation.json) for measured bounds, and [render](Renders/GotlandRing.png) for the actual Unreal image.
