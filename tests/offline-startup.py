#!/usr/bin/env python3
"""Run the real offline login loop, verify no connection, then close through SDL."""
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile

binary, assets = (Path(arg).resolve() for arg in sys.argv[1:3])
with tempfile.TemporaryDirectory(prefix='openstory-offline-') as directory:
    work = Path(directory)
    for source in assets.glob('*.nx'):
        (work / source.name).symlink_to(source)
    helper = work / 'close.c'
    helper.write_text(r'''
#include <SDL.h>
#include <dlfcn.h>
#include <stdio.h>
static int frames;
void SDL_GL_SwapWindow(SDL_Window *window) {
    void (*swap)(SDL_Window*) = dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
    swap(window);
    ++frames;
}
int SDL_PollEvent(SDL_Event *event) {
    int (*poll)(SDL_Event*) = dlsym(RTLD_NEXT, "SDL_PollEvent");
    if (frames == 600) {
        ++frames;
        *event = (SDL_Event){.type = SDL_QUIT};
        puts("Offline preview rendered 600 frames; closing through SDL");
        return 1;
    }
    return poll(event);
}
''')
    library = work / 'close.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-I/usr/include/SDL2', str(helper),
                    '-ldl', '-o', str(library)], check=True)
    with socket.socket() as listener:
        listener.bind(('127.0.0.1', 0))
        listener.listen()
        listener.settimeout(0.1)
        (work / 'Settings').write_bytes((
            f'ServerIP = 127.0.0.1\r\nServerPort = {listener.getsockname()[1]}\r\n'
            'SaveLogin = false\r\nOfflinePreview = true\r\n').encode())
        result = subprocess.run([binary, work], env=dict(os.environ,
                                SDL_AUDIODRIVER='dummy', LD_PRELOAD=str(library)),
                                capture_output=True, text=True, timeout=90)
        print(result.stdout, end='')
        print(result.stderr, end='', file=sys.stderr)
        assert result.returncode == 0, result.returncode
        assert '[Init] Ready.' in result.stdout and 'rendered 600 frames' in result.stdout
        assert 'Connecting to server' not in result.stdout
        assert '[Exit] offline=1 ui_open=1 window_open=0' in result.stdout, result.stdout
        try:
            peer, _ = listener.accept()
        except TimeoutError:
            pass
        else:
            peer.close()
            raise AssertionError('Offline preview attempted a server connection')
print('PASS: offline login renders, sends no connection, and closes normally')
