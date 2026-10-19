# Automatic Code Improvement Workflow

The terminal workflow does not ask users to choose preferences. It infers likely improvement opportunities from the source and automatically runs the transformation pipeline.

## Run

```powershell
py .\adaptive_cli.py .\examples\adaptive_constant.c
```

Supported extensions: `.c`, `.cpp`, `.cc`, `.cxx`, `.py`, `.java`.

The program applies conservative deterministic transformations, then attempts an optional local Ollama code-model transformation. It validates the candidate and saves a separate improved file plus a report. If local AI is unavailable, it falls back to deterministic transformations; this fallback is intentionally limited and may make only formatting or simple constant-folding changes.

The original file is never overwritten. Validation checks syntax/compilation, not full behavioral equivalence. Run program tests and review the output before relying on it.

## Desktop app for user-provided code

Run `py .\adaptive_app.py` on Windows. Paste code into the editor or open a source file, click **Improve Code**, review the candidate and validation report, then save the improved result. The app detects C, C++, Java, or Python automatically; example files are not used as input.
