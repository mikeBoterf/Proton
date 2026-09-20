/* Reproduce Elytra's decrypt pattern under Wine BCrypt to test correctness:
 * AES-128, ChainingMode set on the ALGORITHM handle, IV, no padding,
 * IN-PLACE decrypt (pbInput == pbOutput). Round-trip: plaintext -> encrypt
 * (out-of-place) -> decrypt (in-place) -> must equal plaintext.
 *
 * Build:  x86_64-w64-mingw32-gcc bcrypt_inplace_test.c -o t.exe -lbcrypt
 * Run:    wine t.exe   (prints PASS/FAIL per mode/size)
 */
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif

static int test_mode(const WCHAR *mode, const char *label, ULONG len)
{
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_KEY_HANDLE k1 = NULL, k2 = NULL;
    UCHAR secret[16], iv0[16], iv[16];
    UCHAR keyobj[1024];
    NTSTATUS st;
    ULONG done = 0, objlen = 0, cb = 0;
    UCHAR *plain = malloc(len), *cipher = malloc(len), *work = malloc(len);
    int ok = 0, i;

    for (i = 0; i < 16; i++) { secret[i] = i * 7 + 1; iv0[i] = i * 3 + 5; }
    for (i = 0; i < (int)len; i++) plain[i] = (UCHAR)(i * 131 + 17);

    st = BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (st) { printf("  [%s] open fail %#lx\n", label, (long)st); goto done; }
    /* set chaining mode on the ALGORITHM (as the driver does) */
    st = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)mode,
                           (ULONG)((wcslen(mode) + 1) * sizeof(WCHAR)), 0);
    if (st) { printf("  [%s] setmode fail %#lx\n", label, (long)st); goto done; }
    BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objlen, sizeof(objlen), &cb, 0);

    st = BCryptGenerateSymmetricKey(alg, &k1, keyobj, objlen, secret, sizeof(secret), 0);
    if (st) { printf("  [%s] genkey1 fail %#lx\n", label, (long)st); goto done; }
    memcpy(iv, iv0, 16);
    st = BCryptEncrypt(k1, plain, len, NULL, iv, 16, cipher, len, &done, 0);
    if (st) { printf("  [%s] encrypt fail %#lx\n", label, (long)st); goto done; }

    /* fresh key + IV, IN-PLACE decrypt (input ptr == output ptr) */
    st = BCryptGenerateSymmetricKey(alg, &k2, keyobj, objlen, secret, sizeof(secret), 0);
    if (st) { printf("  [%s] genkey2 fail %#lx\n", label, (long)st); goto done; }
    memcpy(work, cipher, len);
    memcpy(iv, iv0, 16);
    st = BCryptDecrypt(k2, work, len, NULL, iv, 16, work, len, &done, 0);
    if (st) { printf("  [%s] inplace-decrypt fail %#lx\n", label, (long)st); goto done; }

    ok = (memcmp(work, plain, len) == 0);
    printf("  [%s] len=%lu  IN-PLACE round-trip: %s\n", label, len, ok ? "PASS" : "*** FAIL (corrupt) ***");
done:
    if (k1) BCryptDestroyKey(k1);
    if (k2) BCryptDestroyKey(k2);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    free(plain); free(cipher); free(work);
    return ok;
}

int main(void)
{
    printf("Wine BCrypt in-place AES decrypt round-trip test\n");
    test_mode(BCRYPT_CHAIN_MODE_CBC, "AES-CBC 48B",   48);
    test_mode(BCRYPT_CHAIN_MODE_CBC, "AES-CBC 1MB",   1u << 20);
    test_mode(BCRYPT_CHAIN_MODE_CBC, "AES-CBC 52MB",  55275056u);
    test_mode(BCRYPT_CHAIN_MODE_CFB, "AES-CFB 1MB",   1u << 20);
    test_mode(BCRYPT_CHAIN_MODE_ECB, "AES-ECB 1MB",   1u << 20);
    return 0;
}
