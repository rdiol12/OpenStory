#!/usr/bin/env python3
"""Run the publisher's real format and integrity checks; preserve its full log."""
from pathlib import Path
import subprocess
import sys

package, publisher = (Path(value).resolve() for value in sys.argv[1:3])
result = subprocess.run([publisher, 'img_verify', '--passcode', '0' * 32,
    '--format_check', 'on', '--integrity_check', 'on', '--no_progress_bar', package],
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace')
log = package.with_suffix('.verify.log')
log.write_text(result.stdout, encoding='utf-8')
for line in result.stdout.splitlines():
    if '[Error]' in line or '[Warn]' in line or 'Result :' in line:
        print(line)
print('Full verification log:', log)
raise SystemExit(result.returncode)
