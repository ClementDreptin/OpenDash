$ErrorActionPreference = "Stop"

# Make sure the XDK is installed.
if (-not $env:XEDK) {
    Write-Output "The XEDK environment variable is not defined. Make sure the Xbox 360 Software Development Kit is installed properly."
}

$RootDir = "$PSScriptRoot\.."

# Execute blast.
$XLastDir = "$RootDir\xlast"
$XLastProjectFilePath = "$XLastDir\OpenDash.xlast"
$XLastOutputDir = "$XLastDir\Online"
$BlastPath = "$env:XEDK\bin\win32\blast.exe"
& "$BlastPath" "$XLastProjectFilePath" /install:Local /nologo

# Create a temporary directory with the same structure as the output zip.
$TitleId = "B56870A9"
$PublisherOfferingId = "0FFFFFFF"
$OutputFileName = "$TitleId$PublisherOfferingId"
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
