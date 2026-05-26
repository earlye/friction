# Debugging Windows Preview Playback Issues

If you're experiencing blank preview playback on Windows — frames appear to cache correctly in the timeline but the screen goes black when playback starts (while audio continues) — follow these steps to capture a debug log.

## Step 1: Install a release build

Download and install the standard release build — the `friction-*-setup-win64.exe` installer from the CI artifacts or release page. Debug logging is enabled at runtime via an environment variable; no special debug build is required.

## Step 2: Capture a log

### Option A: Use the provided batch script (easiest)

1. Copy `src/scripts/run_debug_preview.bat` into the folder where `friction.exe` is installed (e.g. `C:\Program Files\Friction\`).
2. Double-click `run_debug_preview.bat`.
3. In Friction, open a project, hit play, and wait for the blank preview.
4. Close Friction.
5. Attach `friction-debug.log` (created in the same folder) to your issue report.

### Option B: Command Prompt

```bat
set QT_LOGGING_RULES=friction.renderhandler=true;friction.cachehandler=true;friction.canvas=true;friction.renderoutput=true;friction.audio=true
"C:\Program Files\Friction\friction.exe" > friction-debug.log 2>&1
```

### Option C: PowerShell

```powershell
$env:QT_LOGGING_RULES = "friction.renderhandler=true;friction.cachehandler=true;friction.canvas=true;friction.renderoutput=true;friction.audio=true"
& "C:\Program Files\Friction\friction.exe" *> friction-debug.log
```

## What to look for in the log

| Log entry | Meaning |
|---|---|
| `setSceneFrame(int): CACHE MISS` | Frame evicted before playback started |
| `frame not drawable: storesDataInMemory=false` | Frame was evicted to HDD |
| `frame not drawable: hasImage=false` | GPU rendered a null image (driver issue) |

## Reporting

File issues at: https://github.com/friction2d/friction/issues

Include: GPU model and driver version, Windows version, the `friction-debug.log` file, and steps to reproduce (project type, frame count, canvas size).
