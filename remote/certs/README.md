# OTA trust roots

`ota_roots.pem` is the Mozilla/curl CA snapshot dated 2025-02-25, supplied by
Espressif ESP-IDF **v5.5.1**:
https://github.com/espressif/esp-idf/blob/v5.5.1/components/mbedtls/esp_crt_bundle/cacrt_all.pem

Exact file SHA-256:
`50a6277ec69113f00c5fd45f09e8b97a4b3e32daa35d3a95ab30137a55386cef`.
The trust data is public CA material. Mozilla/curl provenance is retained in the PEM.

`python tools/make_ota_bundle.py` (cryptography 46.0.3) generates the compact
subject/public-key format used by ESP-IDF 4.4's `esp_crt_bundle_set`.
PlatformIO embeds the committed binary, so normal compilation needs no generator
or network access for certificates. Tests compare every byte to regenerated data.

This snapshot adds Sectigo server roots E46/R46 missing from the Arduino 2.0.17
SDK bundle. Both CA fingerprints are independently pinned in tests. It also retains
the DigiCert roots needed by GitHub's Azure asset delivery. No leaf certificate is
pinned; normal certificate chain, hostname and time verification remain enabled.
Updating these public roots requires reviewing a new upstream snapshot and updating
the corresponding provenance/hash tests. Do not download trust roots on the pilot.
