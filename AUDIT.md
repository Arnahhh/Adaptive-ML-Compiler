# Baseline Audit and Extension Notes

## Baseline checked

- Preserved the original C compiler modules, KNN model CSV, examples, and test fixtures.
- Rebuilt the original source successfully with GCC in the audit environment.
- Ran `examples/adaptive_constant.c`; the compiler produced a constant-folding prediction, optimized IR, CFG/data-flow output, and `output.s`.
- Ran the existing Python and Java examples through the original compiler. Its internal front ends accept some language-specific subsets; this does not imply native-language validity.
- Inspected the bundled `amlc.exe`: it was an ELF Linux executable despite the `.exe` extension. It is therefore not a Windows executable and was excluded from the final archive; build the original compiler from source on the target OS.
- The original README's direct GCC command omitted `adaptive.c`; the maintained `build.bat` and `build.sh` include it.
- Initialized the ML prediction structure before the model call to avoid reading an uninitialized label if the CSV model cannot be loaded.
- Updated stale frontend comments that incorrectly described Python and Java front ends as future work.

## New terminal workflow

The CLI does not ask users to select an optimization goal. It infers potential improvements automatically.

`adaptive_cli.py` runs an automatic, no-menu code-improvement pipeline with source profiling, optional local Ollama generation, native validation, rollback on validation failure, non-destructive output files, and a per-run report. It supports `.c`, `.cpp`, `.cc`, `.cxx`, `.py`, and `.java` inputs.

## Validation performed in the build environment

- Original compiler source rebuilt with GCC.
- Four unit tests passed.
- Python syntax compilation passed.
- Native syntax/compile validation passed for the included C, C++, Python, and Java language demos.
- End-to-end terminal runs completed for all four language demos with local AI unavailable; generated adapted files passed their native validators.

## Known limitations

- The original internal compiler is a prototype with subset front ends, not a complete standards-compliant compiler for C, C++, Python, or Java.
- The deterministic transformation layer intentionally avoids speculative memory and semantic bug fixes.
- Ollama integration is optional and was not exercised against a running model during this audit. Its candidate output is accepted only if the native validator succeeds.
- Syntax/type checks do not establish behavioral equivalence, runtime performance improvements, or memory reductions. The tool does not claim benchmark results.
- Existing original fixtures include parser-specific or intentionally malformed examples; use the `preference_demo.*` files for the four-language terminal demo.
