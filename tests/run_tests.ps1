
$ErrorActionPreference = "Stop"

Write-Host "`n=== Reliable UDP Automated Tests ===" -ForegroundColor Cyan

# Ensure the build directory exists.
New-Item -ItemType Directory -Force -Path "build" | Out-Null

$passed = 0
$failed = 0

function Record-Result($name, $success) {
    if ($success) {
        Write-Host "PASS: $name" -ForegroundColor Green
        $script:passed++
    }
    else {
        Write-Host "FAIL: $name" -ForegroundColor Red
        $script:failed++
    }
}

# Test 1: Compile duplicate-packet test.
Write-Host "`nTest 1: Compile duplicate-packet test"
& gcc tests\test_duplicate.c -o build\test_duplicate.exe -lws2_32
Record-Result "Duplicate test compilation" ($LASTEXITCODE -eq 0)

# Test 2: Compile invalid-ACK test.
Write-Host "`nTest 2: Compile invalid-ACK test"
& gcc tests\test_invalid_ack.c -o build\test_invalid_ack.exe -lws2_32
Record-Result "Invalid-ACK test compilation" ($LASTEXITCODE -eq 0)

# Test 3: Run invalid-ACK behavior test.
# The sender must be built already as build\sender_timeout.exe.
if ((Test-Path "build\sender_timeout.exe") -and
    (Test-Path "build\test_invalid_ack.exe")) {

    Write-Host "`nTest 3: Invalid-ACK behavior"

    $receiver = Start-Process `
        -FilePath (Resolve-Path "build\test_invalid_ack.exe").Path `
        -PassThru `
        -NoNewWindow `
        -RedirectStandardOutput "build\invalid_ack_receiver.log" `
        -RedirectStandardError "build\invalid_ack_receiver_error.log"

    try {
        Start-Sleep -Seconds 1

        # Send Hello followed by exit to the sender's standard input.
        $senderOutput = "Hello`nexit`n" |
            & .\build\sender_timeout.exe 2>&1 |
            Out-String

        $senderExitCode = $LASTEXITCODE

        $senderOutput | Set-Content "build\invalid_ack_sender.log"

        Start-Sleep -Milliseconds 500

        $receiverOutput = ""
        if (Test-Path "build\invalid_ack_receiver.log") {
            $receiverOutput = Get-Content "build\invalid_ack_receiver.log" -Raw
        }

        $rejectedAll = (
            ([regex]::Matches($senderOutput, "Unexpected ACK=99")).Count -eq 5
        )

        $stoppedSafely = (
            $senderOutput -match "Maximum attempts reached" -and
            $senderOutput -match "Transmission failed"
        )

        $receiverAnsweredAll = (
            ([regex]::Matches($receiverOutput, "Sent invalid ACK: ACK=99")).Count -eq 5
        )

        Record-Result "Sender rejects five invalid ACKs and stops safely" (
            $rejectedAll -and $stoppedSafely -and $receiverAnsweredAll
        )

        if (-not ($rejectedAll -and $stoppedSafely -and $receiverAnsweredAll)) {
            Write-Host "`nSender output:" -ForegroundColor Yellow
            Write-Host $senderOutput
            Write-Host "`nReceiver log:" -ForegroundColor Yellow
            Write-Host $receiverOutput
        }
    }
    finally {
        if (-not $receiver.HasExited) {
            Stop-Process -Id $receiver.Id -Force
        }
    }
}
else {
    Record-Result "Invalid-ACK behavior prerequisites exist" $false
    Write-Host "Build build\sender_timeout.exe first if it is missing."
}

# Test 4: Duplicate-packet behavior.
Write-Host "`nTest 4: Duplicate-packet behavior"

if ((Test-Path "tests\receiver_duplicate_win.c") -and
    (Test-Path "build\test_duplicate.exe")) {

    # Compile the duplicate-packet receiver.
    & gcc tests\receiver_duplicate_win.c -o build\receiver_duplicate_win.exe -lws2_32

    if ($LASTEXITCODE -eq 0) {
        $receiver = Start-Process `
            -FilePath (Resolve-Path "build\receiver_duplicate_win.exe").Path `
            -PassThru `
            -NoNewWindow `
            -RedirectStandardOutput "build\duplicate_receiver.log" `
            -RedirectStandardError "build\duplicate_receiver_error.log"

        try {
            Start-Sleep -Seconds 1

            $testOutput = & .\build\test_duplicate.exe 2>&1 | Out-String
            $testExitCode = $LASTEXITCODE

            Start-Sleep -Milliseconds 500

            $receiverOutput = ""
            if (Test-Path "build\duplicate_receiver.log") {
                $receiverOutput = Get-Content "build\duplicate_receiver.log" -Raw
            }

            $duplicatePassed = (
                $testExitCode -eq 0 -and
                ([regex]::Matches($testOutput, "PASS: ACK=0 received")).Count -eq 2
            )

            Record-Result "Original and duplicate packets both receive ACK=0" $duplicatePassed

            if (-not $duplicatePassed) {
                Write-Host "`nDuplicate test output:" -ForegroundColor Yellow
                Write-Host $testOutput
                Write-Host "`nReceiver log:" -ForegroundColor Yellow
                Write-Host $receiverOutput
            }
        }
        finally {
            if (-not $receiver.HasExited) {
                Stop-Process -Id $receiver.Id -Force
            }
        }
    }
    else {
        Record-Result "Compile duplicate-packet receiver" $false
    }
}
else {
    Record-Result "Duplicate-packet test prerequisites exist" $false
    Write-Host "Check tests\receiver_duplicate_win.c and build\test_duplicate.exe."
}

Write-Host "`n=== Test Summary ==="
Write-Host "Passed: $passed" -ForegroundColor Green
Write-Host "Failed: $failed" -ForegroundColor Red

if ($failed -gt 0) {
    exit 1
}
