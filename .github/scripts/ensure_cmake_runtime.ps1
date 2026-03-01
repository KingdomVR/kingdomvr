param(
    [string]$FilePath = "work/glare-core/scripts/cmake.rb"
)

Write-Host "Ensuring CMAKE_MSVC_RUNTIME_LIBRARY insertion in: $FilePath"
if (!(Test-Path $FilePath)) { Write-Host "$FilePath not found; skipping"; exit 0 }
$txt = Get-Content $FilePath -Raw
if ($txt -match 'CMAKE_MSVC_RUNTIME_LIBRARY') { Write-Host 'CMAKE_MSVC_RUNTIME_LIBRARY already present; skipping'; exit 0 }

$insertion = @"
# Ensure MSVC runtime set to MultiThreadedDLL to avoid debug CRT linkage
unless cmake_args.include?("CMAKE_MSVC_RUNTIME_LIBRARY")
  cmake_args += " -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL"
end
unless cmake_args.include?("CMAKE_BUILD_TYPE")
    cmake_args += " -DCMAKE_BUILD_TYPE=Release"
end
"@

$pattern = "(?m)(^\s*Dir\.chdir\(@build_dir\) do)"
$replacement = "$insertion`n$1"
$newTxt = [regex]::Replace($txt, $pattern, $replacement)
if ($newTxt -ne $txt) {
    Set-Content -Path $FilePath -Value $newTxt
    Write-Host "Patched $FilePath"
} else {
    Write-Host "Pattern not found; no changes made to $FilePath"
}
