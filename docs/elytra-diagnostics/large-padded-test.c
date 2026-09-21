/* Does Wine's padded in-place AES-CBC decrypt handle the RETAIL SIZE correctly
 * against EXTERNALLY generated ciphertext?
 *
 * This is the gap the rest of the suite leaves open: the 52.7 MB case in
 * bcrypt_inplace_test.c round-trips through Wine itself (self-consistent by
 * construction), and the openssl-backed fixtures are only 48 bytes. The retail
 * driver decrypts 55,275,056 bytes in place with BCRYPT_BLOCK_PADDING. Until
 * now nothing tested that combination.
 *
 * Fixture: openssl AES-128-CBC, no pad, over 55,275,055 pattern bytes plus one
 * 0x01 PKCS7 byte. Correct behaviour: STATUS_SUCCESS, pcbResult 55,275,055.
 */
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif
#define LEN 55275056u
#define EXPECT 55275055u

int main(void)
{
    BCRYPT_ALG_HANDLE alg = NULL; BCRYPT_KEY_HANDLE k = NULL;
    UCHAR secret[16], iv[16], last[16], *keyobj = NULL, *buf = NULL;
    ULONG done = 0, objlen = 0, cb = 0, j; NTSTATUS st = 0; FILE *f;
    int i, rc = 1; unsigned long bad = 0;

    freopen("large-padded-test.txt", "w", stdout);
    if (!(buf = malloc(LEN))) { printf("alloc failed\n"); return 2; }
    if (!(f = fopen("fixtures\\large\\cbc-driver-size.bin", "rb"))) { printf("fixture missing\n"); return 2; }
    if (fread(buf, 1, LEN, f) != LEN) { printf("fixture short\n"); fclose(f); return 2; }
    fclose(f);
    memcpy(last, buf + LEN - 16, 16);
    for (i = 0; i < 16; i++) { secret[i] = (UCHAR)(i*7+1); iv[i] = (UCHAR)(i*3+5); }

    if ((st = BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0))) goto out;
    if ((st = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
                                sizeof(BCRYPT_CHAIN_MODE_CBC), 0))) goto out;
    BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objlen, sizeof(objlen), &cb, 0);
    printf("ObjectLength reported by Wine = %lu\n", objlen);
    keyobj = malloc(objlen);
    if ((st = BCryptGenerateSymmetricKey(alg, &k, keyobj, objlen, secret, sizeof(secret), 0))) goto out;

    /* exactly the retail shape: in-place, padded, driver size */
    st = BCryptDecrypt(k, buf, LEN, NULL, iv, 16, buf, LEN, &done, BCRYPT_BLOCK_PADDING);
    printf("status    = %#lx (want 0x0)\n", (long)st);
    printf("pcbResult = %lu (want %u)\n", done, EXPECT);
    if (!st) {
        for (j = 0; j < EXPECT; j++)
            if (buf[j] != (UCHAR)((j*131+17)&0xff)) { bad++; if (bad==1) printf("first mismatch at offset %lu\n", j); }
        printf("plaintext mismatched bytes = %lu of %u\n", bad, EXPECT);
    }
    printf("iv advanced to last ciphertext block = %d\n", !memcmp(iv, last, 16));
    rc = (!st && done == EXPECT && !bad) ? 0 : 1;
    printf("RESULT: %s\n", rc ? "*** FAIL ***" : "PASS");
out:
    if (st) printf("setup/decrypt status %#lx\n", (long)st);
    if (k) BCryptDestroyKey(k);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    free(keyobj); free(buf);
    return rc;
}
