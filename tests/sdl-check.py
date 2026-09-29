#!/usr/bin/env python3
"""Run the production SDL backend offline, with an isolated settings directory."""
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile

binary, assets = (Path(arg).resolve() for arg in sys.argv[1:3])
env = dict(os.environ, SDL_AUDIODRIVER='dummy')
subprocess.run([binary, '--platform-check'], env=env, check=True)
with tempfile.TemporaryDirectory(prefix='openstory-check-') as directory:
    work = Path(directory)
    for source in assets.glob('*.nx'):
        (work / source.name).symlink_to(source)
    (work / 'Settings').write_text('ServerIP = 127.0.0.1\nServerPort = 8484\nSaveLogin = false\n')
    command = [binary, '--asset-check', work]
    subprocess.run(command, env=env, check=True)
    identity = (work / 'ClientID.txt').read_text()
    assert len(identity.strip()) == 20
    subprocess.run(command, env=env, check=True)
    assert (work / 'ClientID.txt').read_text() == identity, 'client identity changed'
    (work / 'ClientID.txt').write_text('invalid\n')
    rejected = subprocess.run(command, env=env, capture_output=True, text=True)
    assert rejected.returncode != 0 and 'Invalid ClientID.txt' in rejected.stderr
    assert '[Init]' not in rejected.stdout, 'invalid identity reached game startup'
    (work / 'ClientID.txt').write_text(identity)
    with socket.socket() as refused:
        refused.bind(('127.0.0.1', 0))  # Reserved local port with no listener.
        (work / 'Settings').write_text(
            f'ServerIP = 127.0.0.1\nServerPort = {refused.getsockname()[1]}\nSaveLogin = false\n')
        startup = subprocess.run([binary, work], env=env, capture_output=True, text=True, timeout=30)
    assert startup.returncode == 1 and 'Cannot connect to server.' in startup.stderr
    assert '[Init] Creating window...' in startup.stdout, startup.stdout
    assert startup.stdout.index('Creating window') < startup.stdout.index('Connecting to server')
print('SDL input, NX rendering/audio, and persistent identity checks passed')
