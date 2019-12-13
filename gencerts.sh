#!/bin/bash

# Generate the CA root
openssl genrsa -aes256 -out ca_key.pem 4096
openssl req -new -x509 -days 7300 -key ca_key.pem -sha256 \
    -extensions v3_ca -out ca_cert.pem

# Generate the cpdlcd key & CSR
openssl genrsa -aes256 -out cpdlcd/cpdlcd_key.pem 4096 || exit 1
openssl req -sha256 -new -key cpdlcd/cpdlcd_key.pem -out csr.pem \
    -config openssl.cnf -extensions v3_req || exit 1

# Sign the cpdlcd CSR
openssl x509 -sha256 -req -in csr.pem \
    -CA ca_cert.pem -CAkey ca_key.pem -CAcreateserial \
    -days 3650 -out cpdlcd/cpdlcd_cert.pem \
    -extfile openssl.cnf -extensions v3_req || exit 1

# Verify the signed cert against the CA cert
openssl verify -CAfile ca_cert.pem cpdlcd/cpdlcd_cert.pem || exit 1

# Throw away the CSR and CA serial number file
rm -f csr.pem ca_cert.srl
