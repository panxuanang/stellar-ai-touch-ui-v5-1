#!/usr/bin/env python3
"""Create secret_config.h from secret_config.h.example when upstream requires it."""
from pathlib import Path
import shutil
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
count = 0
for example in root.rglob("secret_config.h.example"):
    target = example.with_name("secret_config.h")
    if target.exists():
        print(f"[secret] exists: {target.relative_to(root)}")
        continue
    shutil.copy2(example, target)
    count += 1
    print(f"[secret] created: {target.relative_to(root)}")
print(f"[secret] generated {count} file(s)")
