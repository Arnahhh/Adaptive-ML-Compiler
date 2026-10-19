import importlib.util
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).with_name("adaptive_cli.py")
SPEC = importlib.util.spec_from_file_location("adaptive_cli", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class AdaptiveCliTests(unittest.TestCase):
    def test_detectable_features(self):
        result = MODULE.analyze("int main(){ int x = 2 + 3; if(x) x++; return x; }", "C")
        self.assertGreaterEqual(result["lines_of_code"], 1)
        self.assertGreaterEqual(result["branches_approx"], 1)

    def test_constant_folding_c(self):
        source = "int main() {\n    int x = 2 + 3;\n    return x;\n}\n"
        result, changes = MODULE.conservative_rules(source, "C")
        self.assertIn("int x = 5;", result)
        self.assertTrue(any("Folded" in item for item in changes))

    def test_python_print_constant_fold(self):
        source = "print(2 + 3)"
        result, changes = MODULE.conservative_rules(source, "Python")
        self.assertEqual(source, "print(2 + 3)")
        self.assertIn("print(5)", result)
        self.assertTrue(any("Folded" in item for item in changes))

    def test_constant_folding_does_not_change_operator_precedence(self):
        source = "x = 2 + 3 * 4\n"
        result, _ = MODULE.conservative_rules(source, "Python")
        self.assertEqual(result, source)

    def test_final_newline(self):
        result, _ = MODULE.conservative_rules("print('hello')", "Python")
        self.assertTrue(result.endswith("\n"))

    def test_language_detection_for_all_supported_languages(self):
        snippets = [
            ('#include <stdio.h>\nint main(){printf("hi");}', "C"),
            ('#include <iostream>\nint main(){std::cout << 5;}', "C++"),
            ('public class Main { public static void main(String[] a) { System.out.println(1); } }', "Java"),
            ('def main():\n    print(1)\n', "Python"),
        ]
        for source, expected in snippets:
            with self.subTest(expected=expected):
                self.assertEqual(MODULE.detect_language(source), expected)

    def test_io_operation_guard_detects_removed_python_input_or_output(self):
        before = MODULE.io_operations("name = input()\nprint(name)\n", "Python")
        after = MODULE.io_operations("name = 'Arna'\nprint(name)\n", "Python")
        self.assertTrue(before - after)
        self.assertTrue(any("input" in marker for marker in before - after))

    def test_interface_symbol_guard_detects_removed_python_function(self):
        before = MODULE.interface_symbols("def calculate(x):\n    return x\n", "Python")
        after = MODULE.interface_symbols("def helper(x):\n    return x\n", "Python")
        self.assertTrue(before - after)

    def test_no_ai_improvement_pipeline_returns_validation_and_report(self):
        result = MODULE.improve_source("print(2 + 3)\n", "demo.py", allow_ai=False)
        self.assertEqual(result["language"], "Python")
        self.assertIn("print(5)", result["code"])
        self.assertIn("Behavioral equivalence: NOT PROVEN", result["report"])
        self.assertIn("Output validation", result["report"])

    def test_python_report_does_not_call_subset_frontend_a_syntax_error(self):
        result = MODULE.improve_source(
            "def calculate(x):\n    return x * 2\n\nprint(calculate(5))\n",
            "demo.py",
            allow_ai=False,
        )
        self.assertIn("Python syntax check passed", result["report"])
        self.assertIn("SKIPPED — LEGACY PYTHON FRONTEND IS A LIMITED SUBSET PARSER", result["report"])
        self.assertNotIn("Syntax Error 1:", result["report"])
        self.assertIn("Substantive source change:", result["report"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
