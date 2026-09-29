#!/usr/bin/env python3
"""Check native data paths and load a real NX file without changing directory."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
base = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as directory:
    work = Path(directory)
    assets = work / 'assets'
    assets.mkdir()
    shutil.copyfile(base, assets / 'Base.nx')
    source = work / 'check.cpp'
    source.write_text('''#include "Util/Paths.h"
#include <nlnx/nx.hpp>
#include <nlnx/node.hpp>
#include <cassert>
#include <filesystem>
int main(int argc, char** argv) {
#ifdef PLATFORM_PS5
    assert(ms::data_path("Settings") == "/download0/openstory/Settings");
    assert(ms::data_path("ClientID.txt") == "/download0/openstory/ClientID.txt");
    assert(ms::data_path("") == "/download0/openstory/");
#else
    assert(ms::data_path("Settings") == "Settings");
    assert(ms::data_path("ClientID.txt") == "ClientID.txt");
    assert(ms::data_path("").empty());
#endif
    assert(ms::data_path("/app0/Settings") == "/app0/Settings");
    const auto before = std::filesystem::current_path();
    nl::nx::load_all(argc > 1 ? argv[1] : "");
    assert(nl::nx::base);
    assert(std::filesystem::current_path() == before);
}
''')
    nx = root / 'vendor/NoLifeNx/nlnx'
    binary = work / 'check'
    for defines in (['-DPLATFORM_PS5'], []):
        subprocess.run(['c++', '-std=c++17', '-O1', *defines,
                    '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                    '-I' + str(root / 'src'), '-I' + str(nx.parent), str(source),
                    *[str(nx / name) for name in ('nx.cpp', 'node.cpp', 'file.cpp')],
                        '-o', str(binary)], check=True)
        for suffix in ('', '/'):
            subprocess.run([binary, str(assets) + suffix], cwd=work, check=True)
        subprocess.run([binary], cwd=assets, check=True)
print('PASS: absolute native data paths; explicit and default NX roots preserve cwd')
