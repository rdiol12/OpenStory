"""Add diagnostic call boundaries to the existing native CRT's build copy."""


def instrument(source):
    replacements = {
        '#include <cstdint>': '#include <cstdint>\nextern "C" void openstory_diagnostics_phase(const char *, std::uintptr_t);',
        '    while (first != last)\n        (*first++)();': '''    while (first != last) {
        const auto callback = *first++;
        openstory_diagnostics_phase("constructor-begin", reinterpret_cast<std::uintptr_t>(callback));
        callback();
        openstory_diagnostics_phase("constructor-end", reinterpret_cast<std::uintptr_t>(callback));
    }''',
        '    if (loader_teardown != nullptr)': '    openstory_diagnostics_phase("atexit-loader", reinterpret_cast<std::uintptr_t>(loader_teardown));\n    if (loader_teardown != nullptr)',
        '    (void)atexit(_fini);': '    openstory_diagnostics_phase("atexit-fini", 0);\n    (void)atexit(_fini);',
        '    _init();': '    openstory_diagnostics_phase("constructors-begin", 0);\n    _init();\n    openstory_diagnostics_phase("constructors-complete", 0);',
    }
    if 'openstory_diagnostics_phase' in source:
        raise ValueError('Native startup is already instrumented')
    for before, after in replacements.items():
        if source.count(before) != 1:
            raise ValueError('Unexpected native startup source: ' + before)
        source = source.replace(before, after)
    return source
