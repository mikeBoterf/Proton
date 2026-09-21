#!/bin/sh
# Regenerate the BCrypt test fixtures.
#
# These are deliberately produced by openssl, NOT by Wine. The earlier crypto
# tests round-tripped through the backend under test, which can only demonstrate
# self-consistency -- an implementation wrong in a symmetric way passes. Ciphertext
# from an independent implementation is what makes a decrypt result meaningful.
#
# Key/IV match the constants in padding-matrix.c and bcrypt_inplace_test.c:
#   secret[i] = i*7+1   iv[i] = i*3+5   (i = 0..15)
#
#   cbc-small.bin            47 plaintext bytes (i*131+17) + one 0x01 PKCS7 byte.
#                            Decrypts cleanly to 47 bytes.
#   cbc-invalid-padding.bin  48 zero bytes, encrypted with no padding. The final
#                            plaintext byte is 0, which is not a legal PKCS7
#                            padding length, so a padded decrypt must reject it.
#                            padding-matrix.c derives its other two malformed
#                            cases from this one by XORing byte 31.
set -e
cd "$(dirname "$0")"
KEY=$(python3 -c 'print(bytes((i*7+1)&0xff for i in range(16)).hex())')
IV=$( python3 -c 'print(bytes((i*3+5)&0xff for i in range(16)).hex())')

python3 -c 'import sys; sys.stdout.buffer.write(bytes((i*131+17)&0xff for i in range(47)) + b"\x01")' \
  | openssl enc -aes-128-cbc -nopad -K "$KEY" -iv "$IV" > cbc-small.bin

python3 -c 'import sys; sys.stdout.buffer.write(b"\x00"*48)' \
  | openssl enc -aes-128-cbc -nopad -K "$KEY" -iv "$IV" > cbc-invalid-padding.bin

sha256sum cbc-small.bin cbc-invalid-padding.bin
