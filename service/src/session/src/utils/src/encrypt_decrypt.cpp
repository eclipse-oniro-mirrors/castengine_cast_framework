/*
 * Copyright (C) 2023-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * Description: encrypt and decrypt data module
 * Author: lijianzhao
 * Create: 2022-01-19
 */

#include "encrypt_decrypt.h"

#include <cstdint>
#include "securec.h"
#include "cast_engine_log.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-EncryptDecrypt");

const std::string EncryptDecrypt::CIPHER_AES_CTR_128 = "aes128ctr";
const std::string EncryptDecrypt::CIPHER_AES_GCM_128 = "aes128gcm";

EncryptDecrypt::EncryptDecrypt() {}

EncryptDecrypt::~EncryptDecrypt() {}

EncryptDecrypt &EncryptDecrypt::GetInstance()
{
    return Singleton<EncryptDecrypt>::GetInstance();
}

void EncryptDecrypt::GetAESIv(uint8_t iv[], int ivLen)
{
    uint8_t buffer[PC_ENCRYPT_LEN] = {0};
    if (iv == nullptr || ivLen < AES_KEY_SIZE) {
        CLOGE("iv is null or ivLen error");
        return;
    }
    errno_t ret = memset_s(iv, ivLen, 0, ivLen);
    if (ret != 0) {
        CLOGE("memset_s failed");
        return;
    }
    if (ivLen > PC_ENCRYPT_LEN) {
        CLOGE("invalid ivLen: %d", ivLen);
        return;
    }
    RAND_bytes(buffer, ivLen);
    ret = memcpy_s(iv, ivLen, buffer, ivLen);
    if (ret != 0) {
        CLOGE("memcpy_s failed");
        return;
    }
    return;
}

int EncryptDecrypt::AES128Encry(ConstPacketData inputData, PacketData &outputData, ConstPacketData sessionKey,
    ConstPacketData sessionIV)
{
    const uint8_t *key = sessionKey.data;
    const uint8_t *iv = sessionIV.data;
    if ((inputData.data == nullptr) || (outputData.data == nullptr) || (key == nullptr) || (iv == nullptr)
        || (sessionKey.length != AES_KEY_LEN) || (sessionIV.length != AES_IV_LEN)) {
        return ERR_R_PASSED_NULL_PARAMETER;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        return ERR_R_MALLOC_FAILURE;
    }

    int ret = EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), nullptr, key, iv);
    if (ret != 1) {
        EVP_CIPHER_CTX_free(ctx);
        ctx = nullptr;
        return ERR_R_INTERNAL_ERROR;
    }

    do {
        int resultLen1 = 0;
        int resultLen2 = 0;
        EVP_CIPHER_CTX_set_key_length(ctx, AES_KEY_LEN_128);

        ret = EVP_EncryptUpdate(ctx, outputData.data, &resultLen1, inputData.data, inputData.length);
        if (ret != 1) {
            ret = SEC_ERR_ENCRYPTUPDATE_FAIL;
            break;
        }

        ret = EVP_EncryptFinal_ex(ctx, outputData.data + resultLen1, &resultLen2);
        if (ret != 1) {
            ret = SEC_ERR_ENCRYPTFINAL_FAIL;
            break;
        }
        outputData.length = resultLen1 + resultLen2;
        ret = 0;
    } while (0);

    EVP_CIPHER_CTX_free(ctx);
    ctx = nullptr;
    return ret;
}

int EncryptDecrypt::AES128Decrypt(ConstPacketData inputData, PacketData &outputData, ConstPacketData sessionKey,
    ConstPacketData sessionIV)
{
    const uint8_t *key = sessionKey.data;
    const uint8_t *iv = sessionIV.data;
    if ((outputData.data == nullptr) || (inputData.data == nullptr) || (key == nullptr) || (iv == nullptr)
        || (sessionKey.length != AES_KEY_LEN) || (sessionIV.length != AES_IV_LEN)) {
        return ERR_R_PASSED_NULL_PARAMETER;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        return ERR_R_MALLOC_FAILURE;
    }

    int ret = EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), nullptr, key, iv);
    if (ret != 1) {
        EVP_CIPHER_CTX_free(ctx);
        ctx = nullptr;
        return ERR_R_INTERNAL_ERROR;
    }

    do {
        int resultLen1 = 0;
        int resultLen2 = 0;
        EVP_CIPHER_CTX_set_key_length(ctx, AES_KEY_LEN_128);

        ret = EVP_DecryptUpdate(ctx, outputData.data, &resultLen1, inputData.data, inputData.length);
        if (ret != 1) {
            ret = SEC_ERR_ENCRYPTUPDATE_FAIL;
            break;
        }

        ret = EVP_DecryptFinal_ex(ctx, outputData.data + resultLen1, &resultLen2);
        if (ret != 1) {
            ret = SEC_ERR_ENCRYPTFINAL_FAIL;
            break;
        }
        outputData.length = resultLen1 + resultLen2;
        ret = 0;
    } while (0);

    EVP_CIPHER_CTX_free(ctx);
    ctx = nullptr;
    return ret;
}

int EncryptDecrypt::AES128GCMCheckEncryPara(ConstPacketData inputData, PacketData &outputData, EncryptInfo &encryInfo)
{
    if (encryInfo.key.length != AES_KEY_LEN_128) {
        return SEC_ERR_INVALID_KEY_LEN;
    }
    if (encryInfo.key.data == nullptr) {
        return SEC_ERR_INVALID_KEY;
    }
    if (inputData.data == nullptr || inputData.length <= 0) {
        return SEC_ERR_INVALID_PLAIN;
    }
    if (encryInfo.iv.length < AES_GCM_MIN_IVLEN) {
        return SEC_ERR_INVALID_IV_LEN;
    }
    if (encryInfo.iv.data == nullptr) {
        return SEC_ERR_INVALID_IV;
    }
    if (outputData.length < inputData.length || outputData.length <= AES_GCM_SIV_TAG_LEN) {
        return SEC_ERR_INVALID_DATA_LEN;
    }
    if (outputData.data == nullptr || outputData.length <= 0) {
        return SEC_ERR_INVALID_CIPHERTEXT;
    }
    return 0;
}

int EncryptDecrypt::EnctyptProcess(ConstPacketData inputData, PacketData &outputData, EncryptInfo &encryInfo)
{
    int ret = 0;
    int len = 0;
    int cipherTextLen = 0;
    EVP_CIPHER_CTX *ctx = nullptr;

    // Enctypt
    do {
        /* Create and initialise the context */
        ctx = EVP_CIPHER_CTX_new();
        if (ctx == nullptr) {
            return SEC_ERR_CREATECIPHER_FAIL;
        }

        /* Initialise the encryption operation. */
        if (EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) != 1) {
            ret = SEC_ERR_INVALID_CID;
            break;
        }

        /* Set IV length if default 12 bytes (96 bits) is not appropriate */
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, encryInfo.iv.length, nullptr) != 1) {
            ret = SEC_ERR_INVALID_IV_LEN;
            break;
        }

        if (!EVP_CIPHER_CTX_set_key_length(ctx, encryInfo.key.length)) {
            ret = SEC_ERR_INVALID_KEY_LEN;
            break;
        }

        /* Initialise key and IV */
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, encryInfo.key.data, encryInfo.iv.data) != 1) {
            ret = SEC_ERR_INVALID_KEY;
            break;
        }

        /*
         * Provide the message to be encrypted, and obtain the encrypted output.
         * EVP_EncryptUpdate can be called multiple times if necessary, like in file encryption,
         * you may read multiple times from a file , each time you read part of the file and call EVP_EncryptUpdate
         * on them one by one to get the whole cipherText of the file at last.
         */
        if (EVP_EncryptUpdate(ctx, outputData.data, &len, inputData.data, inputData.length) != 1) {
            ret = SEC_ERR_ENCRYPTUPDATE_FAIL;
            break;
        }

        cipherTextLen = len;

        /*
         * Finalise the encryption. Normally cipherText bytes may be written at
         * this stage, but this does not occur in GCM mode
         */
        if (EVP_EncryptFinal_ex(ctx, outputData.data + len, &len) != 1) {
            ret = SEC_ERR_ENCRYPTFINAL_FAIL;
            break;
        }
        cipherTextLen += len;

        if (cipherTextLen + AES_GCM_SIV_TAG_LEN < outputData.length) {
            ret = SEC_ERR_INVALID_DATA_LEN;
            break;
        }

        /* Get the tag */
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_GCM_SIV_TAG_LEN, outputData.data + cipherTextLen) != 1) {
            ret = SEC_ERR_GCMGETTAG_FAIL;
            break;
        }

        outputData.length = cipherTextLen + AES_GCM_SIV_TAG_LEN;
    } while (0);
    EVP_CIPHER_CTX_free(ctx);
    ctx = nullptr;
    return ret;
}

int EncryptDecrypt::AES128GCMEncry(ConstPacketData inputData, PacketData &outputData, EncryptInfo &encryInfo)
{
    int ret = AES128GCMCheckEncryPara(inputData, outputData, encryInfo);
    if (ret != 0) {
        return ret;
    }

    ret = EnctyptProcess(inputData, outputData, encryInfo);

    return ret;
}

int EncryptDecrypt::AES128GCMCheckDecryptPara(ConstPacketData inputData, PacketData &outputData, EncryptInfo &encryInfo)
{
    if (encryInfo.key.length != AES_KEY_LEN_128) {
        return SEC_ERR_INVALID_KEY_LEN;
    }
    if (encryInfo.key.data == nullptr) {
        return SEC_ERR_INVALID_KEY;
    }
    if (encryInfo.iv.length < AES_GCM_MIN_IVLEN) {
        return SEC_ERR_INVALID_IV_LEN;
    }
    if (encryInfo.iv.data == nullptr) {
        return SEC_ERR_INVALID_IV;
    }
    if (encryInfo.tag.data == nullptr) {
        return SEC_ERR_NULL_PTR;
    }
    if (encryInfo.tag.length < AES_GCM_SIV_TAG_LEN) {
        return SEC_ERR_INVALID_DATA_LEN;
    }
    if (inputData.data == nullptr || inputData.length <= 0) {
        return SEC_ERR_INVALID_CIPHERTEXT;
    }
    if (outputData.length < inputData.length) {
        return SEC_ERR_INVALID_DATA_LEN;
    }
    if (outputData.data == nullptr || outputData.length <= 0) {
        return SEC_ERR_INVALID_PLAIN;
    }
    return 0;
}

int EncryptDecrypt::DecryptProcess(ConstPacketData inputData, PacketData &outputData, EncryptInfo &encryInfo)
{
    int ret = 0;
    int len = 0;
    int plainTextLen = 0;
    EVP_CIPHER_CTX *ctx = nullptr;

    do {
        /* Create and initialise the context */
        ctx = EVP_CIPHER_CTX_new();
        if (ctx == nullptr) {
            return SEC_ERR_CREATECIPHER_FAIL;
        }

        /* Initialise the decryption operation. */
        if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr)) {
            ret = SEC_ERR_INVALID_CID;
            break;
        }

        /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, encryInfo.iv.length, nullptr)) {
            ret = SEC_ERR_INVALID_IV_LEN;
            break;
        }

        if (!EVP_CIPHER_CTX_set_key_length(ctx, encryInfo.key.length)) {
            CLOGE("key length does not match key algorithm");
            ret = SEC_ERR_INVALID_KEY_LEN;
            break;
        }

        /* Initialise key and IV */
        if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, encryInfo.key.data, encryInfo.iv.data)) {
            ret = SEC_ERR_INVALID_KEY;
            break;
        }

        /*
         * Provide the message to be decrypted, and obtain the plainText output.
         * EVP_DecryptUpdate can be called multiple times if necessary
         */
        if (!EVP_DecryptUpdate(ctx, outputData.data, &len, inputData.data, inputData.length)) {
            ret = SEC_ERR_ENCRYPTUPDATE_FAIL;
            break;
        }
        plainTextLen = len;

        /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, encryInfo.tag.length, encryInfo.tag.data)) {
            ret = SEC_ERR_GCMGETTAG_FAIL;
            break;
        }

        /*
         * Finalise the decryption. A positive return value indicates success,
         * anything else is a failure - the plainText is not trustworthy.
         */
        if (EVP_DecryptFinal_ex(ctx, outputData.data + len, &len) != 1) {
            ret = SEC_ERR_ENCRYPTFINAL_FAIL;
            break;
        }
        plainTextLen += len;
        outputData.length = plainTextLen;
    } while (0);

    EVP_CIPHER_CTX_free(ctx);
    ctx = nullptr;

    return ret;
}

int EncryptDecrypt::AES128GCMDecrypt(ConstPacketData inputData, PacketData &outputData, EncryptInfo &encryInfo)
{
    int ret = AES128GCMCheckDecryptPara(inputData, outputData, encryInfo);
    if (ret != 0) {
        return ret;
    }

    ret = DecryptProcess(inputData, outputData, encryInfo);

    return ret;
}

std::unique_ptr<uint8_t[]> EncryptDecrypt::EncryptData(int algCode, ConstPacketData sessionKey,
    ConstPacketData inputData, int &outLen)
{
    if (algCode != CTR_CODE && algCode != GCM_CODE) {
        CLOGE("encrypt not CTR for extension");
        return nullptr;
    }
    uint8_t sessionIV[AES_IV_LEN] = {0};
    GetAESIv(sessionIV, AES_IV_LEN);

    int encryptDataLen = inputData.length + AES_IV_LEN;
    if (algCode == GCM_CODE) {
        encryptDataLen += AES_GCM_SIV_TAG_LEN;
    }
    std::unique_ptr<uint8_t[]> encryptData = std::make_unique<uint8_t[]>(encryptDataLen);
    if (encryptData == nullptr) {
        return nullptr;
    }
    errno_t ret = memset_s(encryptData.get(), encryptDataLen, 0, encryptDataLen);
    if (ret != 0) {
        return nullptr;
    }
    PacketData output = { encryptData.get() + AES_IV_LEN, encryptDataLen - AES_IV_LEN };
    ConstPacketData iv = { sessionIV, AES_IV_LEN };
    EncryptInfo encryInfo;
    if (algCode == GCM_CODE) {
        encryInfo.key = sessionKey;
        encryInfo.iv = iv;
        ret = AES128GCMEncry(inputData, output, encryInfo);
    } else {
        ret = AES128Encry(inputData, output, sessionKey, iv);
    }
    if (ret != 0) {
        CLOGE("encrypt error enLen [%u][%u]", ret, output.length);
        return nullptr;
    }
    ret = memcpy_s(encryptData.get(), AES_IV_LEN, sessionIV, AES_IV_LEN);
    if (ret != 0) {
        return nullptr;
    }

    outLen = output.length + AES_IV_LEN;
    return encryptData;
}

std::unique_ptr<uint8_t[]> EncryptDecrypt::DecryptData(int algCode, ConstPacketData sessionKey,
    ConstPacketData inputData, int &outLen)
{
    uint8_t sessionIV[AES_IV_LEN] = {0};

    if (algCode != CTR_CODE && algCode != GCM_CODE) {
        CLOGE("decrypt not CTR for extension");
        return nullptr;
    }
    int minLength = (algCode == GCM_CODE) ? (AES_IV_LEN + AES_GCM_SIV_TAG_LEN) : AES_IV_LEN;
    if (inputData.length <= minLength || inputData.data == nullptr) {
        CLOGE("decrypt para error, length:%{public}d", inputData.length);
        return nullptr;
    }
    int32_t ret = memcpy_s(sessionIV, AES_IV_LEN, inputData.data, AES_KEY_SIZE);
    if (ret != 0) {
        CLOGE("memcpy_s failed");
        return nullptr;
    }
    int deLen = inputData.length - AES_IV_LEN;
    deLen = (algCode == GCM_CODE) ? (deLen - AES_GCM_SIV_TAG_LEN) : deLen;
    std::unique_ptr<uint8_t[]> decryptData = std::make_unique<uint8_t[]>(deLen);
    if (decryptData == nullptr) {
        CLOGE("create decrypt data memory failed");
        return nullptr;
    }
    ret = memset_s(decryptData.get(), deLen, 0, deLen);
    if (ret != 0) {
        CLOGE("memset_s failed");
        return nullptr;
    }
    PacketData output = { decryptData.get(), deLen };
    ConstPacketData iv = { sessionIV, AES_IV_LEN };
    ConstPacketData input = { inputData.data + AES_KEY_SIZE, deLen };
    if (algCode == GCM_CODE) {
        EncryptInfo encryInfo;
        encryInfo.key = sessionKey;
        encryInfo.iv = iv;
        encryInfo.tag = { const_cast<uint8_t *>(input.data + input.length), AES_GCM_SIV_TAG_LEN };
        ret = AES128GCMDecrypt(input, output, encryInfo);
    } else {
        ret = AES128Decrypt(input, output, sessionKey, iv);
    }
    if (ret != 0 || output.length != deLen) {
        CLOGE("decrypt error and ret[%{public}d] Len[%u] delen[%{public}d]", ret, output.length, deLen);
        return nullptr;
    }
    outLen = output.length;

    return decryptData;
}

std::string EncryptDecrypt::GetEncryptInfo()
{
    return CIPHER_AES_CTR_128;
}

int EncryptDecrypt::GetMediaEncryptCipher(const std::set<std::string> &cipherList)
{
    if (cipherList.count(CIPHER_AES_CTR_128) != 0) {
        return CTR_CODE;
    }
    CLOGE("not support the cipherlist");

    return INVALID_CODE;
}

int EncryptDecrypt::GetControlEncryptCipher(const std::set<std::string> &cipherList)
{
    // GCM is preferred, followed by CTR
    if (cipherList.count(CIPHER_AES_GCM_128) != 0) {
        return GCM_CODE;
    }

    if (cipherList.count(CIPHER_AES_CTR_128) != 0) {
        return CTR_CODE;
    }
    CLOGE("not support the cipherlist");
    return INVALID_CODE;
}


int EncryptDecrypt::GetVersion()
{
    return VERSION;
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
