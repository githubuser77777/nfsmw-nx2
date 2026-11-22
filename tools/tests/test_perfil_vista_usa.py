import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "perfil_vista_usa.py"
SOURCE = """#include "nfsmw_funcs.25.h"
DEFINE_REX_FUNC(sub_82453BF8) {
\tREX_FUNC_PROLOGUE();
\tsub_824513F0(ctx, base);
\tsub_82453768(ctx, base);
\t__imp__sub_82452F40(ctx, base);
\t__imp__sub_824531C0(ctx, base);
\t__imp__sub_82453450(ctx, base);
\tREX_CALL_INDIRECT_FUNC(ctr.u32);
}
DEFINE_REX_FUNC(sub_11111111) {
\tsub_824513F0(ctx, base);
}
"""


class InstrumentationTest(unittest.TestCase):
    def test_instruments_only_target_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nfsmw_recomp.25.cpp"
            path.write_text(SOURCE, encoding="utf-8")
            subprocess.run([sys.executable, str(SCRIPT), directory], check=True, capture_output=True)
            result = path.read_text(encoding="utf-8")
            self.assertIn("nfsmw::perfil_subetapas::Marcar();", result)
            self.assertEqual(result.count("nfsmw::perfil_subetapas::Medir<"), 6)
            self.assertIn("DEFINE_REX_FUNC(sub_11111111) {\n\tsub_824513F0(ctx, base);", result)
            subprocess.run([sys.executable, str(SCRIPT), directory], check=True, capture_output=True)
            self.assertEqual(path.read_text(encoding="utf-8"), result)
            subprocess.run([sys.executable, str(SCRIPT), directory, "--quitar"], check=True, capture_output=True)
            self.assertEqual(path.read_text(encoding="utf-8"), SOURCE)

    def test_missing_call_does_not_change_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nfsmw_recomp.25.cpp"
            source = SOURCE.replace("__imp__sub_82453450(ctx, base);", "")
            path.write_text(source, encoding="utf-8")
            result = subprocess.run([sys.executable, str(SCRIPT), directory], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(path.read_text(encoding="utf-8"), source)


if __name__ == "__main__":
    unittest.main()
