"""Check patch-state detection without touching real source trees."""
from pathlib import Path
import os
import subprocess
from tempfile import TemporaryDirectory
from build import patch_is_applied, write_script

with TemporaryDirectory() as directory:
    source = Path(directory)
    target = source / 'sample'
    target.write_text('before\n')
    patch = source / 'change.patch'
    patch.write_text('--- a/sample\n+++ b/sample\n@@ -1 +1 @@\n-before\n+after\n')
    assert not patch_is_applied(source, patch), 'Unapplied patch incorrectly detected as applied'
    assert target.read_text() == 'before\n'
    subprocess.run(['patch', '--batch', '--forward', '-p1', '-i', str(patch)], cwd=source, check=True)
    assert patch_is_applied(source, patch)
    assert target.read_text() == 'after\n'
    target.write_text('unrelated edit\n')
    assert not patch_is_applied(source, patch)
    assert target.read_text() == 'unrelated edit\n'
    script = source / 'compiler'
    write_script(script, '#!/bin/sh\nexit 0\n')
    os.utime(script, ns=(1000000, 1000000))
    write_script(script, '#!/bin/sh\nexit 0\n')
    assert script.stat().st_mtime_ns == 1000000, 'Unchanged compiler invalidates configure'
    write_script(script, '#!/bin/sh\nexit 1\n')
    assert script.read_text() == '#!/bin/sh\nexit 1\n'
    assert script.stat().st_mode & 0o111
print('PASS: unapplied, applied and conflicting patch states; source preserved')
print('PASS: compiler scripts update only when changed')
