#!/usr/bin/env python3
"""Check the POSIX logger preserves signal termination and writes its report."""
import argparse
from pathlib import Path
import resource
import signal
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[1])
root = parser.parse_args().source_root.resolve()
with tempfile.TemporaryDirectory(prefix='openstory-crashlog-') as directory:
    work = Path(directory)
    source = work / 'check.cpp'
    source.write_text('''
#include "Util/CrashLog.h"
#include <csignal>
#include <stdexcept>
int main(int argc, char**) {
    ms::install_crash_logger();
    if (argc > 1) throw std::runtime_error("logger check");
    std::raise(SIGABRT);
}
''')
    binary = work / 'check'
    subprocess.run(['c++', '-std=c++17', '-DPLATFORM_MACOS', '-I' + str(root / 'src'),
                    str(source), str(root / 'src/Util/CrashLog.cpp'), '-o', str(binary)], check=True)
    # Only explicit raise/throw in this process; no crash payload or core dump.
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    for args, marker in (([], 'FATAL SIGNAL'), (['terminate'], 'UNCAUGHT EXCEPTION')):
        result = subprocess.run([binary, *args], cwd=work, capture_output=True, timeout=5)
        assert result.returncode == -signal.SIGABRT, result.returncode
        assert marker in (work / 'crashlog.txt').read_text()
        print('PASS:', marker, 'report and original signal termination')
