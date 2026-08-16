param (
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet("Debug", "Release")]
    [string]$BuildConfig
)

# Import the common variables.
. "$PSScriptRoot\common-build.ps1"

# Execute blast.
& "$BlastPath" "$XLastDir\nxeart.xlast" /install:Local /nologo

# Copy the nxeart package to the build directory.
$BuildDir = "$RootDir\build\$BuildConfig\bin"
Copy-Item -Path "$XLastOutputDir\$OutputFileName" -Destination "$BuildDir\nxeart"

# Remove the artifacts.
Remove-Item -Recurse $XLastOutputDir
