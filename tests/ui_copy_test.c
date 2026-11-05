/* Main-loop copy thunks: return values, unaligned buffers and surrounding bytes. */
#include <stdint.h>
#include "../firmware/src/libc.c"
int main(void)
{
    unsigned char a[288], b[288];
    for (unsigned n = 16; n <= 256; n++) for (unsigned shift = 0; shift < 8; shift++) {
        for (unsigned i = 0; i < sizeof a; i++) { a[i] = 0xa5; b[i] = (unsigned char)(i * 37 + n); }
        if (ui_copy(a + shift, b + 3, n) != a + shift) return 1;
        for (unsigned i = 0; i < sizeof a; i++)
            if (a[i] != (i >= shift && i < shift + n ? b[i - shift + 3] : 0xa5)) return 2;
        ui_copy_bytes(a + shift, b + 3, n, 1, 0);
        if (ui_fill(a + shift, 0x19b, n) != a + shift) return 3;
        ui_fill_bytes(a + shift, 0x9b, n, 1, 0);
        for (unsigned i = 0; i < sizeof a; i++)
            if (a[i] != (i >= shift && i < shift + n ? 0x9b : 0xa5)) return 4;
    }
    return 0;
}
