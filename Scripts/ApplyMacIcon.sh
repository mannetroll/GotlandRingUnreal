#!/bin/bash
set -euo pipefail
project_root=$(cd "$(dirname "$0")/.." && pwd)
app_dir=${1:?Pass the macOS app bundle path}
cp "$project_root/Resources/Mac/GotlandRing.icns" "$app_dir/Contents/Resources/AppIcon.icns"
python3 - "$app_dir/Contents/Info.plist" <<'PY'
import plistlib,sys
from pathlib import Path
path=Path(sys.argv[1]);info=plistlib.loads(path.read_bytes())
info['CFBundleIconFile']='AppIcon.icns'
info.pop('CFBundleIconName',None)
path.write_bytes(plistlib.dumps(info))
PY
codesign --force --deep --sign - --preserve-metadata=identifier,entitlements,flags "$app_dir"
codesign --verify --deep --strict "$app_dir"
