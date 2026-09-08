param($templateName)
$rootPath = $PSScriptRoot / '../../..'
$templateDir = Join-Path $rootPath 'templates'
$templateFile = Join-Path $templateDir "$templateName.md"

if (-not (Test-Path $templateFile)) {
    Write-Error "Template not found: $templateFile"
    exit 1
}

$content = Get-Content -Path $templateFile -Raw -Encoding UTF8
Write-Output $content
