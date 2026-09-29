"""Generate references to actual Java exports in the linked static archives."""
from pathlib import Path
import re
import subprocess

JAVA_EXPORT = re.compile(r'(?:Java_|JNI_|JVM_|JDK_|JNU_|ZIP_|JIMAGE_|VerifyClass)[A-Za-z0-9_]*\Z')


def generate(archives, output):
    symbols = set()
    libraries = []
    for archive in archives:
        archive = Path(archive)
        if not re.fullmatch(r'lib[A-Za-z0-9_]+\.a', archive.name):
            raise ValueError(f'Unexpected library name: {archive.name}')
        libraries.append(archive.with_suffix('.so').name)
        listing = subprocess.check_output(['llvm-nm-18', '--defined-only', '--extern-only',
                                          '--format=posix', str(archive)], text=True)
        for line in listing.splitlines():
            fields = line.split()
            if len(fields) >= 2 and fields[1] in ('T', 'W') and JAVA_EXPORT.fullmatch(fields[0]):
                symbols.add(fields[0])
    if not symbols:
        raise ValueError('No Java native exports found in these archives')
    symbols = sorted(symbols)
    lines = ['/* Generated from compiled archives; every entry is resolved by the linker. */']
    for index, name in enumerate(symbols):
        lines.append(f'extern void cosmic_symbol_{index}(void) __asm__("{name}");')
    lines.append('static const struct native_symbol { const char *name; void (*address)(void); } symbols[] = {')
    lines.extend(f'    {{"{name}", cosmic_symbol_{index}}},' for index, name in enumerate(symbols))
    lines.extend(['};', 'static const char *const libraries[] = {'])
    lines.extend(f'    "{name}",' for name in sorted(set(libraries)))
    lines.append('};\n')
    Path(output).write_text('\n'.join(lines))
    return len(symbols)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('archives', nargs='+', type=Path)
    args = parser.parse_args()
    print(f'Generated {generate(args.archives, args.output)} native references in {args.output}')
