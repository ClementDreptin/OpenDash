# Import the common variables.
. "$PSScriptRoot\common-build.ps1"

# Execute blast.
& "$BlastPath" "$XLastDir\demo.xlast" /install:Local /nologo

# Create a temporary directory with the same structure as the output zip.
$BaseTmpDir = "$([System.IO.Path]::GetTempPath())\OpenDash"
$FullTmpDir = "$BaseTmpDir\Content\0000000000000000\$TitleId\00080000"
New-Item -ItemType Directory -Path $FullTmpDir -Force
Copy-Item -Path "$XLastOutputDir\$OutputFileName" -Destination "$FullTmpDir\$OutputFileName" -Force

# Create the zip.
$OutputZip = "$RootDir\OpenDash.zip"
Compress-Archive -Path "$BaseTmpDir\*" -Destination "$OutputZip" -Force

# Remove the artifacts.
Remove-Item -Recurse $XLastOutputDir
Remove-Item -Recurse $BaseTmpDir
