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
# Force MSVC runtime and build type into cmake_args deterministically
# Remove any existing occurrences, then append explicit desired values.
cmake_args = cmake_args.gsub(/-DCMAKE_MSVC_RUNTIME_LIBRARY=[^\s]+/, '')
cmake_args = cmake_args.gsub(/-DCMAKE_BUILD_TYPE=[^\s]+/, '')
cmake_args += ' -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL -DCMAKE_BUILD_TYPE=Release'
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

    # First, replace any explicit -DCMAKE_MSVC_RUNTIME_LIBRARY=... or -DCMAKE_BUILD_TYPE=... occurrences
    $replacedTxt = $txt -replace '(-DCMAKE_MSVC_RUNTIME_LIBRARY=)[^"\s]+' , '${1}MultiThreadedDLL'
    $replacedTxt = $replacedTxt -replace '(-DCMAKE_BUILD_TYPE=)[^"\s]+' , '${1}Release'
    if ($replacedTxt -ne $txt) {
        Set-Content -Path $path -Value $replacedTxt
        Write-Host "Replaced explicit CMake arg values in $path"
        $patched = $true
        # refresh text for further checks
        $txt = $replacedTxt
    }

    # If still missing either arg, insert snippet before Dir.chdir(@build_dir) or before first cmake_args usage
    if ($txt -notmatch 'CMAKE_MSVC_RUNTIME_LIBRARY' -or $txt -notmatch 'CMAKE_BUILD_TYPE') {
        # Special-case: for build_llvm.rb insert right before the cmake_build.configure call
        if ($f -ieq 'build_llvm.rb') {
            $pattern3 = "(?m)(^\s*cmake_build\.configure\()"
            if ([regex]::IsMatch($txt, $pattern3)) {
                $replacement = "$snippet`n$1"
                $newTxt = [regex]::Replace($txt, $pattern3, $replacement)
                if ($newTxt -ne $txt) { Set-Content -Path $path -Value $newTxt; Write-Host "Patched $path (before cmake_build.configure)"; $patched = $true }
            }
                        # Also inject a post-configure check that fails the script if CMake configured Debug
                        $ruby_check = @'
# CI-inserted check: ensure CMake configured Release build type
cache_file = File.join(build_dir, "CMakeCache.txt")
if File.exist?(cache_file)
    cache = File.read(cache_file)
    if cache =~ /CMAKE_BUILD_TYPE:STRING=Debug/
        STDERR.puts "ERROR: CMake configured with Debug build type; aborting."
        exit(1)
    end
else
    STDERR.puts "WARNING: CMakeCache not found to verify build type."
end
'@

                        $pattern4 = "(?m)(^\s*cmake_build\.configure\([^\n]*\))"
                        if ([regex]::IsMatch($txt, $pattern4)) {
                                $newTxt2 = [regex]::Replace($txt, $pattern4, "$&`n$ruby_check", 1)
                                if ($newTxt2 -ne $txt) { Set-Content -Path $path -Value $newTxt2; Write-Host "Patched $path (post-configure check)"; $patched = $true }
                        }
        }

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
    }

    if (-not $patched) {
        Write-Host "No changes necessary for $path"
    }
}
