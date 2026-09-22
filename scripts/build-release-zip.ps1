# Import the common variables.
. "$PSScriptRoot\common-build.ps1"

$BuildDir = "$RootDir\build\Release\bin"

# Make sure the Release binary is present.
if (![System.IO.File]::Exists("$BuildDir\OpenDash.xex")) {
    throw "Release binary not found. Make sure to build in Release mode first."
}

$XexPath = "$BuildDir\OpenDash.xex"
$NxeartPath = "$BuildDir\nxeart"
$AssetsDir = "$BuildDir\assets"

# Create a temporary directory with the same structure as the output zip.
$BaseTmpDir = "$([System.IO.Path]::GetTempPath())\OpenDash"
New-Item -ItemType Directory -Path $BaseTmpDir
Copy-Item -Path "$XexPath" -Destination "$BaseTmpDir"
Copy-Item -Path "$NxeartPath" -Destination "$BaseTmpDir" -Recurse
Copy-Item -Path "$AssetsDir" -Destination "$BaseTmpDir" -Recurse

# Create the zip.
$OutputZip = "$RootDir\OpenDash.zip"
Compress-Archive -Path "$BaseTmpDir\*" -Destination "$OutputZip" -Force

# Remove the artifacts.
Remove-Item -Recurse $BaseTmpDir
