from __future__ import annotations

import sys
from pathlib import Path
from tempfile import TemporaryDirectory

PROJECT_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from adaptive_cli import improve_source
from adaptive_runtime import compile_and_run

CASES = [
    ("01_python_constant_fold.py", "Python", "", "42"),
    ("02_python_stdin_sum.py", "Python", "17\n25\n", "42"),
    ("03_c_constant_fold.c", "C", "", "42"),
    ("04_c_stdin_sum.c", "C", "17 25\n", "42"),
    ("05_cpp_loop_sum.cpp", "C++", "", "15"),
    ("06_cpp_stdin_sum.cpp", "C++", "17 25\n", "42"),
    ("07_java_constant_fold.java", "Java", "", "42"),
    ("08_java_stdin_sum.java", "Java", "17\n25\n", "42"),
]


def main() -> int:
    passed = 0
    failed = 0
    skipped = 0
    print("Adaptive ML Compiler — user test-case smoke test")
    print("AI is disabled in this test so the results are repeatable.\n")
    with TemporaryDirectory(prefix="amlc_user_cases_") as output_dir:
        for filename, expected_language, stdin, expected_stdout in CASES:
            path = PROJECT_ROOT / "test_cases" / filename
            source = path.read_text(encoding="utf-8")
            try:
                improved = improve_source(source, filename, allow_ai=False)
            except Exception as exc:
                print(f"FAIL  {filename}: improvement raised {type(exc).__name__}: {exc}")
                failed += 1
                continue
            if improved["language"] != expected_language:
                print(f"FAIL  {filename}: detected {improved['language']}, expected {expected_language}")
                failed += 1
                continue
            result = compile_and_run(
                improved["code"], expected_language,
                Path(filename).stem + "_smoke", output_dir, stdin=stdin,
            )
            if result.get("compile_status") == "UNAVAILABLE":
                print(f"SKIP  {filename}: {result.get('error', 'required compiler unavailable')}")
                skipped += 1
                continue
            actual = (result.get("stdout") or "").strip()
            okay = result.get("run_status") == "SUCCESS" and actual == expected_stdout and result.get("run_returncode") == 0
            if okay:
                print(f"PASS  {filename}: output={actual!r}")
                passed += 1
            else:
                print(
                    f"FAIL  {filename}: compile={result.get('compile_status')}, "
                    f"run={result.get('run_status')}, output={actual!r}, "
                    f"expected={expected_stdout!r}, error={result.get('error')}"
                )
                if result.get("compile_stderr"):
                    print(result["compile_stderr"].rstrip())
                if result.get("stderr"):
                    print(result["stderr"].rstrip())
                failed += 1
    print(f"\nSummary: {passed} passed, {failed} failed, {skipped} skipped.")
    if failed:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
