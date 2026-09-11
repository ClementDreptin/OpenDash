# This script generates macros of UTF-8 bytes from codepoints for icons in the Convsym font.
# This old compiler doesn't support u8 strings so we have to do the conversion ourselves.

$icons = @(
    @{ Name = "CHAR_BUTTON_A"; Codepoint = 0xF041 },
    @{ Name = "CHAR_BUTTON_B"; Codepoint = 0xF042 },
    @{ Name = "CHAR_BUTTON_X"; Codepoint = 0xF058 },
    @{ Name = "CHAR_BUTTON_Y"; Codepoint = 0xF059 },
    @{ Name = "CHAR_BUTTON_LB"; Codepoint = 0xF05F },
    @{ Name = "CHAR_BUTTON_RB"; Codepoint = 0xF060 },
    @{ Name = "CHAR_BUTTON_BACK"; Codepoint = 0xF03A },
    @{ Name = "CHAR_BUTTON_START"; Codepoint = 0xF03B }
)

function CodepointToUtf8Bytes {
    param([int]$Codepoint)

    $byte0 = 0xE0 -bor ($Codepoint -shr 12)
    $byte1 = 0x80 -bor (($Codepoint -shr 6) -band 0x3F)
    $byte2 = 0x80 -bor ($Codepoint -band 0x3F)

    return @($byte0, $byte1, $byte2)
}

foreach ($icon in $icons) {
    $bytes = CodepointToUtf8Bytes -Codepoint $icon.Codepoint
    $hexBytes = ($bytes | ForEach-Object { "\x{0:X2}" -f $_ }) -join ""
    "#define {0} `"{1}`" // U+{2:X4}" -f $icon.Name, $hexBytes, $icon.Codepoint
}
