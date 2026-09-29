"""Build SDK 0.3.0 presenter fixes in OpenStory's build directory."""
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import tarfile


def build_presenter(sdk, opengl, output):
    provenance = json.loads((opengl / 'provenance.json').read_text())
    if provenance['runtime_source_commit'] != '6cb291abea32281571c49705735046425cf000fd':
        raise ValueError('Recheck the presenter fix before changing the OpenGL SDK')
    inputs = {
        'ps5-opengl.tar': [
            'ps5-opengl/src/platform/ps5_agc_native_runtime.c',
            'ps5-opengl/src/platform/ps5_agc_runtime_backend.c',
            'ps5-opengl/src/platform/ps5_agc_package.h',
            'ps5-opengl/src/platform/ps5_scanout.h',
            'ps5-opengl/src/gallium/ps5/ps5_screen.h',
            'ps5-opengl/toolchain/opengnm-psbc-ps5.patch'],
        'mesa-26.2.0.tar.xz': ['mesa-26.2.0/src/util/os_time.h'],
        'opengnm-psbc.tar': ['opengnm-psbc/libpsbc/psbc_compile.h'],
    }
    for name, members in inputs.items():
        archive_path = opengl / 'sources' / name
        if hashlib.sha256(archive_path.read_bytes()).hexdigest() != provenance['source_archives'][name]:
            raise ValueError('SDK source archive changed: ' + name)
        with tarfile.open(archive_path) as archive:
            for member in members:
                destination = output / member
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(archive.extractfile(member).read())
    # The published library uses this bundled patch, not the unpatched header.
    subprocess.run(['git', 'apply', '--include=libpsbc/psbc_compile.h',
                    str(output / 'ps5-opengl/toolchain/opengnm-psbc-ps5.patch')],
                   cwd=output / 'opengnm-psbc', check=True)
    source = output / inputs['ps5-opengl.tar'][0]
    original = source.read_text()
    before = '        runtime_video_framebuffer_size >= framebuffer_size)'
    after = '''        (runtime_video_framebuffer_size >= framebuffer_size ||
         runtime_video_framebuffer_size >= FRAMEBUFFER_POOL_BYTES))'''
    if original.count(before) != 2:
        raise ValueError('Unexpected SDK presenter registration checks')
    # Registration owns at most two scanout slots. Extra render-arena bytes
    # do not require re-registration (which is forbidden with a pending flip).
    patched = original.replace(before, after)
    before_format = '#define VIDEO_OUT_PIXEL_FORMAT UINT64_C(0x8000000000000000)'
    if patched.count(before_format) != 1:
        raise ValueError('Unexpected SDK display pixel format')
    # EGL scanout is R8G8B8A8_UNORM. Match its RGBA bytes with the PS5 SDL
    # backend's ABGR8888 (little-endian) VideoOut format, including plain UI.
    # https://github.com/ps5-payload-dev/SDL/blob/release-2.30.x-ps5/src/video/ps5/SDL_ps5video.c
    source.write_text(patched.replace(before_format,
        '#define VIDEO_OUT_PIXEL_FORMAT UINT64_C(0x8000000022000000)'))
    subprocess.run(['python3', Path(__file__).resolve().parents[2] / 'tests/ps5-present-pool.py',
                    source], check=True)
    flags = [flag for flag in shlex.split((opengl / 'runtime-config.txt').read_text())
             if flag.startswith(('-D', '-std=', '-O', '-g', '-W', '-f'))]
    obj = output / 'ps5_agc_runtime_backend.o'
    command = [str(sdk / 'bin/prospero-clang'), *flags, '-Wno-error=unused-function',
               '-I' + str(output / 'ps5-opengl/src/gallium/ps5'),
               '-I' + str(output / 'opengnm-psbc/libpsbc'),
               '-I' + str(output / 'mesa-26.2.0/src'),
               '-Dmain=ps5_agc_gate2_run', '-DAGC_TRIANGLE_SUBMIT=1', '-DAGC_RUNTIME_PACKAGES=1',
               '-c', str(output / inputs['ps5-opengl.tar'][1]), '-o', str(obj)]
    subprocess.run(command, check=True)
    receipt = {'baseRuntimeSourceCommit': provenance['runtime_source_commit'],
               'changes': ['Reuse two registered display slots when the caller includes render-arena bytes',
                           'Match VideoOut ABGR8888 to the RGBA8 scanout allocation'],
               'videoOutPixelFormat': '0x8000000022000000',
               'originalSourceSha256': hashlib.sha256(original.encode()).hexdigest(),
               'patchedSourceSha256': hashlib.sha256(source.read_bytes()).hexdigest(),
               'objectSha256': hashlib.sha256(obj.read_bytes()).hexdigest(),
               'command': command, 'hardwareTested': False}
    (output / 'presenter-fix.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return obj
