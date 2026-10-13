param(
    [int]$Port = 8097,
    [string]$SourceRoot,
    [string]$DataDir,
    [string]$RexHelper,
    [string]$RexSdk
)
$ErrorActionPreference = 'Stop'
$labArguments = @((Join-Path $PSScriptRoot 'server.py'), "$Port")
if ($SourceRoot) { $labArguments += @('--source-root', $SourceRoot) }
if ($DataDir) { $labArguments += @('--data-dir', $DataDir) }
if ($RexHelper) { $labArguments += @('--rex-helper', $RexHelper) }
if ($RexSdk) { $labArguments += @('--rex-sdk', $RexSdk) }
python @labArguments
exit $LASTEXITCODE
