param(
    [int]$Frames = 20000,
    [int]$Seed = 20260428
)

$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$ResultDir = Join-Path $Root ("all_results_frames_" + $Frames)
$LogDir = Join-Path $ResultDir "logs"
New-Item -ItemType Directory -Force -Path $ResultDir | Out-Null
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

$Experiments = @(
    @{Folder="Experiment1_Baseline_RouteValidation"; Exe="exp1.exe"; Csv="exp1_results.csv"},
    @{Folder="Experiment2_Hotspot_vs_UniformParity"; Exe="exp2.exe"; Csv="exp2_results.csv"},
    @{Folder="Experiment3_Hotspot_vs_ExtParityV2"; Exe="exp3.exe"; Csv="exp3_results.csv"},
    @{Folder="Experiment4_AdaptiveStructV2_Core"; Exe="exp4.exe"; Csv="exp4_results.csv"},
    @{Folder="Experiment5_MainResult_R0348"; Exe="exp5.exe"; Csv="exp5_results.csv"},
    @{Folder="Experiment6_MainResult_R04"; Exe="exp6.exe"; Csv="exp6_results.csv"},
    @{Folder="Experiment7_Generalization_MultiMatrix"; Exe="exp7.exe"; Csv="exp7_results.csv"},
    @{Folder="Experiment8_ThresholdScan"; Exe="exp8.exe"; Csv="exp8_results.csv"},
    @{Folder="Experiment9_Engineering_AdaptiveSelect_vs_Struct"; Exe="exp9.exe"; Csv="exp9_results.csv"}
)

Write-Host "===================================================="
Write-Host "AdaptiveStructV2 final thesis experiments"
Write-Host "Root   : $Root"
Write-Host "Frames : $Frames"
Write-Host "Seed   : $Seed"
Write-Host "Output : $ResultDir"
Write-Host "===================================================="

try {
    $gppVersion = & g++ --version
    Write-Host "g++ found: $($gppVersion[0])"
} catch {
    Write-Host "ERROR: g++ not found." -ForegroundColor Red
    exit 1
}

foreach ($Exp in $Experiments) {
    $Folder = $Exp.Folder
    $Exe = $Exp.Exe
    $Csv = $Exp.Csv
    $ExpDir = Join-Path $Root $Folder

    Write-Host ""
    Write-Host "===================================================="
    Write-Host "Running $Folder"
    Write-Host "===================================================="

    Push-Location $ExpDir
    if (Test-Path $Exe) { Remove-Item $Exe -Force }
    if (Test-Path $Csv) { Remove-Item $Csv -Force }

    $CompileLog = Join-Path $LogDir ($Folder + "_compile.log")
    $RunLog = Join-Path $LogDir ($Folder + "_run.log")

    $CompileCmd = "g++ -std=c++17 -O2 -fopenmp -I..\common main.cpp -o $Exe"
    cmd /c "$CompileCmd > `"$CompileLog`" 2>&1"
    if (!(Test-Path $Exe)) {
        $CompileCmd2 = "g++ -std=c++17 -O2 -I..\common main.cpp -o $Exe"
        cmd /c "$CompileCmd2 >> `"$CompileLog`" 2>&1"
    }
    if (!(Test-Path $Exe)) {
        Write-Host "Compile failed. Check $CompileLog" -ForegroundColor Red
        Pop-Location
        exit 1
    }

    $RunCmd = ".\$Exe $Frames $Csv $Seed"
    $Start = Get-Date
    cmd /c "$RunCmd > `"$RunLog`" 2>&1"
    $End = Get-Date
    if (Test-Path $RunLog) { Get-Content $RunLog -Tail 6 }

    if (!(Test-Path $Csv)) {
        Write-Host "Run failed. CSV not generated. Check $RunLog" -ForegroundColor Red
        Pop-Location
        exit 1
    }

    Copy-Item $Csv (Join-Path $ResultDir ($Folder + "_" + $Csv)) -Force
    Write-Host "Finished $Folder. Elapsed: $($End - $Start)"
    Pop-Location
}

Write-Host ""
Write-Host "All experiments finished."
Write-Host "Results folder: $ResultDir"
