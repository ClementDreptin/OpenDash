$ErrorActionPreference = "Stop"

# Make sure the XDK is installed.
if (-not $env:XEDK) {
    Write-Output "The XEDK environment variable is not defined. Make sure the Xbox 360 Software Development Kit is installed properly."
}

# Declare the common variables.
$RootDir = "$PSScriptRoot\.."
$XLastDir = "$RootDir\xlast"
$XLastOutputDir = "$XLastDir\Online"
$BlastPath = "$env:XEDK\bin\win32\blast.exe"
$TitleId = "B56870A9"
$PublisherOfferingId = "0FFFFFFF"
$OutputFileName = "$TitleId$PublisherOfferingId"
