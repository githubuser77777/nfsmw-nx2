import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "perfil_virtual_usa.py"
RETURNS = ["F6B0", "F718", "F72C", "F748", "F760", "F784",
           "F798", "F7AC", "F7C0", "F7DC", "F7F4", "F808"]
SOURCE = ('#include "nfsmw_funcs.95.h"\nDEFINE_REX_FUNC(sub_8244F680) {\n'
          '\tREX_FUNC_PROLOGUE();\n'
          + ''.join(f'\tctx.lr = 0x8244{ret};\n\tREX_CALL_INDIRECT_FUNC(ctr.u32);\n' for ret in RETURNS)
          + '\tctx.lr = 0x8244F6FC;\n\tsub_8244E8E0(ctx, base);\n'
          + '\tctx.lr = 0x8244F810;\n\t__imp__sub_82699100(ctx, base);\n'
          + '\tctx.lr = 0x8244F828;\n\tsub_82593A58(ctx, base);\n'
          + '}\nDEFINE_REX_FUNC(sub_11111111) {\n\tsub_82593A58(ctx, base);\n}\n')


class InstrumentationTest(unittest.TestCase):
    def test_instruments_only_target_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nfsmw_recomp.95.cpp"
            path.write_text(SOURCE, encoding="utf-8")
            subprocess.run([sys.executable, str(SCRIPT), directory], check=True, capture_output=True)
            result = path.read_text(encoding="utf-8")
            self.assertIn("nfsmw::perfil_subetapas::MarcarVirtual();", result)
            self.assertEqual(result.count("nfsmw::perfil_subetapas::MedirVirtual<"), 15)
            self.assertIn("DEFINE_REX_FUNC(sub_11111111) {\n\tsub_82593A58(ctx, base);", result)
            subprocess.run([sys.executable, str(SCRIPT), directory], check=True, capture_output=True)
            self.assertEqual(path.read_text(encoding="utf-8"), result)
            subprocess.run([sys.executable, str(SCRIPT), directory, "--quitar"], check=True, capture_output=True)
            self.assertEqual(path.read_text(encoding="utf-8"), SOURCE)

    def test_missing_call_does_not_change_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nfsmw_recomp.95.cpp"
            source = SOURCE.replace("sub_8244E8E0(ctx, base);", "")
            path.write_text(source, encoding="utf-8")
            result = subprocess.run([sys.executable, str(SCRIPT), directory], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(path.read_text(encoding="utf-8"), source)


if __name__ == "__main__":
    unittest.main()
