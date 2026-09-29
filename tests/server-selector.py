#!/usr/bin/env python3
"""Drive the real SDL selector with a virtual pad; connect only to a local refused port."""
import os
from pathlib import Path
import shutil
import socket
import subprocess
import sys
import tempfile

binary, assets, evidence = (Path(arg).resolve() for arg in sys.argv[1:4])
evidence.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='openstory-selector-') as directory:
    work = Path(directory)
    for source in assets.glob('*.nx'):
        (work / source.name).symlink_to(source)
    helper = work / 'pad.c'
    helper.write_text(r'''
#include <SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <png.h>
static int frame, delivered = -1;
int SDL_NumJoysticks(void) { return 1; }
SDL_bool SDL_IsGameController(int n) { return n == 0; }
SDL_GameController* SDL_GameControllerOpen(int n) { return (SDL_GameController*)1; }
SDL_bool SDL_GameControllerGetAttached(SDL_GameController* pad) { return SDL_TRUE; }
const char* SDL_GameControllerName(SDL_GameController* pad) { return "Test DualSense"; }
void SDL_GameControllerClose(SDL_GameController* pad) {}
Sint16 SDL_GameControllerGetAxis(SDL_GameController* pad, SDL_GameControllerAxis axis) { return 0; }
Uint8 SDL_GameControllerGetButton(SDL_GameController* pad, SDL_GameControllerButton button) {
    if (frame == 10 || frame == 70 || frame == 80) return button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    if (frame == 16) return button == SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    if (frame == 20 || frame == 100)
        return button == SDL_CONTROLLER_BUTTON_A;
    if (frame == 36 || frame == 170) return button == SDL_CONTROLLER_BUTTON_B;
    return 0;
}
void SDL_GL_SwapWindow(SDL_Window* window) {
    void (*swap)(SDL_Window*) = dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
    if (frame == 8 || frame == 32 || frame == 160 || frame == 210) {
        int w, h;
        SDL_GL_GetDrawableSize(window, &w, &h);
        unsigned char* pixels = malloc(w*h*4), *flipped = malloc(w*h*4);
        glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        for (int row = 0; row < h; ++row) memcpy(flipped+row*w*4,pixels+(h-row-1)*w*4,w*4);
        png_image image = {0};
        image.version = PNG_IMAGE_VERSION; image.width = w; image.height = h; image.format = PNG_FORMAT_RGBA;
        char name[64]; snprintf(name,sizeof(name),"selector-frame-%03d.png",frame);
        if (!png_image_write_to_file(&image,name,0,flipped,0,NULL)) fputs("FAIL: screenshot capture\n",stderr);
        free(pixels); free(flipped);
    }
    swap(window);
    ++frame;
}
int SDL_PollEvent(SDL_Event* event) {
    int (*poll)(SDL_Event*) = dlsym(RTLD_NEXT, "SDL_PollEvent");
    if (frame != delivered) {
        delivered = frame;
        memset(event, 0, sizeof(*event));
        if (frame == 30) {
            event->type = SDL_TEXTINPUT;
            strcpy(event->text.text, "127.0.0.1");
            return 1;
        }
        if (frame == 8 || frame == 32 || frame == 160 || frame == 210) {
            event->type = SDL_KEYDOWN;
            event->key.keysym.sym = SDLK_F12;
            return 1;
        }
        if (frame == 245) { event->type = SDL_QUIT; puts("Selector test closed normally"); return 1; }
    }
    return poll(event);
}
int connect(int fd, const struct sockaddr* address, socklen_t length) {
    int (*next)(int,const struct sockaddr*,socklen_t) = dlsym(RTLD_NEXT, "connect");
    if (address->sa_family == AF_INET) {
        const struct sockaddr_in* v4 = (const struct sockaddr_in*)address;
        if (v4->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
            fputs("FAIL: attempted non-local network connection\n", stderr);
            errno = EACCES;
            return -1;
        }
        puts("Selector explicitly connected to loopback");
    }
    return next(fd,address,length);
}
''')
    library = work / 'pad.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-I/usr/include/SDL2', helper,
                    '-ldl', '-lGL', '-lpng', '-o', library], check=True)
    with socket.socket() as refused:
        refused.bind(('127.0.0.1', 0))
        (work / 'Settings').write_text(
            f'ServerIP = 192.0.2.1\nServerPort = {refused.getsockname()[1]}\n'
            'OfflinePreview = true\nServerConfigured = false\nSaveLogin = false\n')
        result = subprocess.run([binary, '--server-select', work],
            env=dict(os.environ, SDL_AUDIODRIVER='dummy', LD_PRELOAD=str(library)),
            capture_output=True, text=True, timeout=90)
    (evidence / 'server-selector.log').write_text(result.stdout + result.stderr)
    for screenshot in work.glob('selector-frame-*.png'):
        shutil.copyfile(screenshot, evidence / screenshot.name)
    saved = (work / 'Settings').read_text()
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'FAIL:' not in result.stderr, result.stderr
    assert 'Selector explicitly connected to loopback' in result.stdout, result.stdout
    assert 'ServerIP = 127.0.0.1' in saved and 'ServerConfigured = true' in saved, saved
    assert 'OfflinePreview = true' in saved, saved
    assert '[Exit] offline=1 ui_open=1 window_open=0' in result.stdout, result.stdout
    assert 'Selector test closed normally' in result.stdout
print('PASS: controller text entry, explicit connection, refused-server recovery, offline exit and saved address')
