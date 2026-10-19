"""Desktop UI for the Adaptive ML Compiler's automatic code-improvement workflow."""
from __future__ import annotations

import os
import queue
import re
import subprocess
import shutil
import sys
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from tkinter.scrolledtext import ScrolledText

from adaptive_cli import SUPPORTED, detect_language, improve_source
from adaptive_runtime import compile_and_run

ROOT = Path(__file__).resolve().parent
OUTPUT_ROOT = ROOT / "outputs"


class AdaptiveApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Adaptive ML Compiler — Automatic Code Improvement")
        self.root.geometry("1320x850")
        self.root.minsize(980, 680)
        self.source_path: Path | None = None
        self.output_code = ""
        self.output_language: str | None = None
        self.output_suffix = ".py"
        self.output_stem = "user_code"
        self.source_snapshot = ""
        self.report_text = ""
        self.busy = False
        self.events: queue.Queue[tuple[str, object]] = queue.Queue()
        self.buttons: list[ttk.Button] = []

        outer = ttk.Frame(root, padding=10)
        outer.pack(fill="both", expand=True)
        ttk.Label(outer, text="Adaptive ML Compiler", font=("Segoe UI", 18, "bold")).pack(anchor="w")
        ttk.Label(
            outer,
            text="Paste/open your own C, C++, Java, or Python source. LEFT = original source; RIGHT = improved source; BOTTOM = report or actual program output.",
        ).pack(anchor="w", pady=(2, 8))

        toolbar = ttk.Frame(outer)
        toolbar.pack(fill="x", pady=(0, 4))
        self._button(toolbar, "Open Source File", self.open_file)
        self._button(toolbar, "New / Clear", self.clear_all, padx=(7, 0))
        self._button(toolbar, "Improve Code", self.improve, padx=(7, 0))
        self._button(toolbar, "Compile & Run Improved Code", self.compile_and_run, padx=(7, 0))
        self._button(toolbar, "Compare Original vs Improved", self.compare, padx=(7, 0))

        toolbar_tools = ttk.Frame(outer)
        toolbar_tools.pack(fill="x", pady=(0, 7))
        self._button(toolbar_tools, "Save Improved Code", self.save_output)
        self._button(toolbar_tools, "Save Report", self.save_report, padx=(7, 0))
        self._button(toolbar_tools, "Open Output Folder", self.open_output_folder, padx=(7, 0))
        self._button(toolbar_tools, "Build Original Compiler Core", self.build_core, padx=(7, 0))

        self.status = tk.StringVar(value="Ready — type code or open a source file.")
        ttk.Label(outer, textvariable=self.status).pack(anchor="w", pady=(0, 8))

        panes = ttk.Panedwindow(outer, orient="horizontal")
        panes.pack(fill="both", expand=True)
        left = ttk.Frame(panes, padding=(0, 0, 5, 0))
        right = ttk.Frame(panes, padding=(5, 0, 0, 0))
        panes.add(left, weight=1)
        panes.add(right, weight=1)

        ttk.Label(left, text="YOUR ORIGINAL SOURCE (preserved)", font=("Segoe UI", 10, "bold")).pack(anchor="w", pady=(0, 4))
        self.input_text = ScrolledText(left, wrap="none", undo=True, font=("Consolas", 10), tabs=(4,))
        self.input_text.pack(fill="both", expand=True)
        self.input_text.bind("<<Modified>>", self._on_source_modified)
        self.input_text.edit_modified(False)

        ttk.Label(right, text="IMPROVED SOURCE CODE (not runtime output)", font=("Segoe UI", 10, "bold")).pack(anchor="w", pady=(0, 4))
        self.output_text = ScrolledText(right, wrap="none", font=("Consolas", 10), tabs=(4,), state="disabled")
        self.output_text.pack(fill="both", expand=True)

        self.result_tabs = ttk.Notebook(outer, height=190)
        self.result_tabs.pack(fill="x", expand=False, pady=(8, 0))
        report_tab = ttk.Frame(self.result_tabs, padding=5)
        self.result_tabs.add(report_tab, text="Validation & Improvement Report")
        self.report = ScrolledText(report_tab, height=7, wrap="word", font=("Consolas", 9), state="disabled")
        self.report.pack(fill="both", expand=True)

        execution_tab = ttk.Frame(self.result_tabs, padding=5)
        self.result_tabs.add(execution_tab, text="Compile & Run Output")
        ttk.Label(execution_tab, text="Program input (optional; one value per line, passed to stdin):").pack(anchor="w")
        self.program_input = tk.Text(execution_tab, height=2, wrap="word", font=("Consolas", 9))
        self.program_input.pack(fill="x", pady=(2, 5))
        self.execution_output = ScrolledText(execution_tab, height=6, wrap="word", font=("Consolas", 9), state="disabled")
        self.execution_output.pack(fill="both", expand=True)

        ttk.Label(outer, text="Safety: Compile & Run executes your source code locally. Run only code you trust.", foreground="#8a3b12").pack(anchor="w", pady=(4, 0))
        footer = ttk.Label(outer, text=f"Saved build/run artifacts and logs: {OUTPUT_ROOT}", foreground="#555555")
        footer.pack(anchor="w", pady=(5, 0))
        self.root.after(100, self._poll_events)

    def _button(self, parent: ttk.Frame, label: str, command, padx=(0, 0)) -> None:
        button = ttk.Button(parent, text=label, command=command)
        button.pack(side="left", padx=padx)
        self.buttons.append(button)

    def _on_source_modified(self, _event=None) -> None:
        if self.input_text.edit_modified():
            self.input_text.edit_modified(False)

    def _set_text(self, widget: ScrolledText, value: str) -> None:
        widget.configure(state="normal")
        widget.delete("1.0", "end")
        widget.insert("1.0", value)
        widget.configure(state="disabled")

    def _current_source(self) -> str:
        return self.input_text.get("1.0", "end-1c")

    def _launch_worker(self, action: str, work) -> None:
        if self.busy:
            return
        self.busy = True
        for button in self.buttons:
            button.configure(state="disabled")
        self.status.set(action)

        def runner():
            try:
                result = work()
            except Exception as exc:
                result = {"_worker_error": f"{exc.__class__.__name__}: {exc}"}
            self.events.put((action, result))

        threading.Thread(target=runner, daemon=True).start()

    def _poll_events(self) -> None:
        try:
            action, result = self.events.get_nowait()
        except queue.Empty:
            self.root.after(100, self._poll_events)
            return
        self.busy = False
        for button in self.buttons:
            button.configure(state="normal")
        if isinstance(result, dict) and result.get("_worker_error"):
            messagebox.showerror("Task failed", result["_worker_error"])
            self.status.set("Task failed — see the error message.")
        elif action == "Improving code…":
            self._finish_improve(result)
        elif action == "Compiling and running improved code…":
            self._finish_run(result)
        elif action == "Comparing original and improved code…":
            self._finish_compare(result)
        elif action == "Building original compiler core…":
            self._finish_build(result)
        self.root.after(100, self._poll_events)

    def clear_all(self) -> None:
        self.source_path = None
        self.source_snapshot = ""
        self.output_code = ""
        self.output_language = None
        self.output_stem = "user_code"
        self.output_suffix = ".py"
        self.report_text = ""
        self.input_text.delete("1.0", "end")
        self._set_text(self.output_text, "")
        self._set_text(self.report, "")
        self._set_text(self.execution_output, "")
        self.status.set("Cleared — paste a new program or open a source file.")

    def open_file(self) -> None:
        filename = filedialog.askopenfilename(
            title="Open source code",
            filetypes=[("Supported source files", "*.c *.cpp *.cc *.cxx *.py *.java"), ("All files", "*.*")],
        )
        if not filename:
            return
        path = Path(filename)
        try:
            source = path.read_text(encoding="utf-8")
        except (OSError, UnicodeError) as exc:
            messagebox.showerror("Could not open file", str(exc))
            return
        self.source_path = path
        self.input_text.delete("1.0", "end")
        self.input_text.insert("1.0", source)
        self.input_text.edit_modified(False)
        self.status.set(f"Loaded {path.name}. Click Improve Code to start the pipeline.")

    def improve(self) -> None:
        source = self._current_source()
        if not source.strip():
            messagebox.showinfo("No source code", "Type or paste your own program, or click Open Source File.")
            return
        input_name = self.source_path.name if self.source_path else ""
        language = detect_language(source, input_name)
        if language is None:
            messagebox.showerror("Language not detected", "Could not identify C, C++, Java, or Python. Open a supported file or paste a complete program.")
            return
        stem = self.source_path.stem if self.source_path else "user_code"
        if language == "Java":
            result_match = re.search(r"(?m)^\s*public\s+(?:(?:abstract|final|strictfp)\s+)*class\s+([A-Za-z_$][\w$]*)", source)
            if not result_match:
                result_match = re.search(r"\bclass\s+([A-Za-z_$][\w$]*)", source)
            if result_match:
                stem = result_match.group(1)
        filename = input_name or (stem + next(ext for ext, lang in SUPPORTED.items() if lang == language))
        self.source_snapshot = source

        def work():
            result = improve_source(source, filename, allow_ai=True)
            result["input_name"] = input_name or "Pasted source code"
            result["engine_filename"] = filename
            result["stem"] = stem if language != "Java" else result["stem"]
            return result
        self._launch_worker("Improving code…", work)

    def _finish_improve(self, result: object) -> None:
        if not isinstance(result, dict):
            return
        self.output_code = result["code"]
        self.output_language = result["language"]
        self.output_suffix = result["suffix"]
        self.output_stem = result["stem"]
        self._set_text(self.output_text, self.output_code)
        report = result["report"]
        engine_filename = result.get("engine_filename", "")
        displayed_input = result.get("input_name", "Pasted source code")
        if engine_filename and engine_filename != displayed_input:
            report = report.replace(engine_filename, displayed_input)
        report = report.replace("Input: " + str(Path(engine_filename).name), "Input: " + displayed_input)
        self.report_text = report
        self._set_text(self.report, report)
        self.result_tabs.select(0)
        self.status.set(f"Improvement complete — detected {self.output_language}. The right panel is SOURCE code, not the program's answer. Click Compile & Run Improved Code to see actual output in the bottom output tab. Original source remains unchanged.")

    def _assert_fresh_candidate(self) -> bool:
        if not self.output_code or not self.output_language:
            messagebox.showinfo("No improved code", "Click Improve Code first.")
            return False
        if self._current_source() != self.source_snapshot:
            messagebox.showinfo("Source changed", "You edited the original source after the last improvement. Click Improve Code again before running or comparing.")
            return False
        return True

    def compile_and_run(self) -> None:
        if not self._assert_fresh_candidate():
            return
        candidate = self.output_code
        language = self.output_language
        stem = self.output_stem
        program_input = self.program_input.get("1.0", "end-1c")

        def work():
            return compile_and_run(candidate, language, stem + "_improved", OUTPUT_ROOT, program_input)
        self._launch_worker("Compiling and running improved code…", work)

    def compare(self) -> None:
        if not self._assert_fresh_candidate():
            return
        original = self._current_source()
        candidate = self.output_code
        language = self.output_language
        stem = self.output_stem
        program_input = self.program_input.get("1.0", "end-1c")

        def work():
            before = compile_and_run(original, language, stem + "_original", OUTPUT_ROOT / "comparisons", program_input)
            after = compile_and_run(candidate, language, stem + "_improved", OUTPUT_ROOT / "comparisons", program_input)
            same_stdout = before.get("stdout", "") == after.get("stdout", "")
            same_exit = before.get("run_returncode") == after.get("run_returncode")
            same_stderr = before.get("stderr", "") == after.get("stderr", "")
            usable_statuses = {"SUCCESS", "INTERPRETED — no native executable is produced"}
            both_built = before.get("compile_status") in usable_statuses and after.get("compile_status") in usable_statuses
            same_observations = same_stdout and same_stderr and same_exit and both_built and before.get("run_status") == "SUCCESS" and after.get("run_status") == "SUCCESS"
            report = [
                "ORIGINAL VS IMPROVED — OBSERVABLE OUTPUT COMPARISON",
                "This test uses only the stdin supplied in the app and compares captured stdout, stderr, and exit code.",
                "It is not a proof of equivalence for all possible inputs or side effects.",
                "",
                "ORIGINAL PROGRAM",
                self._format_run_result(before),
                "",
                "IMPROVED PROGRAM",
                self._format_run_result(after),
                "",
                f"Same stdout: {'YES' if same_stdout else 'NO'}",
                f"Same stderr: {'YES' if same_stderr else 'NO'}",
                f"Same exit code: {'YES' if same_exit else 'NO'}",
                f"All compared observations match: {'YES' if same_observations else 'NO / INCOMPLETE'}",
                f"Original artifacts: {before.get('artifact_dir')}",
                f"Improved artifacts: {after.get('artifact_dir')}",
            ]
            text = "\n".join(report) + "\n"
            comparison_file = OUTPUT_ROOT / "comparisons" / (stem + "_comparison.txt")
            comparison_file.parent.mkdir(parents=True, exist_ok=True)
            comparison_file.write_text(text, encoding="utf-8")
            return {"text": text, "comparison_file": str(comparison_file), "same": same_observations}
        self._launch_worker("Comparing original and improved code…", work)

    def _format_run_result(self, data: dict) -> str:
        lines = [
            f"Compilation/build: {data.get('compile_status')}",
            f"Execution: {data.get('run_status')}",
            f"Source file: {data.get('source_path')}",
            f"Artifact folder: {data.get('artifact_dir')}",
        ]
        if data.get("executable_path"):
            lines.append("Compiled executable: " + data["executable_path"])
        if data.get("class_path"):
            lines.append("Java class files: " + data["class_path"])
        if data.get("compile_stderr", "").strip():
            lines.extend(["", "Compiler messages:", data["compile_stderr"].rstrip()])
        elif data.get("compile_stdout", "").strip():
            lines.extend(["", "Compiler output:", data["compile_stdout"].rstrip()])
        lines.extend(["", "PROGRAM OUTPUT:", data.get("stdout", "").rstrip() or "(no standard output)"])
        if data.get("stderr", "").strip():
            lines.extend(["", "Runtime errors / stderr:", data["stderr"].rstrip()])
        if data.get("run_returncode") is not None:
            lines.append(f"Program exit code: {data['run_returncode']}")
        if data.get("error"):
            lines.extend(["", "Details:", data["error"]])
        lines.extend(["", f"Transcript saved to: {Path(data['artifact_dir']) / 'run_output.txt'}"])
        return "\n".join(lines)

    def _finish_run(self, result: object) -> None:
        if not isinstance(result, dict):
            return
        self._set_text(self.execution_output, self._format_run_result(result))
        self.result_tabs.select(1)
        self.status.set(f"Build/run finished — build: {result.get('compile_status')}; execution: {result.get('run_status')}. Artifacts are saved in the outputs folder.")

    def _finish_compare(self, result: object) -> None:
        if not isinstance(result, dict):
            return
        self._set_text(self.execution_output, result["text"])
        self.result_tabs.select(1)
        self.status.set("Comparison finished — outputs match for the supplied input." if result["same"] else "Comparison finished — differences or incomplete compilation are shown in Compile & Run Output.")

    def save_output(self) -> None:
        if not self.output_code:
            messagebox.showinfo("No improved code", "Click Improve Code first.")
            return
        default_name = self.output_stem + "_improved" + self.output_suffix
        filename = filedialog.asksaveasfilename(
            title="Save improved source code", initialfile=default_name,
            defaultextension=self.output_suffix,
            filetypes=[("Source code", "*" + self.output_suffix), ("All files", "*.*")],
        )
        if not filename:
            return
        try:
            Path(filename).write_text(self.output_code, encoding="utf-8", newline="")
        except OSError as exc:
            messagebox.showerror("Could not save file", str(exc))
            return
        self.status.set(f"Improved source saved to {filename}")

    def save_report(self) -> None:
        text = self.report_text
        if not text:
            messagebox.showinfo("No report", "Improve code first to generate a report.")
            return
        filename = filedialog.asksaveasfilename(
            title="Save improvement report", initialfile=self.output_stem + "_improvement_report.txt",
            defaultextension=".txt", filetypes=[("Text report", "*.txt"), ("All files", "*.*")],
        )
        if not filename:
            return
        try:
            Path(filename).write_text(text, encoding="utf-8")
        except OSError as exc:
            messagebox.showerror("Could not save report", str(exc))
            return
        self.status.set(f"Report saved to {filename}")

    def build_core(self) -> None:
        source_files = [
            "ast.c", "cfg.c", "codegen.c", "common_ir.c", "common_ir_cfg_optimizer.c",
            "common_ir_codegen.c", "common_ir_lower.c", "common_ir_optimizer.c", "dataflow.c",
            "error.c", "frontend.c", "ir.c", "java_frontend.c", "language.c", "lexer.c",
            "main.c", "optimizer.c", "parser.c", "python_frontend.c", "repair.c",
            "semantic.c", "source.c", "symbol.c", "ml.c", "adaptive.c",
        ]
        compiler = shutil.which("gcc")
        if not compiler:
            messagebox.showerror("GCC not found", "The original compiler core needs GCC on PATH. Install a GCC toolchain (such as MinGW-w64 on Windows), reopen VS Code, and retry.")
            return
        binary = ROOT / ("amlc.exe" if os.name == "nt" else "amlc")

        def work():
            missing = [item for item in source_files if not (ROOT / item).is_file()]
            if missing:
                return {"success": False, "output": "Missing source modules: " + ", ".join(missing), "binary_path": str(binary)}
            command = [compiler, "-std=c11", "-Wall", "-Wextra", "-pedantic", "-O2", *[str(ROOT / item) for item in source_files], "-o", str(binary), "-lm"]
            try:
                result = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=90)
            except subprocess.TimeoutExpired:
                return {"success": False, "output": "Core build timed out after 90 seconds.", "binary_path": str(binary)}
            transcript = ["COMMAND: " + subprocess.list2cmdline(command), f"BUILD EXIT CODE: {result.returncode}"]
            if result.stdout.strip():
                transcript.extend(["", "Compiler output:", result.stdout.rstrip()])
            if result.stderr.strip():
                transcript.extend(["", "Compiler diagnostics:", result.stderr.rstrip()])
            if result.returncode == 0:
                transcript.extend(["", f"Original compiler core built successfully: {binary}"])
            else:
                transcript.extend(["", "Build failed. Fix the compiler diagnostics and try again."])
            return {"success": result.returncode == 0, "output": "\n".join(transcript), "binary_path": str(binary)}
        self._launch_worker("Building original compiler core…", work)

    def _finish_build(self, result: object) -> None:
        if not isinstance(result, dict):
            return
        self._set_text(self.execution_output, result.get("output", ""))
        self.result_tabs.select(1)
        if result.get("success"):
            self.status.set(f"Original compiler core built: {result.get('binary_path')}. Future improvement reports can include the KNN/IR/CFG/data-flow analysis.")
        else:
            self.status.set("Original compiler core build failed — see Compile & Run Output for diagnostics.")

    def open_output_folder(self) -> None:
        OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
        try:
            if os.name == "nt":
                os.startfile(str(OUTPUT_ROOT))
            elif sys.platform == "darwin":
                subprocess.Popen(["open", str(OUTPUT_ROOT)])
            else:
                subprocess.Popen(["xdg-open", str(OUTPUT_ROOT)])
        except OSError as exc:
            messagebox.showerror("Could not open output folder", f"Open this folder manually:\n{OUTPUT_ROOT}\n\n{exc}")


def main() -> int:
    if not (ROOT / "adaptive_cli.py").is_file() or not (ROOT / "adaptive_runtime.py").is_file():
        print("ERROR: adaptive_app.py, adaptive_cli.py, and adaptive_runtime.py must be together in the project folder.")
        return 1
    root = tk.Tk()
    AdaptiveApp(root)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
