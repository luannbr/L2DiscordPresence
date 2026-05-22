param(
  [Parameter(Mandatory)][string]$Config,
  [Parameter(Mandatory)][string]$Main
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $Config)) { throw "Config nao encontrado: $Config" }
if (-not (Test-Path -LiteralPath $Main))   { throw "main.cpp nao encontrado: $Main" }

$cfg = @{}
Get-Content -LiteralPath $Config | ForEach-Object {
    $line = $_.Trim()
    if ($line -eq '' -or $line.StartsWith('#') -or $line.StartsWith(';')) { return }
    $idx = $line.IndexOf('=')
    if ($idx -lt 1) { return }
    $k = $line.Substring(0, $idx).Trim()
    $v = $line.Substring($idx + 1).Trim()
    $cfg[$k] = $v
}

if (-not $cfg.ContainsKey('APP_ID')) { throw "APP_ID ausente em $Config" }
if ($cfg['APP_ID'] -notmatch '^\d+$') { throw "APP_ID precisa ser numerico, recebido: '$($cfg['APP_ID'])'" }

function Escape-CString([string]$s) {
    $s = $s.Replace('\', '\\')
    $s = $s.Replace('"', '\"')
    return $s
}

function Escape-RegexReplacement([string]$s) {
    return $s.Replace('$', '$$')
}

function Patch-Const([string]$text, [string]$name, [string]$value) {
    $esc = Escape-CString $value
    $replLiteral = "$name = `"$esc`""
    $repl = Escape-RegexReplacement $replLiteral
    $pattern = [regex]::Escape($name) + '\s*=\s*"[^"]*"'
    return [regex]::Replace($text, $pattern, $repl)
}

$src = Get-Content -LiteralPath $Main -Raw

# APP_ID is a string literal in this version: APP_ID = "12345"
$src = Patch-Const $src 'APP_ID' $cfg['APP_ID']

$fields = @{
    'details'       = 'DETAILS'
    'state'         = 'STATE'
    'large_image'   = 'LARGE_IMAGE'
    'large_text'    = 'LARGE_TEXT'
    'small_image'   = 'SMALL_IMAGE'
    'small_text'    = 'SMALL_TEXT'
    'button1_label' = 'BUTTON1_LABEL'
    'button1_url'   = 'BUTTON1_URL'
    'button2_label' = 'BUTTON2_LABEL'
    'button2_url'   = 'BUTTON2_URL'
}

foreach ($k in $fields.Keys) {
    if ($cfg.ContainsKey($k)) {
        $src = Patch-Const $src $fields[$k] $cfg[$k]
    }
}

[System.IO.File]::WriteAllText($Main, $src, (New-Object System.Text.UTF8Encoding $false))

Write-Host "[patch] APP_ID=$($cfg['APP_ID'])" -ForegroundColor Cyan
foreach ($k in 'details','state','large_image','large_text','small_image','small_text','button1_label','button1_url','button2_label','button2_url') {
    if ($cfg.ContainsKey($k)) {
        $v = $cfg[$k]
        if ([string]::IsNullOrEmpty($v)) { $v = '(vazio)' }
        Write-Host "[patch] $k=$v" -ForegroundColor Cyan
    }
}
Write-Host "[patch] main.cpp atualizado" -ForegroundColor Green
