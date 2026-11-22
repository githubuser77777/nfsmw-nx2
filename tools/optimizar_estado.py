#!/usr/bin/env python3
"""Reuse the translation of the effect-state object's +28 field on Switch 2."""

import argparse
from pathlib import Path
import re

PAL = "8244EA48"
ORIGINAL = "REX_LOAD_U32(r31.u32 + 28)"
MARKER = "nfsmw_estado_offset28"
PREFIX = """#if defined(__SWITCH2__)
#define NFSMW_ESTADO_LOAD28() __builtin_bswap32(*(volatile u32*)(base + nfsmw_estado_offset28))
#else
#define NFSMW_ESTADO_LOAD28() REX_LOAD_U32(r31.u32 + 28)
#endif
"""
SUFFIX = "\n#undef NFSMW_ESTADO_LOAD28\n"
DECLARATION = ("\n#if defined(__SWITCH2__)\n"
               "\tconst uint64_t nfsmw_estado_offset28 = rex::memory::FoldGuestAddress(r31.u32 + 28);\n"
               "#endif")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("generated", type=Path)
    parser.add_argument("--tabla", type=Path, help="PAL to edition address table")
    parser.add_argument("--quitar", action="store_true", help="restore the original generated loads")
    parser.add_argument("--opcional", action="store_true", help="skip editions without an exact mapping")
    args = parser.parse_args()
    address = PAL
    if args.tabla:
        rows = [line.split() for line in args.tabla.read_text(encoding="utf-8").splitlines()]
        matches = [row[2].upper() for row in rows
                   if len(row) >= 4 and row[0].upper() == PAL and row[3] == "exacta"]
        if len(matches) != 1 or not re.fullmatch(r"[0-9A-F]{8}", matches[0]):
            if args.opcional and not matches:
                print(f"Skipping effect-state optimization: no exact edition mapping for {PAL}")
                return
            raise SystemExit(f"No unique exact edition mapping for {PAL}")
        address = matches[0]
    target = f"DEFINE_REX_FUNC(sub_{address}) {{"
    files = []
    for path in args.generated.glob("nfsmw_recomp.*.cpp"):
        text = path.read_text(encoding="utf-8")
        if target in text:
            files.append((path, text))
    if len(files) != 1:
        raise SystemExit(f"Expected exactly one {target}; found {len(files)}")
    path, text = files[0]
    start = text.index(target)
    end = text.find("\nDEFINE_REX_FUNC(", start + 1)
    if end == -1:
        end = len(text)
    body = text[start:end]
    if MARKER in body:
        if (PREFIX not in text or SUFFIX not in body or body.count(DECLARATION) != 1
                or body.count("NFSMW_ESTADO_LOAD28()") != 6):
            raise SystemExit("Incomplete or unexpected existing optimization")
        if args.quitar:
            body = body.replace(DECLARATION, "", 1)
            body = body.replace("NFSMW_ESTADO_LOAD28()", ORIGINAL)
            body = body.replace(SUFFIX, "", 1)
            text = text[:start] + body + text[end:]
            text = text.replace(PREFIX, "", 1)
            path.write_text(text, encoding="utf-8", newline="\n")
            print(f"Restored effect-state translations: {path}")
            return
        print(f"Already optimized: {path}")
        return
    if args.quitar:
        print(f"Already unoptimized: {path}")
        return
    if body.count(ORIGINAL) != 6:
        raise SystemExit("Unexpected effect-state object loads; file not changed")
    assignment = "\tr31.u64 = ctx.r3.u64;"
    assignments = re.findall(r"\br31\.\w+\s*(?:\+\+|--|(?:<<|>>|[+\-*/&|^%])?=(?!=))", body)
    if (body.count(assignment) != 1 or len(assignments) != 1
            or re.search(r"(?:\+\+|--)\s*r31\.\w+", body)):
        raise SystemExit("Unexpected effect-state object assignment; file not changed")
    body = body.replace(assignment, assignment + DECLARATION, 1)
    body = body.replace(ORIGINAL, "NFSMW_ESTADO_LOAD28()")
    text = text[:start] + PREFIX + body + SUFFIX + text[end:]
    path.write_text(text, encoding="utf-8", newline="\n")
    print(f"Optimized Switch 2 effect-state translations: {path}")


if __name__ == "__main__":
    main()
