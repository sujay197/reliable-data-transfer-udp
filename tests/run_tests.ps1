
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

Write-Host "`n=== Test Summary ==="
Write-Host "Passed: $passed" -ForegroundColor Green
Write-Host "Failed: $failed" -ForegroundColor Red

if ($failed -gt 0) {
    exit 1
}
