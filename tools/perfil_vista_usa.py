#!/usr/bin/env python3
"""Instrument USA generated view preparation after codegen and direct-call rewriting."""

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
        if "DEFINE_REX_FUNC(sub_82453BF8) {" in text:
            files.append((path, text))
    if len(files) != 1:
        raise SystemExit(f"Expected exactly one USA sub_82453BF8; found {len(files)}")
    path, text = files[0]
    start = text.index("DEFINE_REX_FUNC(sub_82453BF8) {")
    end = text.find("\nDEFINE_REX_FUNC(", start + 1)
    if end == -1:
        end = len(text)
    body = text[start:end]
    include = '#include "../../src/nfsmw_perfil_subetapas.h"'
    calls = [
        r"(?:__imp__)?sub_824513F0\(ctx, base\);",
        r"(?:__imp__)?sub_82453768\(ctx, base\);",
        r"(?:__imp__)?sub_82452F40\(ctx, base\);",
        r"(?:__imp__)?sub_824531C0\(ctx, base\);",
        r"(?:__imp__)?sub_82453450\(ctx, base\);",
        r"REX_CALL_INDIRECT_FUNC\(ctr\.u32\);",
    ]
    marker = "nfsmw::perfil_subetapas::Medir<"
    if marker in body:
        if (include not in text or body.count("nfsmw::perfil_subetapas::Marcar();") != 1
                or any(body.count(f"{marker}{i}>") != 1 for i in range(len(calls)))):
            raise SystemExit("Incomplete or unexpected existing instrumentation")
        if args.quitar:
            for i in range(len(calls)):
                pattern = rf"{re.escape(marker)}{i}>\(\[&\] \{{ (.*?;) \}}\);"
                body, count = re.subn(pattern, r"\1", body)
                if count != 1:
                    raise SystemExit(f"Unexpected substage {i}; file not changed")
            body = body.replace("\n\tnfsmw::perfil_subetapas::Marcar();", "", 1)
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
    for i, pattern in enumerate(calls):
        body, count = re.subn(
            pattern, lambda m: f"{marker}{i}>([&] {{ {m.group()} }});", body
        )
        if count != 1:
            raise SystemExit(f"Expected one call for substage {i}, found {count}; file not changed")
    if include in text:
        raise SystemExit("Instrumentation header already present unexpectedly; file not changed")
    body, count = re.subn(r"\tREX_FUNC_PROLOGUE\(\);",
                          "\tREX_FUNC_PROLOGUE();\n\tnfsmw::perfil_subetapas::Marcar();", body)
    if count != 1:
        raise SystemExit(f"Expected one function prologue, found {count}; file not changed")
    text = text[:start] + body + text[end:]
    first_line, remainder = text.split("\n", 1)
    if not first_line.startswith('#include "nfsmw_funcs.'):
        raise SystemExit("Unexpected generated file header; file not changed")
    path.write_text(first_line + "\n" + include + "\n" + remainder, encoding="utf-8", newline="\n")
    print(f"Instrumented: {path}")


if __name__ == "__main__":
    main()
