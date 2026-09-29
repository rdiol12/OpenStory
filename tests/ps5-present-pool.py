#!/usr/bin/env python3
"""Exercise SDK registration, RGBA8 display format and pending-frame ownership."""
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
if len(sys.argv) > 1:
    source = Path(sys.argv[1]).read_text()
else:
    with tarfile.open(root / 'build/deps/ps5-opengl-sdk-0.3.0/sources/ps5-opengl.tar') as archive:
        source = archive.extractfile('ps5-opengl/src/platform/ps5_agc_native_runtime.c').read().decode()

def section(start, end):
    return source[source.index(start):source.index(end)]

code = r'''
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#define PS5_GPU_PRESENT_BATCH 1
#define FRAMEBUFFER_BYTES 0xa00000u
#define FRAMEBUFFER_POOL_BYTES (2u * FRAMEBUFFER_BYTES)
#define FRAMEBUFFER_ALIGNMENT 0x200000u
#define DISPLAY_WIDTH 1920
#define DISPLAY_HEIGHT 1080
#define RENDER_MARKER 100
''' + next(line for line in source.splitlines() if line.startswith('#define VIDEO_OUT_PIXEL_FORMAT ')) + '\n' + section('typedef struct video_buffer {', 'typedef struct agc_api {') + r'''
typedef struct { int unused; } agc_api_t;
''' + section('typedef struct video_api {', 'static int load_apis(') + r'''
static video_api_t runtime_video_api;
static int runtime_video_handle = -1, runtime_video_registered;
static uint8_t *runtime_video_framebuffer;
static size_t runtime_video_framebuffer_size;
static unsigned runtime_present_count, runtime_gpu_present_count;
static uint64_t runtime_render_marker;
static int runtime_gpu_present_buffer = -1;
static int opens, closes, registrations, loads, flushes;
static unsigned char *base = (void *)(uintptr_t)0x214000000;
static int open_video(int32_t u, int32_t b, int32_t i, const void *p) { ++opens; return 7; }
static int close_video(int32_t h) { ++closes; return 0; }
static int rate(int32_t h, int32_t r) { return 0; }
static void attribute(video_attribute_t *a, uint64_t f, uint32_t t, uint32_t w,
                      uint32_t h, uint64_t x, uint32_t y, uint64_t z) {
    /* EGL scanout is PIPE_FORMAT_R8G8B8A8_UNORM: little-endian ABGR8888. */
    assert(f == UINT64_C(0x8000000022000000));
    assert(t == 0 && w == DISPLAY_WIDTH && h == DISPLAY_HEIGHT);
}
static int buffers(int32_t h, int32_t g, int32_t s, video_buffer_t *b, int32_t n,
                   video_attribute_t *a, int32_t f, void *p) {
    assert(n == 2 && b[0].data == base);
    ++registrations; return 0;
}
static int pending(int32_t h) { return 0; }
static int wait_vblank(int32_t h) { return 0; }
static video_api_t api = {.open=open_video, .close=close_video, .set_flip_rate=rate,
    .set_attribute2=attribute, .register_buffers2=buffers,
    .is_flip_pending=pending, .wait_vblank=wait_vblank};
static int load_apis(void *a, void *b, void *c, agc_api_t *agc, video_api_t *v) {
    ++loads; *v=api; return 0;
}
static void flush_gpu_data(const void *p, size_t n) { ++flushes; }
static int sceKernelUsleep(uint32_t us) { return 0; }
static int runtime_video_wait_idle(void);
''' + section('int ps5_agc_gate2_shutdown_present(void)', 'static int runtime_video_prepare_draw(') + r'''
int main(void) {
    int attempts;
    /* The draw path registers 20 MiB, then EGL passes its 64 MiB allocation. */
    assert(!runtime_video_acquire(&api, base, FRAMEBUFFER_POOL_BYTES, &attempts));
    assert(opens == 1 && registrations == 1);
    runtime_gpu_present_buffer = 0;
    assert(!ps5_agc_gate2_prepare_present(base, 0x4000000));
    assert(!runtime_video_acquire(&api, base, 0x4000000, &attempts));
    assert(!attempts && !loads && !flushes && !closes);
    assert(opens == 1 && registrations == 1 && runtime_gpu_present_buffer == 0);
    assert(!ps5_agc_gate2_prepare_present(base, FRAMEBUFFER_POOL_BYTES));
    /* Invalid input and replacement still fail without releasing owned memory. */
    assert(ps5_agc_gate2_prepare_present(NULL, 0x4000000));
    assert(ps5_agc_gate2_prepare_present(base + 1, 0x4000000));
    assert(ps5_agc_gate2_prepare_present(base, FRAMEBUFFER_BYTES - 1));
    assert(ps5_agc_gate2_prepare_present(base + FRAMEBUFFER_ALIGNMENT, 0x4000000));
    assert(runtime_video_framebuffer == base && !closes && opens == 1);
    /* A single aliased slot cannot satisfy two slots while still pending. */
    runtime_video_framebuffer_size = FRAMEBUFFER_BYTES;
    assert(ps5_agc_gate2_prepare_present(base, 0x4000000));
    assert(runtime_video_framebuffer_size == FRAMEBUFFER_BYTES && !closes);
    runtime_gpu_present_buffer = -1;
    assert(!ps5_agc_gate2_prepare_present(base, 0x4000000));
    assert(closes == 1 && opens == 2 && registrations == 2);
    assert(!ps5_agc_gate2_prepare_present(base, FRAMEBUFFER_POOL_BYTES));
    assert(closes == 1 && opens == 2);
    puts("PASS: RGBA8 display format, shared pool reuse, pending-frame ownership, expansion and invalid input");
}
'''
with tempfile.TemporaryDirectory(prefix='openstory-present-') as temp:
    folder = Path(temp)
    (folder / 'check.c').write_text(code)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Werror', str(folder / 'check.c'),
                    '-o', str(folder / 'check')], check=True)
    subprocess.run([str(folder / 'check')], check=True)
