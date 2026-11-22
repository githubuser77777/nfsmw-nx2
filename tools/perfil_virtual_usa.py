#!/usr/bin/env python3
"""Instrument the USA 8244F680 virtual draw function after direct-call rewriting."""

import argparse
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("generated", type=Path, help="USA generated/default directory")
    parser.add_argument("--quitar", action="store_true", help="remove this instrumentation")
    args = parser.parse_args()
    files = []
    for path in args.generated.glob("nfsmw_recomp.*.cpp"):
        text = path.read_text(encoding="utf-8")
        if "DEFINE_REX_FUNC(sub_8244F680) {" in text:
            files.append((path, text))
    if len(files) != 1:
        raise SystemExit(f"Expected exactly one USA sub_8244F680; found {len(files)}")
    path, text = files[0]
    start = text.index("DEFINE_REX_FUNC(sub_8244F680) {")
    end = text.find("\nDEFINE_REX_FUNC(", start + 1)
    if end == -1:
        end = len(text)
    body = text[start:end]
    include = '#include "../../src/nfsmw_perfil_subetapas.h"'
    marker = "nfsmw::perfil_subetapas::MedirVirtual<"
    if marker in body:
        if (include not in text or body.count("nfsmw::perfil_subetapas::MarcarVirtual();") != 1
                or any(body.count(f"{marker}{i}>") != 1 for i in range(15))):
            raise SystemExit("Incomplete or unexpected existing instrumentation")
        if args.quitar:
            for i in range(15):
                pattern = rf"{re.escape(marker)}{i}>\(\[&\] \{{ (.*?;) \}}\);"
                body, count = re.subn(pattern, r"\1", body)
                if count != 1:
                    raise SystemExit(f"Unexpected substage {i}; file not changed")
            body = body.replace("\n\tnfsmw::perfil_subetapas::MarcarVirtual();", "", 1)
            text = text[:start] + body + text[end:]
            if "nfsmw::perfil_subetapas::" not in text:
                text = text.replace(include + "\n", "", 1)
            path.write_text(text, encoding="utf-8", newline="\n")
            print(f"Instrumentation removed: {path}")
            return
        print(f"Already instrumented: {path}")
        return
    if args.quitar:
        print(f"Already uninstrumented: {path}")
        return
    returns = ["F6B0", "F718", "F72C", "F748", "F760", "F784",
               "F798", "F7AC", "F7C0", "F7DC", "F7F4", "F808"]
    calls = [(ret, r"REX_CALL_INDIRECT_FUNC\(ctr\.u32\);") for ret in returns]
    calls += [("F6FC", r"(?:__imp__)?sub_8244E8E0\(ctx, base\);"),
              ("F810", r"(?:__imp__)?sub_82699100\(ctx, base\);"),
              ("F828", r"(?:__imp__)?sub_82593A58\(ctx, base\);")]
    for i, (ret, call) in enumerate(calls):
        pattern = rf"(ctx\.lr = 0x8244{ret};\s*)({call})"
        body, count = re.subn(
            pattern, lambda m: f"{m[1]}{marker}{i}>([&] {{ {m[2]} }});", body
        )
        if count != 1:
            raise SystemExit(f"Expected one call after 8244{ret}, found {count}; file not changed")
    body, count = re.subn(r"\tREX_FUNC_PROLOGUE\(\);",
                          "\tREX_FUNC_PROLOGUE();\n\tnfsmw::perfil_subetapas::MarcarVirtual();", body)
    if count != 1:
        raise SystemExit(f"Expected one function prologue, found {count}; file not changed")
    text = text[:start] + body + text[end:]
    if include not in text:
        first_line, remainder = text.split("\n", 1)
        if not first_line.startswith('#include "nfsmw_funcs.'):
            raise SystemExit("Unexpected generated file header; file not changed")
        text = first_line + "\n" + include + "\n" + remainder
    path.write_text(text, encoding="utf-8", newline="\n")
    print(f"Instrumented: {path}")


if __name__ == "__main__":
    main()
