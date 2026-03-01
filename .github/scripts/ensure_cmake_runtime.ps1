param(
    [string]$BasePath = "work/glare-core/scripts"
)

Write-Host "Patching CMake wrappers and build scripts in: $BasePath"

$targets = @(
    "cmake.rb",
    "build_llvm.rb",
    "build_libressl.rb",
    "build_libjpegturbo.rb"
)

$snippet = @"
# Ensure MSVC runtime set to MultiThreadedDLL to avoid debug CRT linkage
unless cmake_args.include?("CMAKE_MSVC_RUNTIME_LIBRARY")
  cmake_args += " -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL"
end
unless cmake_args.include?("CMAKE_BUILD_TYPE")
  cmake_args += " -DCMAKE_BUILD_TYPE=Release"
end
"@

foreach ($f in $targets) {
    $path = Join-Path $BasePath $f
    if (!(Test-Path $path)) { Write-Host "$path not found; skipping"; continue }

    Write-Host "Checking: $path"
    $txt = Get-Content $path -Raw

    if ($txt -match 'CMAKE_MSVC_RUNTIME_LIBRARY' -and $txt -match 'CMAKE_BUILD_TYPE') {
        Write-Host "Already patched: $path"
        continue
    }

    $patched = $false

    # Prefer inserting before a Dir.chdir(@build_dir) block if present
    $pattern1 = "(?m)(^\s*Dir\.chdir\(@build_dir\) do)"
    if ([regex]::IsMatch($txt, $pattern1)) {
        $replacement = "$snippet`n$1"
        $newTxt = [regex]::Replace($txt, $pattern1, $replacement)
        if ($newTxt -ne $txt) { Set-Content -Path $path -Value $newTxt; Write-Host "Patched $path (via Dir.chdir)"; $patched = $true }
    }

    if (-not $patched) {
        # Fallback: insert before first occurrence of 'cmake_args' variable usage/assignment
        $pattern2 = "(?m)(^.*cmake_args.*$)"
        if ([regex]::IsMatch($txt, $pattern2)) {
            $newTxt = [regex]::Replace($txt, $pattern2, "$snippet`n$1", 1)
            if ($newTxt -ne $txt) { Set-Content -Path $path -Value $newTxt; Write-Host "Patched $path (via cmake_args)"; $patched = $true }
        }
    }

    if (-not $patched) {
        Write-Host "Pattern not found; no changes made to $path"
    }
}
