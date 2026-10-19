# Teacher Demo Guide — End-to-End Walkthrough

## What each panel means

- **Your Original Source**: the user's original text. It is not edited by the app.
- **Improved Source Code**: the candidate source after conservative transforms and, when available, local Ollama suggestions. This is still code, not program output.
- **Validation & Improvement Report**: language detection, static profile, transformations, validation, AI status, and original compiler-core analysis where the input is within its supported subset.
- **Compile & Run Output**: actual stdout/stderr, compiler diagnostics, exit status, and paths to saved artifacts.

Constant folding may legitimately turn a literal expression such as `20 + 22` into `42` in the improved *source*. That is a valid optimization but a poor demonstration of runtime output. For a clear demo, use a program whose result depends on input or data and enter the test input in the program-input box. The output should appear only in the **Compile & Run Output** tab as the program executes.

## Before class

1. Open the full project folder in VS Code.
2. Run ` .\build.bat ` in PowerShell, or click **Build Original Compiler Core** in the desktop app.
3. Check the local AI model with `ollama list`. The AI stage is optional; if no model is available, the deterministic fallback runs but performs fewer types of changes.
4. Run `py -m unittest -v` and `py .\test_cases\run_test_cases.py` from the project root.

## Live demo sequence

1. Open the desktop app using `py .\adaptive_app.py`.
2. Paste a small program you wrote or open your own source file; don't start with the constant-folding toy expression.
3. Click **Improve Code**. Point to the separate original and improved source panels and the report. Explain that validation alone does not prove behavioral equivalence.
4. Click the **Compile & Run Output** tab and enter the test input in the program-input box.
5. Click **Compile & Run Improved Code**. The app selects the output tab and shows the real program output, diagnostics, exit code, and artifact folder.
6. Click **Compare Original vs Improved** with the same input. Show whether stdout, stderr, and exit status match for that test input. This is a test case, not a proof for every possible input.
7. To demonstrate the original academic compiler core in more detail, run ` .\amlc .\examples\adaptive_loop.c ` after building it. This supported example reports IR, KNN phase prediction, CFG, data-flow analysis and code generation.

## What to say about scope

The desktop workflow accepts C, C++, Java and Python, with syntax/compile/run support dependent on installed toolchains. The retained C academic compiler core supports a narrower subset of constructs; it does not parse every valid program in each language. State this clearly if the core analysis reports that a construct is unsupported. The Ollama stage is also optional, and syntax validity is not the same as proof of behavior preservation or speedup.
