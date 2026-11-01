param(
    [string]$PackageDirectory='C:\Dev\GotlandRing\Releases\reference-portable\Windows',
    [string]$OutputDirectory='C:\Dev\GotlandRing\Releases\unreal.8-single-file'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$PackageDirectory=(Resolve-Path $PackageDirectory).Path
if(-not(Test-Path "$PackageDirectory\GotlandRingUnreal.exe")){throw 'Build the portable Unreal game folder first.'}
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$work=Join-Path $root 'Saved\SingleFile'
New-Item -ItemType Directory -Force $work,$OutputDirectory | Out-Null
$payload=Join-Path $work 'Game.zip'
$stream=[IO.File]::Open($payload,[IO.FileMode]::Create)
$archive=[IO.Compression.ZipArchive]::new($stream,[IO.Compression.ZipArchiveMode]::Create)
try {
    Get-ChildItem -LiteralPath $PackageDirectory -Recurse -File | Sort-Object FullName | ForEach-Object {
        $entry=$_.FullName.Substring($PackageDirectory.Length).TrimStart('\').Replace('\','/')
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$_.FullName,$entry,[IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally { $archive.Dispose();$stream.Dispose() }
& dotnet publish "$root\PortableLauncher\PortableLauncher.csproj" -c Release -r win-x64 --self-contained true "-p:GamePayload=$payload" -o $OutputDirectory
if($LASTEXITCODE -ne 0){throw "Single-file publish failed: $LASTEXITCODE"}
$exe=Join-Path $OutputDirectory 'GotlandRingUnreal-Portable.exe'
$hash=(Get-FileHash $exe -Algorithm SHA256).Hash.ToLower()
"$hash  GotlandRingUnreal-Portable.exe" | Set-Content (Join-Path $OutputDirectory 'SHA256SUMS.txt') -Encoding ASCII
$info=@{
    file='GotlandRingUnreal-Portable.exe'
    bytes=(Get-Item $exe).Length
    sha256=$hash
    payload_sha256=(Get-FileHash $payload -Algorithm SHA256).Hash.ToLower()
    configuration='Shipping'
    platform='Win64'
    tag='unreal.8'
}
$info | ConvertTo-Json | Set-Content (Join-Path $OutputDirectory 'build-info.json') -Encoding UTF8
Write-Output "Single-file portable game: $exe"
Write-Output "Bytes: $($info.bytes)"
