# Import the common variables.
. "$PSScriptRoot\common-build.ps1"

# Make sure the Release binary is present.
if (![System.IO.File]::Exists("$RootDir\build\Release\bin\OpenDash.xex")) {
    throw "Release binary not found. Make sure to build in Release mode first."
}

# Execute blast.
& "$BlastPath" "$XLastDir\demo.xlast" /install:Local /nologo

# Create a temporary directory with the same structure as the output zip.
$BaseTmpDir = "$([System.IO.Path]::GetTempPath())\OpenDash"
$FullTmpDir = "$BaseTmpDir\Content\0000000000000000\$TitleId\00080000"
New-Item -ItemType Directory -Path $FullTmpDir -Force
Copy-Item -Path "$XLastOutputDir\$OutputFileName" -Destination "$FullTmpDir\$OutputFileName" -Force

# Create the zip.
$OutputZip = "$RootDir\OpenDash-demo.zip"
Compress-Archive -Path "$BaseTmpDir\*" -Destination "$OutputZip" -Force

# Remove the artifacts.
Remove-Item -Recurse $XLastOutputDir
Remove-Item -Recurse $BaseTmpDir
