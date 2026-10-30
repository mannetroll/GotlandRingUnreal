# Visual asset credits

Photographic environment and scanned material assets from Poly Haven, distributed under CC0:

- [Kloppenheim 05](https://polyhaven.com/a/kloppenheim_05) HDR environment, Greg Zaal, 2K HDR.
- [Aerial Asphalt 01](https://polyhaven.com/a/aerial_asphalt_01), 2K diffuse and OpenGL normal map.
- [Withered Grass](https://polyhaven.com/a/withered_grass), 2K diffuse and OpenGL normal map.
- [Gravel Floor](https://polyhaven.com/a/gravel_floor), 2K diffuse and OpenGL normal map.
- [Poly Haven license](https://polyhaven.com/license).

## Rally car — CC BY-NC 4.0

- **Title:** Rally Car
- **Author:** [SpatialNeglect (@jeandiz)](https://sketchfab.com/jeandiz)
- **Source:** [Rally Car on Sketchfab](https://sketchfab.com/3d-models/rally-car-e0dfd3b6d19947df85002fd8de0a3a02)
- **License:** [Creative Commons Attribution-NonCommercial 4.0 International](https://creativecommons.org/licenses/by-nc/4.0/)
- **Files:** `Assets/Models/RallyCar/` and the derived `Assets/Resources/RallyCar.prefab`.
- **Adaptations:** Unity Standard materials; metallic/smoothness texture packing; normal-map and glass configuration; uniform scaling and placement; camera anchors; animated wheel/steering pivots; live procedural LCD with speed, RPM and gear; skinned driver arms and hands with procedural steering and regrips; head/body mesh separation for cockpit visibility. The original white paint texture is retained, with optional red and blue material tints. The pace-note book, co-driver and co-driver’s worn harness are removed from the game prefab. The driver’s geometry is split at existing seams so only the head and helmet are hidden in cockpit view.

## Subaru Impreza — CC BY-NC 4.0

- **Title:** Subaru Impreza
- **Author:** [Mateusz Woliński (@jeandiz)](https://sketchfab.com/jeandiz), as credited in the supplied GLB.
- **Source:** [Subaru Impreza on Sketchfab](https://sketchfab.com/3d-models/subaru-impreza-7fb4298d5d8f4185b25bb2c43d7f3787)
- **License:** [Creative Commons Attribution-NonCommercial 4.0 International](https://creativecommons.org/licenses/by-nc/4.0/)
- **Files:** `Assets/Models/SubaruImpreza/` and the derived `Assets/Resources/SubaruImpreza.prefab`. Embedded attribution is retained in `Source/source.json`.
- **Adaptations:** Geometry converted from the user-supplied `Impreza/subaru_impreza.glb` with Assimp; texture sizes reduced to 2K; metallic/roughness channels packed for Unity Standard materials; glass and light materials configured; vehicle scaled and aligned to the road; wheel steering and rotation pivots added. The original blue-and-gold livery is retained. The cabin shell is supplemented with the cockpit, driver, steering wheel and camera anchors from the separately credited Rally Car above.

Credit the authors, link the sources and licenses, identify modifications, and retain the noncommercial restriction when redistributing these assets or builds containing them. These assets are not covered by the repository's MIT license.

The pine cutout and application icon were AI-generated for this project. The track and project shaders were authored for this project. Forest distribution in `Assets/Resources/Track/Forest.csv` is derived from the supplied Lantmäteriet aerial crops in `track/`: © Lantmäteriet, CC BY 4.0, processed information. See [track sources and woodland processing](../TRACK.md). The supplied video, kept locally at `unstaged/KOENIGSEGG.webm` outside Git, is used as a visual reference only; its footage is not included in the built game.
