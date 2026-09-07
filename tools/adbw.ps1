<#
Wireless adb for the Pico headsets.

The Neo 2 never re-enumerates on USB after a reboot, so every reboot costs a
physical unplug/replug. Over TCP that goes away: adbd keeps listening and we just
reconnect.

  .\adbw.ps1                 set up wireless on whatever is plugged in, then connect
  .\adbw.ps1 -Watch          stay running and reconnect automatically across reboots
  .\adbw.ps1 -Persist        also bake persist.adb.tcp.port into /system (needs root)
  .\adbw.ps1 -Status         show what is connected right now
  .\adbw.ps1 -Forget         drop saved state and disconnect everything

Saved endpoints live in adbw.json next to this script, so a later run can
reconnect with nothing plugged in at all.
#>
[CmdletBinding()]
param(
    [switch]$Watch,
    [switch]$Persist,
    [switch]$Status,
    [switch]$Forget,
    [string]$Serial,
    [int]$Port = 5555,
    [int]$IntervalSec = 10,
    [string]$Adb = 'C:\adb\adb.exe'
)

$ErrorActionPreference = 'Stop'
$StateFile = Join-Path $PSScriptRoot 'adbw.json'

function Say($msg, $colour = 'Gray') { Write-Host ("  " + $msg) -ForegroundColor $colour }
function Head($msg) { Write-Host ""; Write-Host $msg -ForegroundColor Cyan }

function Adb { & $Adb @args 2>&1 }

function Get-Devices {
    # returns objects: Serial, State, IsNetwork
    $out = Adb devices
    $list = @()
    foreach ($line in $out) {
        $t = "$line".Trim()
        if ($t -match '^(\S+)\s+(device|offline|unauthorized)$') {
            $list += [pscustomobject]@{
                Serial    = $matches[1]
                State     = $matches[2]
                IsNetwork = $matches[1] -match ':\d+$'
            }
        }
    }
    return $list
}

function Load-State {
    if (Test-Path $StateFile) {
        try { return Get-Content $StateFile -Raw | ConvertFrom-Json } catch { }
    }
    return [pscustomobject]@{ endpoints = @() }
}

function Save-State($state) {
    $state | ConvertTo-Json -Depth 5 | Set-Content -Encoding ASCII $StateFile
}

function Get-DeviceIp($serial) {
    # wlan0 first; some builds land on a different iface, so fall back to any
    # global-scope v4 address that is not loopback or a usb/rndis tether.
    $out = Adb -s $serial shell "ip -f inet addr show 2>/dev/null"
    $cur = $null
    foreach ($line in $out) {
        $t = "$line".Trim()
        if ($t -match '^\d+:\s+(\S+?):') { $cur = $matches[1] }
        elseif ($t -match '^inet\s+(\d+\.\d+\.\d+\.\d+)') {
            $ip = $matches[1]
            if ($ip -like '127.*') { continue }
            if ($cur -match 'rndis|usb|dummy') { continue }
            return [pscustomobject]@{ Ip = $ip; Iface = $cur }
        }
    }
    return $null
}

function Enable-Tcp($serial, $port) {
    # `adb tcpip` restarts adbd, which drops THIS usb session. That is expected;
    # the device keeps listening on the port afterwards.
    Say "switching adbd to tcp:$port (the usb session will drop, that is normal)"
    Adb -s $serial tcpip $port | Out-Null
    Start-Sleep -Seconds 3
}

function Try-Connect($endpoint) {
    $r = (Adb connect $endpoint) -join ' '
    if ($r -match 'connected to') { return $true }
    return $false
}

function Set-Persist($serial, $port) {
    # persist.adb.tcp.port is read by adbd at start, so baking it into build.prop
    # makes wireless survive reboots with no host involvement at all.
    Say "writing persist.adb.tcp.port=$port into /system/build.prop"
    $sh = @(
        'mount -o rw,remount /system',
        "sed -i -E '/^persist\.adb\.tcp\.port=/d' /system/build.prop",
        # guarantee a trailing newline or the append glues onto the last property
        '[ -n "$(tail -c1 /system/build.prop)" ] && echo "" >> /system/build.prop',
        "echo 'persist.adb.tcp.port=$port' >> /system/build.prop",
        'sync',
        "setprop persist.adb.tcp.port $port",
        'grep -nE "^[a-z].*=.*[a-z]+\.[a-z].*=" /system/build.prop && echo "WARNING glued lines" || echo "build.prop clean"'
    ) -join "`n"
    $tmp = Join-Path $env:TEMP 'adbw_persist.sh'
    [System.IO.File]::WriteAllText($tmp, $sh + "`n")
    Adb -s $serial push $tmp /data/local/tmp/adbw_persist.sh | Out-Null
    (Adb -s $serial shell "su -c 'sh /data/local/tmp/adbw_persist.sh'") |
        ForEach-Object { Say ("  " + "$_".Trim()) }
}

# ---------------------------------------------------------------- actions ----

if ($Forget) {
    Head "Forgetting saved endpoints"
    $state = Load-State
    foreach ($e in $state.endpoints) { Adb disconnect $e | Out-Null; Say "disconnected $e" }
    Remove-Item $StateFile -ErrorAction SilentlyContinue
    Say "state cleared" 'Green'
    return
}

if ($Status) {
    Head "adb devices"
    $d = Get-Devices
    if (-not $d) { Say "nothing connected" 'Yellow' }
    foreach ($x in $d) {
        $kind = if ($x.IsNetwork) { 'wireless' } else { 'usb     ' }
        $col  = if ($x.State -eq 'device') { 'Green' } else { 'Yellow' }
        Say ("{0}  {1,-28} {2}" -f $kind, $x.Serial, $x.State) $col
    }
    $state = Load-State
    if ($state.endpoints) { Head "saved endpoints"; $state.endpoints | ForEach-Object { Say $_ } }
    return
}

function Connect-Once {
    $state = Load-State
    $devices = Get-Devices

    # A usb device that is not yet wireless is the whole point of this script, so
    # it takes priority. Checking "is anything wireless" first was wrong: with a
    # second headset already on the network it short-circuited and never promoted
    # the one actually plugged in.
    $usb = $devices | Where-Object { -not $_.IsNetwork -and $_.State -eq 'device' }
    if ($Serial) { $usb = $usb | Where-Object { $_.Serial -eq $Serial } }
    $usb = $usb | Select-Object -First 1

    if (-not $usb) {
        # nothing on usb: either we are already done, or this is the reboot case
        # where the device is listening but the host dropped the socket.
        $live = $devices | Where-Object { $_.IsNetwork -and $_.State -eq 'device' }
        if ($Serial) { $live = $live | Where-Object { $_.Serial -like "$Serial*" } }
        if ($live) {
            foreach ($x in $live) { Say ("already wireless: " + $x.Serial) 'Green' }
            return $true
        }
        foreach ($e in $state.endpoints) {
            if (Try-Connect $e) { Say "reconnected $e" 'Green'; return $true }
        }
        Say "no usb device and no saved endpoint reachable" 'Yellow'
        return $false
    }

    Say ("usb device: " + $usb.Serial)
    $net = Get-DeviceIp $usb.Serial
    if (-not $net) {
        Say "device has no wifi address - connect it to a network first" 'Yellow'
        Say "(wireless adb needs the headset on the same LAN as this pc)" 'Yellow'
        return $false
    }
    Say ("address: {0} on {1}" -f $net.Ip, $net.Iface)

    if ($Persist) { Set-Persist $usb.Serial $Port }

    Enable-Tcp $usb.Serial $Port
    $endpoint = "{0}:{1}" -f $net.Ip, $Port
    for ($i = 0; $i -lt 5; $i++) {
        if (Try-Connect $endpoint) {
            Say "connected $endpoint" 'Green'
            if ($state.endpoints -notcontains $endpoint) {
                $state.endpoints = @($state.endpoints | Where-Object { $_ }) + $endpoint
                Save-State $state
                Say "saved for future runs"
            }
            return $true
        }
        Start-Sleep -Seconds 2
    }
    Say "could not reach $endpoint after enabling tcpip" 'Red'
    return $false
}

Head "Wireless adb"
$ok = Connect-Once

if (-not $Watch) {
    if ($ok) {
        Head "Ready"
        Say "you can unplug the usb cable now" 'Green'
        Say "adb -s <endpoint> ... , or just adb ... if it is the only device"
    }
    return
}

# --watch: survive reboots without anyone touching the cable
Head "Watching (Ctrl-C to stop)"
$wasUp = $ok
while ($true) {
    Start-Sleep -Seconds $IntervalSec
    $devices = Get-Devices
    $live = $devices | Where-Object { $_.IsNetwork -and $_.State -eq 'device' }
    if ($live) {
        if (-not $wasUp) { Say ("back: " + ($live | ForEach-Object Serial)) 'Green' }
        $wasUp = $true
        continue
    }
    if ($wasUp) { Say "link lost (reboot?), retrying" 'Yellow'; $wasUp = $false }
    # a rebooting device refuses the port until adbd is up; Connect-Once handles it
    $null = Connect-Once
    if ((Get-Devices | Where-Object { $_.IsNetwork -and $_.State -eq 'device' })) { $wasUp = $true }
}
