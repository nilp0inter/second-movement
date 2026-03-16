---
name: build-and-serve
description: >
  Build the second-movement project, fix compilation errors and session-modified-file warnings
  automatically, then serve the simulator. Use this skill whenever the user asks to build,
  compile, test, run, or preview the project — or after making code changes that should be
  verified in the simulator. Also trigger when the user says things like "let me see it",
  "check if it compiles", "run it", or "open the simulator".
---

# Build and Serve — second-movement

This skill covers the full build → fix → serve loop for the second-movement project.

---

## Tracked files

From the **start of each Claude Code session**, maintain a list of every source file you create
or edit. Call it the **session file set**. You'll use it to decide which warnings to fix.

---

## Build command

Always run from the repo root:

```bash
emmake make clean && emmake make BOARD=sensorwatch_pro DISPLAY=custom TIMESET=day
```

---

## Error & warning handling loop

After every build attempt:

1. **Errors** (build failed): Fix all errors, then recompile. Repeat until the build succeeds.

2. **Warnings on session files**: Once the build succeeds, scan the compiler output for warnings
   whose file path matches any file in the **session file set**. Fix them, then recompile.
   Repeat until no such warnings remain.

3. **Warnings on other files**: Ignore entirely — do not touch files outside the session file set.

4. **Clean build**: Proceed to the serve step.

> If after several iterations errors or session-file warnings persist and you're unsure how to
> fix them, stop and explain the situation to the user rather than looping indefinitely.

---

## Serve step

Once a clean build is achieved, start the HTTP server in the background:

```bash
python3 -m http.server -d build-sim -b 0.0.0.0 8000 &
```

Then open the simulator in the user's browser:

```bash
xdg-open http://127.0.0.1:8000/firmware.html
```

Tell the user the simulator is available at **http://127.0.0.1:8000/firmware.html** and which files
were modified during the session (if any warnings were fixed).

---

## Summary to give the user

After everything completes, briefly report:
- ✅ Build succeeded
- Any files modified to fix errors or warnings (or "no fixes needed")
- 🌐 Simulator opened at http://127.0.0.1:8000/firmware.html
