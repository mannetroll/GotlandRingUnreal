# Data licenses

This repository contains geospatial data with licensing separate from the project's MIT-licensed source code.

## Track centerline and road surface

The following files contain versions of the adapted centerline database:

- `track/gotland_ring_full_centerline_3m.csv`
- `track/gotland_ring_full_centerline_3m_lowpass.csv`
- `track/gotland_ring_full_surface_3m.csv`
- `Assets/Resources/Track/Surface.csv`

The bundled Unity resource is an unchanged copy of the surface CSV, which retains all columns of the low-pass centerline and adds estimated road widths, crossfall, crown/hollow and apron heights. The surface database has the same attribution and licensing as the centerline; see [SURFACE_README.md](track/SURFACE_README.md) for its additional terrain and imagery sources and limitations.

The centerline was derived in part from OpenStreetMap data and is distributed under the Open Database License (ODbL) 1.0.

Attribution:

> © OpenStreetMap contributors — ODbL 1.0

OpenStreetMap copyright and licence information:
https://www.openstreetmap.org/copyright

Redistributors should preserve attribution and comply with the applicable ODbL requirements, including share-alike requirements for derivative databases where they apply.

## Lantmäteriet source material

Lantmäteriet Min karta orthophoto and elevation information were used as source material during reconstruction and processing.

Attribution:

> Datakälla: Lantmäteriets Min karta, © Lantmäteriet, CC BY 4.0. Information has been processed.

Creative Commons Attribution 4.0:
https://creativecommons.org/licenses/by/4.0/

The CSV files are processed/derived information rather than unmodified source imagery. See [TRACK.md](TRACK.md) for provenance, processing details, coordinate systems, accuracy limitations, and source references.

## Wind turbines

`windmills/gotland_ring_wind_turbines.csv` and its unchanged bundled copy, `Assets/Resources/Track/WindTurbines.csv`, contain supplied positions and turbine specifications from Länsstyrelserna / Vindbrukskollen, with terrain elevations from © Lantmäteriet. The accompanying map includes Lantmäteriet orthophotography. These source materials are separate from the project's MIT-licensed code. Preserve the supplied attribution and [source/import notes](windmills/WIND_TURBINES_README.txt) with redistributed data; those notes record the service URLs, 8 October 2026 retrieval, coordinate reference systems and accuracy limits.

## No survey claim

The main building footprint and orientation are visually derived from the Lantmäteriet orthophotography in the supplied turbine map: © Lantmäteriet, processed information. The modeled façade, dimensions and ground transitions are approximate; see [scenery references](docs/SCENERY.md).

The centerline and elevations are an approximate reconstruction for a game prototype. They are not an authoritative survey of the circuit and must not be represented as surveyed track geometry.
