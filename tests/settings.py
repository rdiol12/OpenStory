#!/usr/bin/env python3
"""Read actual Settings files with Unix and Windows line endings."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='openstory-settings-') as directory:
    work = Path(directory)
    source = work / 'check.cpp'
    source.write_text(r'''
#include "Configuration.h"
#include <cassert>
int main() {
    ms::Configuration::get().load();
    assert(ms::Setting<ms::ServerIP>::get().load() == "127.0.0.1");
    assert(ms::Setting<ms::ServerPort>::get().load() == "8484");
    assert(ms::Setting<ms::SaveLogin>::get().load());
    assert(ms::Setting<ms::DefaultAccount>::get().load() == "test account  ");
}
''')
    binary = work / 'check'
    subprocess.run(['c++', '-std=c++17', '-O1', '-ffunction-sections',
                    '-fdata-sections', '-Wl,--gc-sections', '-I/usr/include/GL',
                    '-I' + str(root / 'src'), '-I' + str(root / 'vendor/NoLifeNx'),
                    str(source), str(root / 'src/Configuration.cpp'), '-o', str(binary)], check=True)
    lines = ['# Connection', '', 'ServerIP = 127.0.0.1', 'ServerPort = 8484',
             'SaveLogin = true', 'Account = test account  ', '']
    for newline in ('\n', '\r\n'):
        (work / 'Settings').write_bytes(newline.join(lines).encode())
        subprocess.run([binary], cwd=work, check=True)
print('PASS: LF/CRLF settings load equally; intentional value spaces preserved')
