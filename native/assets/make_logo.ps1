# Gera o logo do Lettuce Live Editor: native/assets/app_icon_lettuce.ico (7 tamanhos) e native/src/ui/logo_data.inc (textura 128 px do app).
# Uso: powershell -ExecutionPolicy Bypass -File native\tools\make_logo.ps1
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
$assets = Join-Path $PSScriptRoot "..\assets"
$srcui = Join-Path $PSScriptRoot "..\src\ui"
$PI = [math]::PI

function Smooth([double]$a, [double]$b, [double]$x) {
    $t = [math]::Max(0.0, [math]::Min(1.0, ($x - $a) / ($b - $a)))
    return $t * $t * (3.0 - 2.0 * $t)
}
function Col([int]$a, [int]$r, [int]$g, [int]$b) { return [System.Drawing.Color]::FromArgb($a, $r, $g, $b) }

function New-RoundedPath([double]$x, [double]$y, [double]$w, [double]$h, [double]$r) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = [single]($r * 2)
    $p.AddArc([single]$x, [single]$y, $d, $d, 180, 90)
    $p.AddArc([single]($x + $w - $d), [single]$y, $d, $d, 270, 90)
    $p.AddArc([single]($x + $w - $d), [single]($y + $h - $d), $d, $d, 0, 90)
    $p.AddArc([single]$x, [single]($y + $h - $d), $d, $d, 90, 90)
    $p.CloseFigure()
    return $p
}

# contorno da cabeca: circulo com ondulacao suave (borda "frisada" como nas referencias)
function New-WavyCircle([double]$cx, [double]$cy, [double]$R, [double]$amp, [double]$k) {
    $pts = New-Object System.Collections.Generic.List[System.Drawing.PointF]
    $n = 180
    for ($i = 0; $i -lt $n; $i++) {
        $a = 2 * $PI * $i / $n
        $rr = $R * (1.0 + $amp * [math]::Pow([math]::Abs([math]::Sin($k * $a / 2.0)), 0.7) - $amp * 0.5)
        $pts.Add((New-Object System.Drawing.PointF([single]($cx + $rr * [math]::Cos($a)), [single]($cy + $rr * [math]::Sin($a)))))
    }
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $p.AddPolygon($pts.ToArray())
    return $p
}

function Half-W([double]$t, [double]$wid) {
    return $wid * [math]::Pow([math]::Max([math]::Sin($PI * [math]::Pow($t, 0.62)), 0.0), 0.48)
}

function Get-Leaf([double]$bx, [double]$by, [double]$ang, [double]$len, [double]$wid, [double]$ruf, [double]$cyc, [double]$phase) {
    $n = 64
    $ca = [math]::Cos($ang); $sa = [math]::Sin($ang)
    $loc = New-Object 'System.Collections.Generic.List[double[]]'
    for ($i = 0; $i -le $n; $i++) { $t = $i / $n; $loc.Add(@((-(Half-W $t $wid)), ($len * $t))) }
    for ($i = $n; $i -ge 0; $i--) { $t = $i / $n; $loc.Add(@((Half-W $t $wid), ($len * $t))) }
    $cnt = $loc.Count
    $cyl = $len * 0.5
    $pts = New-Object System.Collections.Generic.List[System.Drawing.PointF]
    for ($i = 0; $i -lt $cnt; $i++) {
        $p = $loc[$i]
        $a = $loc[($i - 1 + $cnt) % $cnt]; $b = $loc[($i + 1) % $cnt]
        $tx = $b[0] - $a[0]; $ty = $b[1] - $a[1]
        $tl = [math]::Sqrt($tx * $tx + $ty * $ty); if ($tl -lt 1e-9) { $tl = 1.0 }
        $nx = $ty / $tl; $ny = -$tx / $tl
        if (($nx * $p[0] + $ny * ($p[1] - $cyl)) -lt 0) { $nx = -$nx; $ny = -$ny }
        $u = $i / ($cnt - 1)
        $mask = (Smooth 0.08 0.26 $u) * (Smooth 0.92 0.74 $u)
        $e = $ruf * $len * [math]::Pow([math]::Abs([math]::Sin($PI * $cyc * $u + $phase)), 0.8) * $mask
        $x = $p[0] + $nx * $e; $y = -($p[1] + $ny * $e)
        $pts.Add((New-Object System.Drawing.PointF([single]($bx + $x * $ca - $y * $sa), [single]($by + $x * $sa + $y * $ca))))
    }
    return @{ Pts = $pts.ToArray(); Bx = $bx; By = $by; Ca = $ca; Sa = $sa; Len = $len; Wid = $wid }
}

function LeafPt($leaf, [double]$lx, [double]$ly) {
    $x = $lx; $y = -$ly
    return New-Object System.Drawing.PointF([single]($leaf.Bx + $x * $leaf.Ca - $y * $leaf.Sa), [single]($leaf.By + $x * $leaf.Sa + $y * $leaf.Ca))
}

# nervuras pretas: central + laterais curvas (espinha de peixe), como nas referencias
function Draw-Veins($g, $leaf, [double]$S, [bool]$rich, $ink) {
    $L = $leaf.Len; $W = $leaf.Wid
    $rw = [single][math]::Max(1.0, $S * 0.026)
    $mid = New-Object System.Drawing.Pen($ink, $rw)
    $mid.StartCap = [System.Drawing.Drawing2D.LineCap]::Round; $mid.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $g.DrawLine($mid, (LeafPt $leaf 0 ($L * 0.05)), (LeafPt $leaf 0 ($L * 0.74)))
    $side = New-Object System.Drawing.Pen($ink, [single][math]::Max(1.0, $rw * 0.72))
    $side.StartCap = [System.Drawing.Drawing2D.LineCap]::Round; $side.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $steps = if ($rich) { @(0.20, 0.36, 0.52) } else { @(0.28, 0.50) }
    foreach ($t in $steps) {
        $wt = (Half-W ([math]::Min($t + 0.16, 0.96)) $W) * 0.70
        foreach ($sgn in -1, 1) {
            $p0 = LeafPt $leaf 0 ($L * $t)
            $p1 = LeafPt $leaf ($sgn * $wt * 0.30) ($L * ($t + 0.01))
            $p2 = LeafPt $leaf ($sgn * $wt * 0.76) ($L * ($t + 0.06))
            $p3 = LeafPt $leaf ($sgn * $wt) ($L * ($t + 0.15))
            $g.DrawBezier($side, $p0, $p1, $p2, $p3)
        }
    }
    $mid.Dispose(); $side.Dispose()
}

# folha chapada (sem degrade) com contorno preto grosso
function Draw-LeafFlat($g, $leaf, $fill, [double]$S, [bool]$veins, [bool]$rich, $ink, [double]$outline) {
    $b = New-Object System.Drawing.SolidBrush($fill)
    $g.FillPolygon($b, $leaf.Pts); $b.Dispose()
    if ($veins) { Draw-Veins $g $leaf $S $rich $ink }
    $ep = New-Object System.Drawing.Pen($ink, [single][math]::Max(1.0, $outline))
    $ep.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
    $g.DrawPolygon($ep, $leaf.Pts); $ep.Dispose()
}

function Render-Logo([int]$S, [int]$ss, [bool]$simple) {
    $B = $S * $ss
    $bmp = New-Object System.Drawing.Bitmap($B, $B, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)
    $f = [double]$B
    $m = $f * 0.04
    $size = $f - 2 * $m
    $r = $size * 0.25
    $path = New-RoundedPath $m $m $size $size $r
    $ink = Col 255 22 26 22   # preto das referencias

    for ($k = 1; $k -le 3; $k++) {
        $sp = New-RoundedPath $m ($m + $f * 0.006 * $k) $size $size $r
        $sb = New-Object System.Drawing.SolidBrush((Col 22 0 0 0))
        $g.FillPath($sb, $sp); $sb.Dispose(); $sp.Dispose()
    }
    # fundo verde-floresta com um brilho suave atras da alface (combina com o app escuro)
    $bgRect = New-Object System.Drawing.RectangleF(0, 0, [single]$f, [single]$f)
    $bg = New-Object System.Drawing.Drawing2D.LinearGradientBrush($bgRect, (Col 255 34 108 58), (Col 255 7 44 24), [System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
    $g.FillPath($bg, $path); $bg.Dispose()
    $g.SetClip($path)
    $glp = New-Object System.Drawing.Drawing2D.GraphicsPath
    $glp.AddEllipse([single]($f * 0.12), [single]($f * 0.14), [single]($f * 0.76), [single]($f * 0.76))
    $glb = New-Object System.Drawing.Drawing2D.PathGradientBrush($glp)
    $glb.CenterColor = Col 110 150 215 70
    $glb.SurroundColors = @((Col 0 150 215 70))
    $g.FillPath($glb, $glp); $glb.Dispose(); $glp.Dispose()
    $g.ResetClip()

    # tudo fica recortado dentro do quadrado
    $g.SetClip($path)

    # alface aberta (roseta): folhas abrindo em leque, camadas de tras (escuras) para a frente (claras)
    $cx = $f * 0.5
    $bx = $cx; $by = $f * 0.805
    $ol = $f * $(if ($simple) { 0.030 } else { 0.021 })
    $rich = ($S -ge 96)
    $k1 = Col 255 66 134 48
    $k2 = Col 255 92 160 54
    $k3 = Col 255 126 190 62
    $k4 = Col 255 172 220 88
    $defs = @(
        @{ a = -1.08; len = 0.40; wid = 0.165; ruf = 0.050; cyc = 3.5; ph = 0.2; fill = $k1 },
        @{ a =  1.08; len = 0.40; wid = 0.165; ruf = 0.050; cyc = 3.5; ph = 1.4; fill = $k1 },
        @{ a = -0.66; len = 0.52; wid = 0.195; ruf = 0.050; cyc = 3.5; ph = 0.8; fill = $k2 },
        @{ a =  0.66; len = 0.52; wid = 0.195; ruf = 0.050; cyc = 3.5; ph = 2.0; fill = $k2 },
        @{ a = -0.26; len = 0.58; wid = 0.200; ruf = 0.055; cyc = 3.5; ph = 1.1; fill = $k3 },
        @{ a =  0.26; len = 0.58; wid = 0.200; ruf = 0.055; cyc = 3.5; ph = 0.5; fill = $k3 },
        @{ a =  0.00; len = 0.55; wid = 0.190; ruf = 0.060; cyc = 3.0; ph = 0.3; fill = $k4 }
    )
    foreach ($d in $defs) {
        $leaf = Get-Leaf $bx $by $d.a ($f * $d.len * 1.05) ($f * $d.wid * 1.05) $d.ruf $d.cyc $d.ph
        Draw-LeafFlat $g $leaf $d.fill $f ($S -ge 20) $rich $ink $ol
    }
    # base palida cortada
    $crx = $f * 0.085; $cry = $f * 0.036
    $cbr = New-Object System.Drawing.RectangleF([single]($bx - $crx), [single]($by - $cry * 0.5), [single]($crx * 2), [single]($cry * 2))
    $cb = New-Object System.Drawing.SolidBrush((Col 255 226 238 192))
    $g.FillEllipse($cb, $cbr); $cb.Dispose()
    $cp = New-Object System.Drawing.Pen($ink, [single][math]::Max(1.0, $ol))
    $g.DrawEllipse($cp, $cbr); $cp.Dispose()
    $g.ResetClip()

    # aro fino do quadrado
    $bp = New-Object System.Drawing.Pen((Col 190 150 215 80), [single]($f * 0.016))
    $g.DrawPath($bp, $path); $bp.Dispose()
    $g.Dispose()
    if ($ss -eq 1) { return $bmp }
    $out = New-Object System.Drawing.Bitmap($S, $S, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $go = [System.Drawing.Graphics]::FromImage($out)
    $go.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $go.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $go.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    $go.DrawImage($bmp, 0, 0, $S, $S)
    $go.Dispose(); $bmp.Dispose()
    return $out
}

function To-PngBytes($bmp) {
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    return ,$ms.ToArray()
}

$sizes = 16, 24, 32, 48, 64, 128, 256
$pngs = @()
foreach ($s in $sizes) {
    $ss = if ($s -le 48) { 8 } elseif ($s -le 128) { 4 } else { 3 }
    $bm = Render-Logo $s $ss ($s -le 48)
    $pngs += ,(To-PngBytes $bm)
}
$ico = Join-Path $assets "app_icon_lettuce.ico"
$fs = [System.IO.File]::Create($ico)
$bw = New-Object System.IO.BinaryWriter($fs)
$bw.Write([uint16]0); $bw.Write([uint16]1); $bw.Write([uint16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $s = $sizes[$i]
    $wb = if ($s -ge 256) { 0 } else { $s }
    $bw.Write([byte]$wb); $bw.Write([byte]$wb); $bw.Write([byte]0); $bw.Write([byte]0)
    $bw.Write([uint16]1); $bw.Write([uint16]32)
    $bw.Write([uint32]$pngs[$i].Length); $bw.Write([uint32]$offset)
    $offset += $pngs[$i].Length
}
foreach ($p in $pngs) { $bw.Write($p) }
$bw.Close(); $fs.Close()
"ico: $ico ($((Get-Item $ico).Length) bytes)"

$b128 = Render-Logo 128 4 $true
$rect = New-Object System.Drawing.Rectangle(0, 0, 128, 128)
$data = $b128.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$raw = New-Object byte[] (128 * 128 * 4)
[System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $raw, 0, $raw.Length)
$b128.UnlockBits($data)
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("// Gerado por make_logo.ps1: logo Lettuce Live Editor, 128x128 RGBA (alfa direto).")
[void]$sb.AppendLine("static const int kLogoSize = 128;")
[void]$sb.AppendLine("static const unsigned char kLogoRgba[128 * 128 * 4] = {")
for ($i = 0; $i -lt $raw.Length; $i += 4) {
    $rr = $raw[$i + 2]; $gg = $raw[$i + 1]; $bb = $raw[$i]; $aa = $raw[$i + 3]
    if ($aa -eq 0) { $rr = 20; $gg = 76; $bb = 40 }   # cor de fundo nos pixels transparentes: sem franja escura nos mipmaps
    [void]$sb.Append(('0x{0:x2},0x{1:x2},0x{2:x2},0x{3:x2},' -f $rr, $gg, $bb, $aa))
    if ((($i / 4) % 8) -eq 7) { [void]$sb.AppendLine() }
}
[void]$sb.AppendLine("};")
[System.IO.File]::WriteAllText((Join-Path $srcui "logo_data.inc"), $sb.ToString())
"inc: $((Get-Item (Join-Path $srcui 'logo_data.inc')).Length) bytes"

