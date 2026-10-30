
$ErrorActionPreference = "Stop"

# Create graphs directory
New-Item -ItemType Directory -Force graphs | Out-Null

# Load comparison CSV
$data = Import-Csv "results\protocol_comparison.csv"

# -----------------------------------------
# GRAPH 1: Final Adaptive RTO Comparison
# -----------------------------------------

$protocols = @("Stop-and-Wait", "Go-Back-N", "Selective Repeat")
$scenarios = @("No Loss", "Packet Loss", "ACK Loss")
$colors = @("#2563eb", "#16a34a", "#d97706")

$width = 900
$height = 520
$chartTop = 80
$chartBottom = 410
$chartHeight = $chartBottom - $chartTop
$maxRto = 6000

$svg1 = @"
<svg xmlns="http://www.w3.org/2000/svg" width="$width" height="$height" viewBox="0 0 $width $height">
<rect width="100%" height="100%" fill="white"/>
<text x="450" y="35" text-anchor="middle" font-size="22" font-family="Arial">Final Adaptive RTO by Protocol</text>
"@

for ($tick = 0; $tick -le $maxRto; $tick += 1000) {
    $y = $chartBottom - ($tick / $maxRto * $chartHeight)
    $svg1 += "<line x1='90' y1='$y' x2='850' y2='$y' stroke='#dddddd'/>"
    $svg1 += "<text x='78' y='$($y + 5)' text-anchor='end' font-size='12' font-family='Arial'>$tick</text>"
}

$svg1 += "<text x='25' y='245' transform='rotate(-90 25 245)' text-anchor='middle' font-size='14' font-family='Arial'>Final RTO (ms)</text>"

$groupWidth = 220
$barWidth = 42

for ($s = 0; $s -lt $scenarios.Count; $s++) {
    for ($p = 0; $p -lt $protocols.Count; $p++) {

        $item = $data | Where-Object {
            $_.Protocol -eq $protocols[$p] -and
            $_.Test_Type -eq $scenarios[$s]
        }

        if ($item) {
            $value = [double]$item.Final_RTO_ms
            $barHeight = $value / $maxRto * $chartHeight
            $x = 150 + ($s * $groupWidth) + ($p * 52)
            $y = $chartBottom - $barHeight

            $svg1 += "<rect x='$x' y='$y' width='$barWidth' height='$barHeight' fill='$($colors[$p])'/>"
            $svg1 += "<text x='$($x + 21)' y='$($y - 7)' text-anchor='middle' font-size='10' font-family='Arial'>$([math]::Round($value))</text>"
        }
    }

    $labelX = 150 + ($s * $groupWidth) + 75
    $svg1 += "<text x='$labelX' y='440' text-anchor='middle' font-size='13' font-family='Arial'>$($scenarios[$s])</text>"
}

for ($p = 0; $p -lt $protocols.Count; $p++) {
    $x = 120 + ($p * 250)
    $svg1 += "<rect x='$x' y='475' width='14' height='14' fill='$($colors[$p])'/>"
    $svg1 += "<text x='$($x + 20)' y='487' font-size='12' font-family='Arial'>$($protocols[$p])</text>"
}

$svg1 += "</svg>"

Set-Content "graphs\final_rto_comparison.svg" $svg1 -Encoding UTF8

Write-Host "Created: final_rto_comparison.svg"


# -----------------------------------------
# GRAPH 2: Total Packets by Protocol
# -----------------------------------------

$packetData = @(
    @{Name = "Stop-and-Wait"; Count = 4},
    @{Name = "Go-Back-N"; Count = 12},
    @{Name = "Selective Repeat"; Count = 10}
)

$svg2 = @"
<svg xmlns="http://www.w3.org/2000/svg" width="800" height="450" viewBox="0 0 800 450">
<rect width="100%" height="100%" fill="white"/>
<text x="400" y="35" text-anchor="middle" font-size="22" font-family="Arial">Total Packets in Week 5 Experiments</text>
"@

# Y-axis grid
for ($tick = 0; $tick -le 12; $tick += 2) {
    $y = 350 - ($tick * 22)
    $svg2 += "<line x1='100' y1='$y' x2='750' y2='$y' stroke='#dddddd'/>"
    $svg2 += "<text x='85' y='$($y + 5)' text-anchor='end' font-size='12' font-family='Arial'>$tick</text>"
}

$svg2 += "<text x='25' y='220' transform='rotate(-90 25 220)' text-anchor='middle' font-size='14' font-family='Arial'>Number of Packets</text>"

for ($i = 0; $i -lt $packetData.Count; $i++) {
    $x = 170 + ($i * 200)
    $barHeight = $packetData[$i].Count * 22
    $y = 350 - $barHeight

    $svg2 += "<rect x='$x' y='$y' width='80' height='$barHeight' fill='#2563eb'/>"
    $svg2 += "<text x='$($x + 40)' y='$($y - 8)' text-anchor='middle' font-size='16' font-family='Arial'>$($packetData[$i].Count)</text>"
    $svg2 += "<text x='$($x + 40)' y='375' text-anchor='middle' font-size='13' font-family='Arial'>$($packetData[$i].Name)</text>"
}

$svg2 += "</svg>"

Set-Content "graphs\packet_count_comparison.svg" $svg2 -Encoding UTF8

Write-Host "Created: packet_count_comparison.svg"

Write-Host ""
Write-Host "Both graphs generated successfully."
