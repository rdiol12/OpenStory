/* Host regression check for the PS5 libffi ABI restriction; not a console test. */
#include <assert.h>
#include <ffi.h>
#include <stdint.h>
#include <stdio.h>

static double mixed(int64_t a, double b, int64_t c, double d,
                    int64_t e, int64_t f, int64_t g, int64_t h, int64_t i) {
    return a + b + c + d + e + f + g + h + i;
}

int main(void) {
    ffi_cif cif;
    ffi_type *types[] = {&ffi_type_sint64, &ffi_type_double, &ffi_type_sint64,
        &ffi_type_double, &ffi_type_sint64, &ffi_type_sint64, &ffi_type_sint64,
        &ffi_type_sint64, &ffi_type_sint64};
    int64_t a = 1, c = 3, e = 5, f = 6, g = 7, h = 8, i = 9;
    double b = 2.25, d = 4.5, result = 0;
    void *args[] = {&a, &b, &c, &d, &e, &f, &g, &h, &i};
    assert(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 9, &ffi_type_double, types) == FFI_OK);
    ffi_call(&cif, FFI_FN(mixed), &result, args);
    assert(result == mixed(a, b, c, d, e, f, g, h, i));
    assert(ffi_prep_cif(&cif, FFI_WIN64, 9, &ffi_type_double, types) == FFI_BAD_ABI);
    assert(ffi_prep_cif(&cif, FFI_GNUW64, 9, &ffi_type_double, types) == FFI_BAD_ABI);
    puts("PASS: mixed register/stack arguments; unsupported ABIs rejected (host only)");
}
