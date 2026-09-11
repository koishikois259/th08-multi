#pragma once

#include <windows.h>
#include <wincrypt.h>

namespace th08
{

inline bool FillSecureRandom(void *output, DWORD size)
{
    HCRYPTPROV provider;
    bool result;

    if (output == NULL || size == 0)
        return false;
    provider = 0;
    if (!CryptAcquireContextA(&provider, NULL, NULL, PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
        return false;
    result = CryptGenRandom(provider, size, static_cast<BYTE *>(output)) != FALSE;
    CryptReleaseContext(provider, 0);
    return result;
}

inline bool GenerateSecureRandomNonZeroU32(DWORD *value)
{
    unsigned int attempt;

    if (value == NULL)
        return false;
    for (attempt = 0; attempt < 8; ++attempt)
    {
        if (!FillSecureRandom(value, sizeof(*value)))
            return false;
        if (*value != 0)
            return true;
    }
    return false;
}

} // namespace th08
