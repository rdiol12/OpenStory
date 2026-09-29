#!/usr/bin/env python3
"""Check the production screenshot path reports failures without changing the game."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'platform/sdl/WindowSDL.cpp').read_text()
start = source.index('    void Window::take_screenshot()')
method = source[start:source.index('\n    }', start) + len('\n    }')]
code = r'''
#include <cassert>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
using GLenum = unsigned;
constexpr unsigned GL_RGBA=1, GL_UNSIGNED_BYTE=2, GL_NO_ERROR=0;
static int writes, failure;
static const unsigned char expected[]={255,0,0,255,0,0,255,255};
void glReadPixels(int x,int y,int w,int h,unsigned f,unsigned t,void *p) {
    assert(!x && !y && w==2 && h==1 && f==GL_RGBA && t==GL_UNSIGNED_BYTE);
    memcpy(p,expected,sizeof(expected));
}
unsigned glGetError() { return failure==1 ? 0x502 : 0; }
void stbi_flip_vertically_on_write(int flip) { assert(flip==1); }
int stbi_write_png(const char *,int w,int h,int c,const void *p,int stride) {
    assert(w==2 && h==1 && c==4 && stride==8 && !memcmp(p,expected,sizeof(expected)));
    ++writes; return failure!=2;
}
std::string data_path(const std::string& p) { return p; }
struct Window { int display_width=2, display_height=1; void take_screenshot(); };
''' + method + r'''
int main(int argc,char **argv) {
    if(argc>1) failure=argv[1][0]-'0';
    Window window; window.take_screenshot();
    assert(writes==(failure!=1));
}
'''
with tempfile.TemporaryDirectory(prefix='openstory-screenshot-') as temp:
    folder = Path(temp)
    (folder / 'check.cpp').write_text(code)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Werror', str(folder / 'check.cpp'),
                    '-o', str(folder / 'check')], check=True)
    for failure, message in [(1, 'screenshot-read-failed'), (2, 'screenshot-write-failed'), (0, 'screenshot-saved')]:
        result = subprocess.run([str(folder / 'check'), str(failure)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        assert message in result.stdout + result.stderr, result.stdout + result.stderr
print('PASS: screenshot preserves RGBA bytes and reports read/write failures')
