#!/bin/bash
set -euo pipefail
project_root=$(cd "$(dirname "$0")/.." && pwd)
engine_root=${UE_MAC_ROOT:-"$HOME/Epic Games/UE_5.8"}
output_dir=${1:-"$project_root/../Releases/v0.1.0-macOS"}
mkdir -p "$output_dir"
output_dir=$(cd "$output_dir" && pwd)
if [[ ! -x "$engine_root/Engine/Build/BatchFiles/RunUAT.sh" ]]; then
  echo "Unreal Engine 5.8 for macOS is required. Set UE_MAC_ROOT to its installation directory." >&2
  exit 1
fi
"$engine_root/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  "-project=$project_root/GotlandRingUnreal.uproject" \
  -noP4 -platform=Mac -architecture=arm64 -clientconfig=Shipping \
  -build -cook -map=/Game/Migration/Prototype -stage -pak -iostore -package \
  -archive "-archivedirectory=$output_dir" -nodebuginfo -unattended -utf8output
app_dir="$output_dir/Mac/GotlandRingUnreal.app"
rm -rf "$app_dir"
mv "$output_dir/Mac/GotlandRingUnreal-Mac-Shipping.app" "$app_dir"
mkdir -p "$app_dir/Contents/Resources/Credits"
for credit in LICENSE DATA_LICENSES.md VEHICLE_ASSET_CREDITS.md ENGINE_AUDIO_CREDITS.md; do
  cp "$project_root/$credit" "$output_dir/"
  cp "$project_root/$credit" "$app_dir/Contents/Resources/Credits/"
done
bash "$project_root/Scripts/ApplyMacIcon.sh" "$app_dir"
printf 'macOS build archived to: %s\n' "$output_dir"
