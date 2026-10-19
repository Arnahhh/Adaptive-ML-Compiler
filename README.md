# Adaptive ML Compiler

**Adaptive ML Compiler with Intelligent Phase Prediction and Automatic Code Improvement**

A complete academic project combining an original C-based compiler/optimizer core with a desktop application that accepts user-provided code in C, C++, Java, or Python.

## Quick start (Windows)

1. Extract the entire ZIP.
2. Open the extracted folder in VS Code.
3. In VS Code terminal run:

```powershell
py .\adaptive_app.py
```

Or double-click `launch_app.bat`.

The app starts with a blank editor. Paste your code or click **Open Source File**. Select **Improve Code** to get a candidate and report; choose **Compile & Run Improved Code** to see build diagnostics and program output; choose **Compare Original vs Improved** to compare both versions with the same supplied stdin. Save the candidate or report when finished.

For complete setup, architecture, and output paths, read [`START_HERE.md`](START_HERE.md) and [`PROJECT_WORKFLOW.md`](PROJECT_WORKFLOW.md).

## Included

- Desktop UI with source editor, improved code editor, validation/improvement report, program stdin, compile/run output, diff-based comparison, save/open controls, output-folder access, and core build command.
- Automatic language detection for `.c`, `.cpp`, `.cc`, `.cxx`, `.java`, `.py` and paste-in source in those languages.
- Deterministic source cleanup and conservative simple constant-folding rules.
- Optional local Ollama coding-model candidate generation with native validation and declared-interface checks.
- C/C++ compile and execution, Java compilation and execution, Python syntax checking and execution, stdin support, captured stdout/stderr/exit codes, timeouts, saved artifacts and logs.
- Original C compiler source: lexer/parser, AST, semantics, IR, optimizer and repair, CFG/data-flow analyses, codegen, KNN-style phase prediction and model dataset.
- Automated unit tests, original sample programs, Windows and Linux build scripts, audit and demo guides.

## Original C core build

Windows with GCC installed:

```powershell
.\build.bat
```

Linux/WSL with GCC:

```bash
./build.sh
```

The final ZIP intentionally omits prebuilt compiler binaries because binaries are operating-system-specific. Build locally on your target OS.

## Tests

```powershell
py -m unittest -v
```

The unit suite covers deterministic transforms, language detection, validation and end-to-end compile/run tests where a native toolchain is available.

## Ollama (optional)

The application tries the local Ollama API automatically and falls back to deterministic changes when it is unavailable. It prefers `qwen2.5-coder:7b`; install/pull a coding model in Ollama if you want the AI-assisted step. Environment variables `AMLC_OLLAMA_MODEL` and `AMLC_OLLAMA_URL` can set a different installed model/endpoint. No API key is required.

## Important scope and safety notes

This is a working academic prototype, not a production compiler certified for every valid program in all four languages. The built-in C frontend and ML dataset cover limited research/demo cases. Native compiler validation does not prove semantic equivalence; code output can change behavior and is not guaranteed faster. Use the comparison tool and your own tests. Compile/run executes code locally with the current user's permissions and is not a security sandbox—run only trusted source.

## Reading the desktop app correctly

The right-hand **Improved Source Code** pane is source text, not runtime stdout. Optimizations such as constant folding can change a source expression to a constant in this pane. The program's actual printed result is shown in the bottom **Compile & Run Output** tab only after execution. Use **Compare Original vs Improved** with the same program input to check observable behavior for that test.

