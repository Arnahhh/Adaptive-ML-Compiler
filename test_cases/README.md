# User Test Cases — Adaptive ML Compiler

These are user-authored sample programs for testing the desktop workflow. They are not used as automatic defaults: the app must start blank, and you open a case only when choosing to test it.

## GUI test procedure

For each valid case (01–08):

1. Open the app with `py .\adaptive_app.py` from the project root.
2. Click **Open Source File** and select the case file.
3. Click **Improve Code**. Confirm the detected language and inspect the report and the improved source.
4. For the constant-folding cases (01 and 03, and usually 07), the deterministic pipeline is expected to fold the isolated `20 + 22` expression to `42` in the source. Other cases may only receive cleanup or an AI suggestion; do not claim an optimization unless it appears in the report.
5. Click **Compile & Run Improved Code**. Open the **Compile & Run Output** tab and check the expected output below.
6. Click **Compare Original vs Improved**. For these cases, stdout and exit code should match for the supplied input.

## Valid cases and expected results

| File | Language / feature | Input in the app's Program input box | Expected program output |
|---|---|---|---|
| `01_python_constant_fold.py` | Python / constant folding | *(leave blank)* | `42` |
| `02_python_stdin_sum.py` | Python / stdin | `17` then `25` on the next line | `42` |
| `03_c_constant_fold.c` | C / constant folding | *(leave blank)* | `42` |
| `04_c_stdin_sum.c` | C / stdin | `17 25` | `42` |
| `05_cpp_loop_sum.cpp` | C++ / loop | *(leave blank)* | `15` |
| `06_cpp_stdin_sum.cpp` | C++ / stdin | `17 25` | `42` |
| `07_java_constant_fold.java` | Java / constant folding | *(leave blank)* | `42` |
| `08_java_stdin_sum.java` | Java / stdin | `17` then `25` on the next line | `42` |

For input cases, select the **Compile & Run Output** tab, enter the input exactly as shown in the box above, and then click **Compile & Run Improved Code**. You can enter input before or after clicking Improve Code.

## Negative validation cases

- `09_python_invalid_syntax.py`
- `10_c_invalid_syntax.c`

Open each one and click **Improve Code**. The app should not crash. Depending on installed tools and whether Ollama repairs the code, the invalid candidate should either be rejected/rolled back or a repaired, valid candidate may be presented. If a candidate is generated, compile/run it and inspect the validation report. Do not expect a fixed wording because compiler diagnostic messages vary by platform.

## Automated smoke test

From the project root, run:

```powershell
py .\test_cases\run_test_cases.py
```

It runs the deterministic improvement workflow (without Ollama), executes the eight valid programs, checks their expected stdout and reports unavailable compilers as skips. A failing case exits non-zero. It writes run artifacts under `outputs\test_cases`.

Also run the project's unit tests:

```powershell
py -m unittest -v
```

## Pass criteria for a demo

- All eight valid programs are detected correctly.
- Every available-language test compiles/runs successfully and returns the listed output.
- The constant folding cases show the `20 + 22` expression changed to `42` in the improved source/report when deterministic rules are used.
- The original file stays unchanged.
- The comparison report says stdout/exit codes match for the supplied test input.
- Invalid syntax tests are handled with a report rather than a crashed interface.

The smoke tests validate representative inputs only; they do not prove equivalence for all inputs or guarantee code runs faster.
