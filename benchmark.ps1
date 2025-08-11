# ThreadPool Benchmark Runner
# PowerShell script to run various benchmark configurations

param(
    [string]$Exe,
    [string]$Config = "standard",
    [switch]$All = $false,
    [switch]$Verbose = $false
)

$BenchmarkConfigs = @{
    "quick"    = @{tasks = 1000; threads = 4; iterations = 3; description = "Quick validation test" }
    "standard" = @{tasks = 5000; threads = 8; iterations = 5; description = "Standard performance baseline" }
    "stress"   = @{tasks = 20000; threads = 16; iterations = 3; description = "High load stress test" }
}

function Run-Benchmark {
    param($Name, $Config)
    
    Write-Host "`n=== Running $Name benchmark ===" -ForegroundColor Green
    Write-Host "Description: $($Config.description)" -ForegroundColor Yellow
    
    $testArgs = @("--tasks", $Config.tasks, "--iterations", $Config.iterations)
    
    $testArgs = $testArgs + @("--threads", $Config.threads)
    if ($Verbose) { $testArgs += "--verbose" }
    
    & $Exe $testArgs
}

function Main {
    Write-Host "ThreadPool Benchmark Runner" -ForegroundColor Magenta
    Write-Host "============================" -ForegroundColor Magenta

    if (-not (Test-Path $Exe)) {
        Write-Error "$Exe not found. Please build the project first."
        exit 1
    }
    
    if ($OutputFile) {
        Write-Host "Output will be saved to: $OutputFile"
        $originalOut = [Console]::Out
        $fileStream = [System.IO.File]::CreateText($OutputFile)
        [Console]::SetOut($fileStream)
    }
    
    try {
        if ($All) {
            foreach ($configName in $BenchmarkConfigs.Keys) {
                Run-Benchmark $configName $BenchmarkConfigs[$configName]
            }
        }
        elseif ($BenchmarkConfigs.ContainsKey($Config)) {
            Run-Benchmark $Config $BenchmarkConfigs[$Config]
        }
        else {
            Write-Error "Unknown configuration: $Config"
            Write-Host "Available configurations:"
            foreach ($configName in $BenchmarkConfigs.Keys) {
                $desc = $BenchmarkConfigs[$configName].description
                Write-Host "  $configName - $desc"
            }
            exit 1
        }
    }
    finally {
        if ($OutputFile) {
            [Console]::SetOut($originalOut)
            $fileStream.Close()
            Write-Host "Results saved to: $OutputFile"
        }
    }
    
    Write-Host "`nBenchmark completed!" -ForegroundColor Green
}

if (-not $Exe) {
    Write-Host "Usage: .\benchmark.ps1 -Exe <path_to_executable> [-Config <config_name>] [-All] [-Verbose]"
    Write-Host "`nAvailable configurations:"
    foreach ($configName in $BenchmarkConfigs.Keys | Sort-Object) {
        $desc = $BenchmarkConfigs[$configName].description
        Write-Host "  $configName - $desc" -ForegroundColor Cyan
    }
    Write-Host "`nExamples:"
    Write-Host "  .\benchmark.ps1 -Config quick"
    Write-Host "  .\benchmark.ps1 -All -Verbose"
    exit 0
}

Main
