#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

def get_import_symbols(objects: list[str]):
  "get symbols imported by objects"
  import subprocess
  process = subprocess.Popen(["nm", "--undefined-only", *objects], stdout=subprocess.PIPE, encoding="ascii")
  for line in filter(None, map(str.strip, process.stdout)):
    if line.endswith(":"):
      continue
    yield line.rsplit(None, 1)[-1]

def generate_reexport_c(symbols, c_symbols):
  "generate c source to reexport symbols"
  lines = []
  # declare imported symbol
  lines.append("#define S(i, s) extern char _##i asm(#s);")
  for i, sym in enumerate(symbols):
    lines.append("S(%d, %s)" % (i, sym))
  for sym in c_symbols:
    lines.append("extern char %s;" % sym)
  # define global symbol and use imported symbols
  lines.append("const char *libtest[] = {")
  for i in range(len(symbols)):
    lines.append("  &_%i," % i)
  for sym in c_symbols:
    lines.append("  &%s," % sym)
  lines.append("};")
  return lines

if __name__ == "__main__":
  import sys
  _, out, *objects = sys.argv
  symbols = sorted(set(get_import_symbols(objects)))
  # GTest::gmock_main
  c_symbols = ["main"]
  lines = generate_reexport_c(symbols, c_symbols)
  with open(out, "w") as f:
    f.write("".join(line + "\n" for line in lines))
