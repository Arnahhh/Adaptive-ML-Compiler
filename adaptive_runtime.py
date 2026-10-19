"""Compile and run source code while preserving build artifacts and transcripts."""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

EXTENSIONS = {"C": ".c", "C++": ".cpp", "Java": ".java", "Python": ".py"}


def java_main_class(source: str, fallback: str = "Main") -> str:
    """Return the package-qualified top-level Java class likely to contain main()."""
    package_match = re.search(r"(?m)^\s*package\s+([A-Za-z_$][\w$]*(?:\.[A-Za-z_$][\w$]*)*)\s*;", source)
    package = package_match.group(1) if package_match else ""
    declarations = list(re.finditer(r"(?m)^\s*(?:(?:public|abstract|final|strictfp)\s+)*class\s+([A-Za-z_$][\w$]*)", source))
    simple = fallback
    public_match = re.search(r"(?m)^\s*public\s+(?:(?:abstract|final|strictfp)\s+)*class\s+([A-Za-z_$][\w$]*)", source)
    if public_match:
        simple = public_match.group(1)
    elif declarations:
        simple = declarations[0].group(1)
    return f"{package}.{simple}" if package else simple


def _safe_name(value: str) -> str:
    value = re.sub(r"[^A-Za-z0-9_.-]+", "_", value).strip("._")
    return value or "user_code"


def _run(command: list[str], cwd: Path, stdin: str = "", timeout: int = 30) -> dict[str, Any]:
    try:
        result = subprocess.run(
            command,
            cwd=str(cwd),
            input=stdin,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
            check=False,
        )
        return {
            "returncode": result.returncode,
            "stdout": result.stdout or "",
            "stderr": result.stderr or "",
            "timed_out": False,
            "command": command,
        }
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout or ""
        stderr = exc.stderr or ""
        if isinstance(stdout, bytes):
            stdout = stdout.decode("utf-8", errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode("utf-8", errors="replace")
        return {
            "returncode": None,
            "stdout": stdout,
            "stderr": stderr,
            "timed_out": True,
            "command": command,
        }


def compile_and_run(
    source: str,
    language: str,
    stem: str,
    output_root: str | Path,
    stdin: str = "",
    compile_timeout: int = 45,
    run_timeout: int = 10,
) -> dict[str, Any]:
    """Build and execute one source revision in a unique persistent run folder.

    The output folder is intentionally not temporary: native executables and Java
    class files remain available for inspection after this function returns.
    """
    if language not in EXTENSIONS:
        raise ValueError(f"Unsupported language: {language}")
    clean_stem = _safe_name(stem)
    root = Path(output_root).expanduser().resolve()
    run_dir = root / clean_stem / datetime.now().strftime("run_%Y%m%d_%H%M%S_%f")
    run_dir.mkdir(parents=True, exist_ok=False)
    source_name = java_main_class(source, clean_stem).split(".")[-1] if language == "Java" else clean_stem
    source_path = run_dir / (source_name + EXTENSIONS[language])
    source_path.write_text(source, encoding="utf-8", newline="")

    data: dict[str, Any] = {
        "language": language,
        "artifact_dir": str(run_dir),
        "source_path": str(source_path),
        "stdin": stdin,
        "compiler": None,
        "compile_status": "NOT REQUIRED",
        "compile_returncode": None,
        "compile_stdout": "",
        "compile_stderr": "",
        "run_status": "NOT RUN",
        "run_returncode": None,
        "stdout": "",
        "stderr": "",
        "executable_path": None,
        "class_path": None,
        "timed_out": False,
        "error": None,
    }

    if language in ("C", "C++"):
        compiler_name = "gcc" if language == "C" else "g++"
        compiler = shutil.which(compiler_name)
        if not compiler:
            data["compile_status"] = "UNAVAILABLE"
            data["error"] = (
                f"{compiler_name} was not found on PATH. Install a Windows GCC toolchain "
                "such as MinGW-w64 and restart VS Code."
            )
            _write_transcripts(data, run_dir)
            return data
        executable = run_dir / (clean_stem + (".exe" if os.name == "nt" else ""))
        command = [compiler, "-std=c11" if language == "C" else "-std=c++17", "-O2", "-Wall", "-Wextra", str(source_path), "-o", str(executable)]
        if language == "C":
            command.append("-lm")
        compiled = _run(command, run_dir, timeout=compile_timeout)
        data.update({
            "compiler": compiler_name,
            "compile_status": "TIMED OUT" if compiled["timed_out"] else ("SUCCESS" if compiled["returncode"] == 0 else "FAILED"),
            "compile_returncode": compiled["returncode"],
            "compile_stdout": compiled["stdout"],
            "compile_stderr": compiled["stderr"],
            "compile_command": compiled["command"],
        })
        if compiled["timed_out"] or compiled["returncode"] != 0:
            data["run_status"] = "SKIPPED — compilation did not succeed"
            data["timed_out"] = compiled["timed_out"]
            data["error"] = "Compilation timed out." if compiled["timed_out"] else "Compilation failed."
            _write_transcripts(data, run_dir)
            return data
        data["executable_path"] = str(executable)
        run_command = [str(executable)]

    elif language == "Java":
        javac = shutil.which("javac")
        java = shutil.which("java")
        if not javac or not java:
            data["compile_status"] = "UNAVAILABLE"
            data["error"] = "Both javac and java must be on PATH. Install a JDK and restart VS Code."
            _write_transcripts(data, run_dir)
            return data
        compile_command = [javac, "-encoding", "UTF-8", "-Xlint:none", "-d", str(run_dir), str(source_path)]
        compiled = _run(compile_command, run_dir, timeout=compile_timeout)
        main_class = java_main_class(source, clean_stem)
        data.update({
            "compiler": "javac",
            "compile_status": "TIMED OUT" if compiled["timed_out"] else ("SUCCESS" if compiled["returncode"] == 0 else "FAILED"),
            "compile_returncode": compiled["returncode"],
            "compile_stdout": compiled["stdout"],
            "compile_stderr": compiled["stderr"],
            "compile_command": compiled["command"],
            "main_class": main_class,
        })
        if compiled["timed_out"] or compiled["returncode"] != 0:
            data["run_status"] = "SKIPPED — compilation did not succeed"
            data["timed_out"] = compiled["timed_out"]
            data["error"] = "Java compilation timed out." if compiled["timed_out"] else "Java compilation failed."
            _write_transcripts(data, run_dir)
            return data
        data["class_path"] = str(run_dir)
        run_command = [java, "-cp", str(run_dir), main_class]

    else:
        data["compile_status"] = "INTERPRETED — no native executable is produced"
        try:
            compile(source, str(source_path), "exec")
        except SyntaxError as exc:
            data["compile_status"] = "FAILED — Python syntax error"
            data["error"] = f"Line {exc.lineno}: {exc.msg}"
            data["run_status"] = "SKIPPED — syntax check did not succeed"
            _write_transcripts(data, run_dir)
            return data
        data["compiler"] = sys.executable
        run_command = [sys.executable, str(source_path)]

    run = _run(run_command, run_dir, stdin=stdin, timeout=run_timeout)
    data.update({
        "run_status": "TIMED OUT" if run["timed_out"] else ("SUCCESS" if run["returncode"] == 0 else "NONZERO EXIT"),
        "run_returncode": run["returncode"],
        "stdout": run["stdout"],
        "stderr": run["stderr"],
        "run_command": run["command"],
        "timed_out": run["timed_out"],
    })
    if run["timed_out"]:
        data["error"] = f"Execution stopped after the {run_timeout}-second time limit."
    _write_transcripts(data, run_dir)
    return data


def _write_transcripts(data: dict[str, Any], run_dir: Path) -> None:
    lines = [
        "ADAPTIVE ML COMPILER — BUILD AND RUN REPORT",
        f"Language: {data.get('language')}",
        f"Source: {data.get('source_path')}",
        f"Artifacts folder: {data.get('artifact_dir')}",
        f"Compiler: {data.get('compiler') or 'Unavailable'}",
        f"Compilation: {data.get('compile_status')}",
    ]
    if data.get("compile_command"):
        lines.append("Compile command: " + subprocess.list2cmdline(data["compile_command"]))
    if data.get("compile_stdout", "").strip():
        lines.extend(["", "Compiler stdout:", data["compile_stdout"].rstrip()])
    if data.get("compile_stderr", "").strip():
        lines.extend(["", "Compiler diagnostics:", data["compile_stderr"].rstrip()])
    lines.extend(["", f"Execution: {data.get('run_status')}"])
    if data.get("run_command"):
        lines.append("Run command: " + subprocess.list2cmdline(data["run_command"]))
    lines.extend(["", "Program stdout:", data.get("stdout", "").rstrip() or "(no standard output)"])
    if data.get("stderr", "").strip():
        lines.extend(["", "Program stderr:", data["stderr"].rstrip()])
    if data.get("run_returncode") is not None:
        lines.append(f"Program exit code: {data['run_returncode']}")
    if data.get("error"):
        lines.extend(["", "Status detail:", data["error"]])
    transcript = "\n".join(lines) + "\n"
    (run_dir / "run_output.txt").write_text(transcript, encoding="utf-8")
    (run_dir / "compile.log").write_text(
        "\n".join([
            f"Compilation: {data.get('compile_status')}",
            data.get("compile_stdout", "").rstrip(),
            data.get("compile_stderr", "").rstrip(),
        ]) + "\n",
        encoding="utf-8",
    )
