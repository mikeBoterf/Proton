/* Test Wine's ECDSA P-384 / SHA-384 sign+verify round-trip (the driver's other
 * crypto: BCryptImportKeyPair ECCPUBLICBLOB 104B + BCryptVerifySignature, SHA-384).
 * If this round-trips, Wine's ECDSA verify is correct.
 * Build: x86_64-w64-mingw32-gcc ecdsa_test.c -o e.exe -lbcrypt
 */
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    BCRYPT_ALG_HANDLE ecdsa = NULL, sha = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    NTSTATUS st;
    UCHAR hash[48], sig[96], msg[200];
    ULONG done = 0, siglen = 0, i;

    for (i = 0; i < sizeof(msg); i++) msg[i] = (UCHAR)(i * 91 + 3);

    st = BCryptOpenAlgorithmProvider(&sha, BCRYPT_SHA384_ALGORITHM, NULL, 0);
    if (st) { printf("sha open %#lx\n", (long)st); return 1; }
    st = BCryptHash(sha, NULL, 0, msg, sizeof(msg), hash, sizeof(hash));
    if (st) { printf("hash %#lx\n", (long)st); return 1; }

    st = BCryptOpenAlgorithmProvider(&ecdsa, BCRYPT_ECDSA_P384_ALGORITHM, NULL, 0);
    if (st) { printf("ecdsa open %#lx\n", (long)st); return 1; }
    st = BCryptGenerateKeyPair(ecdsa, &key, 384, 0);
    if (st) { printf("genkeypair %#lx\n", (long)st); return 1; }
    st = BCryptFinalizeKeyPair(key, 0);
    if (st) { printf("finalize %#lx\n", (long)st); return 1; }

    st = BCryptSignHash(key, NULL, hash, sizeof(hash), sig, sizeof(sig), &siglen, 0);
    if (st) { printf("sign %#lx\n", (long)st); return 1; }
    printf("signed, siglen=%lu\n", siglen);

    st = BCryptVerifySignature(key, NULL, hash, sizeof(hash), sig, siglen, 0);
    printf("ECDSA P-384/SHA-384 verify: %s (%#lx)\n",
           st == 0 ? "PASS" : "*** FAIL ***", (long)st);

    /* also a deliberately corrupted hash must FAIL (sanity) */
    hash[0] ^= 0xff;
    st = BCryptVerifySignature(key, NULL, hash, sizeof(hash), sig, siglen, 0);
    printf("corrupted-hash verify (expect fail): %s (%#lx)\n",
           st != 0 ? "correctly-rejected" : "*** WRONGLY ACCEPTED ***", (long)st);
    return 0;
}
