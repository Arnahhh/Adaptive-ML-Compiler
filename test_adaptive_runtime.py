import shutil
import tempfile
import unittest
from pathlib import Path

from adaptive_runtime import compile_and_run


class AdaptiveRuntimeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="amlc_runtime_test_")
        self.output_root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def test_python_run_and_persist_transcript(self):
        result = compile_and_run("name = input()\nprint('Hello, ' + name)\n", "Python", "greeting", self.output_root, "Arna\n")
        self.assertEqual(result["run_returncode"], 0)
        self.assertEqual(result["stdout"].strip(), "Hello, Arna")
        self.assertTrue((Path(result["artifact_dir"]) / "run_output.txt").is_file())
        self.assertIsNone(result["executable_path"])

    @unittest.skipUnless(shutil.which("gcc"), "gcc is required for C run tests")
    def test_c_compiles_runs_and_keeps_executable(self):
        source = '#include <stdio.h>\nint main(void) { printf("C_OK\\n"); return 0; }\n'
        result = compile_and_run(source, "C", "sample_c", self.output_root)
        self.assertEqual(result["compile_status"], "SUCCESS")
        self.assertEqual(result["stdout"].strip(), "C_OK")
        self.assertTrue(Path(result["executable_path"]).is_file())

    @unittest.skipUnless(shutil.which("g++"), "g++ is required for C++ run tests")
    def test_cpp_compiles_and_runs(self):
        source = '#include <iostream>\nint main() { std::cout << "CPP_OK" << std::endl; return 0; }\n'
        result = compile_and_run(source, "C++", "sample_cpp", self.output_root)
        self.assertEqual(result["compile_status"], "SUCCESS")
        self.assertEqual(result["stdout"].strip(), "CPP_OK")

    @unittest.skipUnless(shutil.which("javac") and shutil.which("java"), "JDK is required for Java run tests")
    def test_java_compiles_and_runs(self):
        source = 'public class Main { public static void main(String[] args) { System.out.println("JAVA_OK"); } }\n'
        result = compile_and_run(source, "Java", "Main", self.output_root)
        self.assertEqual(result["compile_status"], "SUCCESS")
        self.assertEqual(result["stdout"].strip(), "JAVA_OK")
        self.assertTrue((Path(result["artifact_dir"]) / "Main.class").is_file())

    def test_bad_python_is_not_run(self):
        result = compile_and_run("print(\n", "Python", "bad_python", self.output_root)
        self.assertIn("FAILED", result["compile_status"])
        self.assertIn("SKIPPED", result["run_status"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
