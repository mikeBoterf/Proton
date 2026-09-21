#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Synthetic native BCrypt regression test. No game data or driver IOCTLs.
 * Compile with the same x64 toolchain/import libraries as crypto-probe.c.
 * Windows expectations measured on build 26200.9457; failure output size is
 * reported as an observation, not used as a success criterion.
 */
static FILE *report;
static int test(int kind, int in_place, int explicit_provider)
{
    const char *labels[] = {"valid-small", "invalid-zero-length",
                           "invalid-length-17", "invalid-inconsistent-bytes"};
    FILE *file = NULL;
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    UCHAR input[48], original[48], output[48], secret[16], iv[16], last[16];
    UCHAR *object = NULL, *destination = in_place ? input : output;
    ULONG object_size = 0, ignored = 0, output_size = 0;
    NTSTATUS status = 0;
    int pass = 0, plaintext_ok = 1, setup_ok = 0;
    file = fopen(kind ? "fixtures\\cbc-invalid-padding.bin" : "fixtures\\cbc-small.bin", "rb");
    if (!file || fread(input, 1, 48, file) != 48) goto done;
    if (kind == 2) input[31] ^= 17;
    if (kind == 3) input[31] ^= 2;
    memcpy(original, input, 48);
    memcpy(last, input + 32, 16);
    memset(output, 0xcc, 48);
    for (unsigned i = 0; i < 16; ++i) { secret[i] = i * 7 + 1; iv[i] = i * 3 + 5; }
    status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_AES_ALGORITHM,
        explicit_provider ? L"Microsoft Primitive Provider" : NULL, 0);
    if (status) goto done;
    status = BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE,
        (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    if (status) goto done;
    status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
        (PUCHAR)&object_size, sizeof(object_size), &ignored, 0);
    if (status || !(object = malloc(object_size))) goto done;
    status = BCryptGenerateSymmetricKey(algorithm, &key, object, object_size,
        secret, sizeof(secret), 0);
    if (status) goto done;
    setup_ok = 1;
    status = BCryptDecrypt(key, input, 48, NULL, iv, 16,
        destination, 48, &output_size, BCRYPT_BLOCK_PADDING);
    if (!kind && !status) {
        for (unsigned i = 0; i < 47; ++i)
            if (destination[i] != (UCHAR)(i * 131 + 17)) plaintext_ok = 0;
    }
    pass = kind ? status == (NTSTATUS)0xc000003e :
        (!status && output_size == 47 && plaintext_ok && !memcmp(iv, last, 16));
done:
    fprintf(report, "%s provider=%s inplace=%d setup=%s status=0x%08lx output=%lu expected_windows_semantics=%s",
        labels[kind], explicit_provider ? "MicrosoftPrimitiveProvider" : "default",
        in_place, setup_ok ? "PASS" : "FAIL", (unsigned long)status,
        output_size, pass ? "PASS" : "FAIL");
    if (setup_ok) fprintf(report, " final_iv_matches=%d input_unchanged=%d",
        !memcmp(iv, last, 16), !memcmp(input, original, 48));
    fprintf(report, "\n");
    if (key) BCryptDestroyKey(key);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    if (file) fclose(file);
    free(object);
    return pass;
}
void mainCRTStartup(void)
{
    int pass = 1;
    report = fopen("padding-matrix.txt", "w");
    if (!report) ExitProcess(2);
    for (int provider = 0; provider < 2; ++provider)
        for (int in_place = 1; in_place >= 0; --in_place)
            for (int kind = 0; kind < 4; ++kind)
                pass &= test(kind, in_place, provider);
    fclose(report);
    ExitProcess(pass ? 0 : 1);
}
