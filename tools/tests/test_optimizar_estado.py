import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "optimizar_estado.py"
LOADS = [(16, 6), (32, 0), (32, 0), (32, 16), (32, 20), (32, 24)] + [(32, 28)] * 6
SOURCE = ('#include "nfsmw_funcs.1.h"\nDEFINE_REX_FUNC(sub_8244E8E0) {\n'
          '\tr31.u64 = ctx.r3.u64;\n'
          + ''.join(f'\tctx.r11.u64 = REX_LOAD_U{width}(r31.u32 + {offset});\n'
                    for width, offset in LOADS)
          + '}\n\nDEFINE_REX_FUNC(sub_11111111) {\n'
          + '\tctx.r11.u64 = REX_LOAD_U32(r31.u32 + 28);\n}\n')
TABLE = "8244EA48\t.text\t8244E8E0\texacta\n"


class EffectStateOptimizationTest(unittest.TestCase):
    def test_round_trip_and_edition_mapping(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nfsmw_recomp.1.cpp"
            table = Path(directory) / "tabla.tsv"
            path.write_text(SOURCE, encoding="utf-8")
            table.write_text(TABLE, encoding="utf-8")
            command = [sys.executable, str(SCRIPT), directory, "--tabla", str(table)]
            subprocess.run(command, check=True, capture_output=True)
            result = path.read_text(encoding="utf-8")
            self.assertEqual(result.count("NFSMW_ESTADO_LOAD28()"), 8)
            self.assertIn("DEFINE_REX_FUNC(sub_11111111) {\n"
                          "\tctx.r11.u64 = REX_LOAD_U32(r31.u32 + 28);", result)
            subprocess.run(command, check=True, capture_output=True)
            self.assertEqual(path.read_text(encoding="utf-8"), result)
            subprocess.run(command + ["--quitar"], check=True, capture_output=True)
            self.assertEqual(path.read_text(encoding="utf-8"), SOURCE)

    def test_missing_expected_load_fails_without_editing(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nfsmw_recomp.1.cpp"
            table = Path(directory) / "tabla.tsv"
            source = SOURCE.replace("REX_LOAD_U32(r31.u32 + 28)", "REX_LOAD_U32(r31.u32 + 32)", 1)
            path.write_text(source, encoding="utf-8")
            table.write_text(TABLE, encoding="utf-8")
            result = subprocess.run([sys.executable, str(SCRIPT), directory, "--tabla", str(table)],
                                    capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(path.read_text(encoding="utf-8"), source)

    def test_object_register_change_fails_without_editing(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nfsmw_recomp.1.cpp"
            table = Path(directory) / "tabla.tsv"
            source = SOURCE.replace("r31.u64 = ctx.r3.u64;", "r31.u64 = ctx.r3.u64;\n\tr31.u32 += 4;")
            path.write_text(source, encoding="utf-8")
            table.write_text(TABLE, encoding="utf-8")
            result = subprocess.run([sys.executable, str(SCRIPT), directory, "--tabla", str(table)],
                                    capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(path.read_text(encoding="utf-8"), source)


if __name__ == "__main__":
    unittest.main()
