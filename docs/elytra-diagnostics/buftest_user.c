/* Sends a 52MB METHOD_BUFFERED IOCTL to \\.\BufTest and compares the driver's
 * FNV-1a hash of what it received against the hash we computed here. Match =>
 * Wine delivered the full buffer intact. Mismatch/short => marshaling bug. */
#include <windows.h>
#include <stdio.h>
#define IOCTL_BUFTEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

int main(int argc, char **argv)
{
    ULONG len = (argc > 1) ? (ULONG)strtoul(argv[1], 0, 10) : 55275056u;
    unsigned char *buf = malloc(len);
    unsigned long long expect, got = 0;
    ULONG i; DWORD ret = 0; BOOL ok;
    HANDLE h;

    for (i = 0; i < len; i++) buf[i] = (unsigned char)(i * 131 + 17);
    expect = 1469598103934665603ULL ^ len;
    for (i = 0; i < len; i++) expect = (expect ^ buf[i]) * 1099511628211ULL;

    h = CreateFileA("\\\\.\\BufTest", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) { printf("open \\\\.\\BufTest failed: %lu\n", GetLastError()); return 2; }

    ok = DeviceIoControl(h, IOCTL_BUFTEST, buf, len, &got, sizeof(got), &ret, NULL);
    printf("IOCTL ok=%d err=%lu bytes_returned=%lu  (sent %lu bytes)\n", ok, GetLastError(), ret, len);
    printf("  user-side hash   = %016llx\n", expect);
    printf("  driver-side hash = %016llx\n", got);
    if (ok && ret == sizeof(got) && got == expect)
        printf("RESULT: buffer delivered INTACT -> marshaling fine -> WD-L014 is (b) environment\n");
    else
        printf("RESULT: buffer MISMATCH/short -> Wine large-IOCTL marshaling BUG -> (a) fixable\n");
    CloseHandle(h);
    return 0;
}
