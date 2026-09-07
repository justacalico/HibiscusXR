# 3s 440Hz stereo 48k/16-bit WAV, for an actual "do the speakers make noise" test.
# 48k because that's the native rate of the sdm845 primary path; avoids any
# resampler in the way of the question we're asking.
$path = 'F:\PN2Lineage\notes\tone440.wav'
$rate = 48000; $secs = 3; $freq = 440.0; $ch = 2; $bits = 16
$n = $rate * $secs
$dataBytes = $n * $ch * ($bits/8)

$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter($ms)
$bw.Write([char[]]'RIFF')
$bw.Write([int](36 + $dataBytes))
$bw.Write([char[]]'WAVE')
$bw.Write([char[]]'fmt ')
$bw.Write([int]16)
$bw.Write([int16]1)                              # PCM
$bw.Write([int16]$ch)
$bw.Write([int]$rate)
$bw.Write([int]($rate * $ch * ($bits/8)))        # byte rate
$bw.Write([int16]($ch * ($bits/8)))              # block align
$bw.Write([int16]$bits)
$bw.Write([char[]]'data')
$bw.Write([int]$dataBytes)

for ($i = 0; $i -lt $n; $i++) {
  # fade in/out so a bad clock sounds like a click, not a pop we'd blame on it
  $env = [Math]::Min(1.0, [Math]::Min($i, $n - $i) / ($rate * 0.05))
  $s = [int16](12000 * $env * [Math]::Sin(2 * [Math]::PI * $freq * $i / $rate))
  $bw.Write($s); $bw.Write($s)
}
$bw.Flush()
[System.IO.File]::WriteAllBytes($path, $ms.ToArray())
$bw.Close()
"wrote $path ($((Get-Item $path).Length) bytes)"
