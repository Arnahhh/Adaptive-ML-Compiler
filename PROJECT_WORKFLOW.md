# Project workflow and module map

## End-to-end user path

```text
User pastes code / opens a source file
              |
              v
Language detection (C / C++ / Java / Python)
              |
              v
Static profile + conservative deterministic transformations
              |
              +------ Ollama available? ------ yes ---> ask local coding model for an automatic candidate
              |                                         |
              |                                         v
              |                              check declared interfaces + native validation
              |                                         |
              +-----------------------------------------+
                                |
                                v
                    choose safe valid candidate
                                |
                                v
                    UI shows code and report
                                |
                    +-----------+------------+
                    |                        |
                    v                        v
             save improved code      compile/run improved version
                                             |
                                             v
                                  preserve exe/class/source/logs
                                             |
                                             v
                              optional original-vs-improved run
```

## Top-level Python modules

- `adaptive_app.py`: Tkinter desktop UI. Manages source editor, improved source pane, report, program stdin, build/run tab, comparison, save/open operations, and original core build action.
- `adaptive_cli.py`: language detection, source profiling, deterministic transformations, local Ollama connection, candidate validation, interface symbol guard, change report, and command-line workflow.
- `adaptive_runtime.py`: native compile/run for C and C++, Java compile/run, Python syntax check/run, time limits, and persistent logs/artifacts.
- `test_adaptive_cli.py`: transformation, detection, interface-guard, and report regression tests.
- `test_adaptive_runtime.py`: tests execution/log persistence and language toolchain integration.

## Original academic compiler core (C files)

- `main.c`, `frontend.c`, `lexer.c`, `parser.c`, `ast.c`, `semantic.c`, `symbol.c`, `error.c`: source handling, lexical/syntax parsing, AST, semantic checks, symbols, and error reporting.
- `ir.c`, `common_ir.c`, `common_ir_lower.c`, `common_ir_codegen.c`: intermediate representations and lowering/code generation components.
- `optimizer.c`, `common_ir_optimizer.c`, `repair.c`: optimization and repair logic.
- `cfg.c`, `common_ir_cfg_optimizer.c`, `dataflow.c`: control-flow and data-flow analyses/optimizations.
- `ml.c`, `ml/model.csv`, `adaptive.c`: KNN-style optimization-phase prediction and adaptive phase selection.
- `codegen.c` and headers: output generation and shared module interfaces.

The C core remains a research/teaching prototype with supported language subsets. The Python workflow uses native tools (GCC, G++, javac, or Python's syntax compiler/interpreter) for language validation and execution.
