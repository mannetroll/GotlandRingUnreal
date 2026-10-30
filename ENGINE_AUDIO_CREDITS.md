# Impreza engine recording

The three engine cycles (`ImprezaLow.wav`, `ImprezaMid.wav`, `ImprezaHigh.wav`) originate from the repository owner’s own film, `sound/Gotland Ring 2022 - 1 of 1.mp4`, recorded in their Impreza at GotlandRing in 2022 with Alec Arho-Havrén. The full film is not included. The Unity repository’s THIRD_PARTY_NOTICES.md licenses these derived audio assets under the project’s MIT LICENSE.

Source excerpts are 4:47.250–4:49.450 (estimated 2,530 RPM), 2:30.500–2:32.300 (3,060 RPM) and 3:54.375–3:55.950 (4,500 RPM). The supplied assets contain isolated, phase-aligned engine cycles, not the full cabin excerpts. Their source hashes are preserved in Validation/engine_audio_sources.json. Scripts/export_engine_audio.py converts the original mono 44.1 kHz PCM samples to EngineCycles.h without resampling or altering sample values.

RingEngineMixer ports the existing Unity ImprezaEngineMixer under the supplied MIT license. Captured Unreal audio in Renders/GotlandRingEngine.wav derives from these same cycles and mixer. Idle and RPM outside the recording ranges use pitched cycles, as in Unity.
