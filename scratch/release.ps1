# igi1conv Packaging & Release Script (v1.7.0)
# Automatically packages the 32-bit (x86) and 64-bit (x64) Release folders into ZIP files.

$version = "1.7.0"
$x86Zip = "igi1conv_v${version}_x86.zip"
$x64Zip = "igi1conv_v${version}_x64.zip"

Write-Host "=================================================="
Write-Host "         Packaging igi1conv v$version Releases"
Write-Host "=================================================="

# 1. Compress 32-bit Release
if (Test-Path "bin/Release32") {
    # Remove opengl32sw.dll if present to keep release size small
    $opengl32 = "bin/Release32/opengl32sw.dll"
    if (Test-Path $opengl32) {
        Write-Host "Removing opengl32sw.dll from 32-bit build..."
        Remove-Item $opengl32 -Force
    }
    Write-Host "Compressing 32-bit binaries to $x86Zip..."
    if (Test-Path $x86Zip) { Remove-Item $x86Zip -Force }
    Compress-Archive -Path "bin/Release32/*" -DestinationPath $x86Zip -Force
    Write-Host "[SUCCESS] 32-bit package created successfully."
} else {
    Write-Warning "Directory bin/Release32 not found. Cannot package x86 build."
}

# 2. Compress 64-bit Release
if (Test-Path "bin/Release") {
    # Remove opengl32sw.dll if present to keep release size small
    $opengl32 = "bin/Release/opengl32sw.dll"
    if (Test-Path $opengl32) {
        Write-Host "Removing opengl32sw.dll from 64-bit build..."
        Remove-Item $opengl32 -Force
    }
    Write-Host "Compressing 64-bit binaries to $x64Zip..."
    if (Test-Path $x64Zip) { Remove-Item $x64Zip -Force }
    Compress-Archive -Path "bin/Release/*" -DestinationPath $x64Zip -Force
    Write-Host "[SUCCESS] 64-bit package created successfully."
} else {
    Write-Warning "Directory bin/Release not found. Cannot package x64 build."
}

Write-Host "=================================================="
Write-Host "ZIP files successfully generated!"
Write-Host "=================================================="
Write-Host "To publish this release to GitHub, copy and run the command below:"
Write-Host "gh release create v$version $x86Zip $x64Zip --title 'v$version - Text MEF/MEX Export Folder Selection' --notes-file CHANGELOG.md"
Write-Host "=================================================="
