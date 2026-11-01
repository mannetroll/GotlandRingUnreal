param(
    [string]$SingleExe='C:\Dev\GotlandRing\Releases\v0.1.0-single-file\GotlandRingUnreal-Portable.exe',
    [string]$PackageDirectory='C:\Dev\GotlandRing\Releases\v0.1.0-portable\Windows'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$work=Join-Path $root 'Saved\Single File Validation'
$standalone=Join-Path $work 'Standalone'
New-Item -ItemType Directory -Force $standalone | Out-Null
$exe=Join-Path $standalone 'GotlandRingUnreal-Portable.exe'
Copy-Item $SingleExe $exe -Force
if(@(Get-ChildItem $standalone -File).Count -ne 1){throw 'The standalone test folder must contain only the EXE.'}
$info=Get-Content (Join-Path (Split-Path $SingleExe -Parent) 'build-info.json') -Raw | ConvertFrom-Json
$version=$info.payload_sha256.Substring(0,16).ToUpperInvariant()
$cache=Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) "Mannetroll\GotlandRingUnreal\$version"
$ready=Join-Path $cache 'ready.txt'
if(Test-Path $ready){throw 'This test requires the new package to have a cold cache.'}
$oldDotnetRoot=$env:DOTNET_ROOT;$oldDotnetX64=$env:DOTNET_ROOT_X64
try {
    $env:DOTNET_ROOT=Join-Path $work 'No external .NET runtime'
    $env:DOTNET_ROOT_X64=$env:DOTNET_ROOT
    $watch=[Diagnostics.Stopwatch]::StartNew()
    $first=Start-Process $exe -ArgumentList '--extract-only' -PassThru
    [void]$first.Handle
    $second=Start-Process $exe -ArgumentList '--extract-only' -PassThru
    [void]$second.Handle
    foreach($p in @($first,$second)){
        if(-not $p.WaitForExit(180000)){$p.Kill();throw 'Concurrent extraction timed out.'}
        if($p.ExitCode -ne 0){throw "Extraction failed: $($p.ExitCode)"}
    }
    $watch.Stop();$coldSeconds=$watch.Elapsed.TotalSeconds
    $checked=0
    Get-ChildItem $PackageDirectory -Recurse -File | ForEach-Object {
        $relative=$_.FullName.Substring($PackageDirectory.Length).TrimStart('\')
        $extracted=Join-Path $cache $relative
        if(-not(Test-Path $extracted)){throw "Extracted file is missing: $relative"}
        if((Get-FileHash $_.FullName -Algorithm SHA256).Hash -ne (Get-FileHash $extracted -Algorithm SHA256).Hash){throw "Extracted file differs: $relative"}
        $checked++
    }
    $stamp=(Get-Item $ready).LastWriteTimeUtc
    $watch.Restart();$warm=Start-Process $exe -ArgumentList '--extract-only' -PassThru
    [void]$warm.Handle
    if(-not $warm.WaitForExit(60000)){$warm.Kill();throw 'Cached launch timed out.'}
    $watch.Stop();$warmSeconds=$watch.Elapsed.TotalSeconds
    if($warm.ExitCode -ne 0 -or (Get-Item $ready).LastWriteTimeUtc -ne $stamp){throw 'The warm launch did not reuse the cache.'}
    $modes=@{}
    foreach($mode in @('Migration','Instruments','Driving')){
        $report=Join-Path "$work\Saved" ($mode.ToLower()+'_validation.json')
        if(Test-Path $report){Remove-Item $report}
        $p=Start-Process $exe -ArgumentList @("-Ring${mode}Test",'-windowed','-ResX=1600','-ResY=900',"-UserDir=`"$work`"") -PassThru
        [void]$p.Handle
        if(-not $p.WaitForExit(60000)){$p.Kill();throw 'Portable launcher timed out.'}
        if($p.ExitCode -ne 0){throw "Portable launch failed: $($p.ExitCode)"}
        $watch.Restart()
        do {
            Start-Sleep -Milliseconds 500
            $games=@(Get-CimInstance Win32_Process -Filter "Name='GotlandRingUnreal-Win64-Shipping.exe'" | Where-Object {$_.ExecutablePath -like "$cache\*" -and $_.CommandLine -like "*-Ring${mode}Test*"})
            $complete=(Test-Path $report) -and $games.Count -eq 0
        } while(-not $complete -and $watch.Elapsed.TotalSeconds -lt 150)
        if(-not $complete){throw "$mode did not complete through the single-file launcher."}
        $data=Get-Content $report -Raw | ConvertFrom-Json
        if(-not $data.passed){throw "$mode failed through the single-file launcher."}
        $modes[$mode]=$true
    }
    $result=@{
        passed=$true;single_exe_only=$true;self_contained_runtime=$true
        concurrent_cold_extraction=$true;extracted_files_checked=$checked
        cold_seconds=$coldSeconds;cached_launch_seconds=$warmSeconds;cache_reused=$true
        forwarded_arguments_with_spaces=$true;gameplay=$modes
        sha256=(Get-FileHash $SingleExe -Algorithm SHA256).Hash.ToLower()
    }
    $result | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $root 'Validation\single_file_fractal.json') -Encoding UTF8
    $result | ConvertTo-Json -Depth 4
} finally {
    $env:DOTNET_ROOT=$oldDotnetRoot;$env:DOTNET_ROOT_X64=$oldDotnetX64
}
