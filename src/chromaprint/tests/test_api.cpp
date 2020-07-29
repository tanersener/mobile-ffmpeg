#include <gtest/gtest.h>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include "chromaprint.h"
#include "test_utils.h"
#include "utils/scope_exit.h"

namespace chromaprint {

TEST(API, TestFp) {
	std::vector<short> data = LoadAudioFile("data/test_stereo_44100.raw");

	ChromaprintContext *ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_TEST2);
	ASSERT_NE(nullptr, ctx);
	SCOPE_EXIT(chromaprint_free(ctx));

	ASSERT_EQ(1, chromaprint_get_num_channels(ctx));
	ASSERT_EQ(11025, chromaprint_get_sample_rate(ctx));

	ASSERT_EQ(1, chromaprint_start(ctx, 44100, 1));
	ASSERT_EQ(1, chromaprint_feed(ctx, data.data(), data.size()));

	char *fp;
	uint32_t fp_hash;

	ASSERT_EQ(1, chromaprint_finish(ctx));
	ASSERT_EQ(1, chromaprint_get_fingerprint(ctx, &fp));
	SCOPE_EXIT(chromaprint_dealloc(fp));
	ASSERT_EQ(1, chromaprint_get_fingerprint_hash(ctx, &fp_hash));

	EXPECT_EQ(std::string("AQAAC0kkZUqYREkUnFAXHk8uuMZl6EfO4zu-4ABKFGESWIIMEQE"), std::string(fp));
	ASSERT_EQ(3732003127, fp_hash);
}

TEST(API, Test2SilenceFp)
{
	short zeroes[1024];
	std::fill(zeroes, zeroes + 1024, 0);

	ChromaprintContext *ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_TEST2);
	ASSERT_NE(nullptr, ctx);
	SCOPE_EXIT(chromaprint_free(ctx));

	ASSERT_EQ(1, chromaprint_start(ctx, 44100, 1));
	for (int i = 0; i < 130; i++) {
		ASSERT_EQ(1, chromaprint_feed(ctx, zeroes, 1024));
	}

	char *fp;
	uint32_t fp_hash;

	ASSERT_EQ(1, chromaprint_finish(ctx));
	ASSERT_EQ(1, chromaprint_get_fingerprint(ctx, &fp));
	SCOPE_EXIT(chromaprint_dealloc(fp));
	ASSERT_EQ(1, chromaprint_get_fingerprint_hash(ctx, &fp_hash));

	ASSERT_EQ(18, strlen(fp));
	EXPECT_EQ(std::string("AQAAA0mUaEkSRZEGAA"), std::string(fp));
	ASSERT_EQ(627964279, fp_hash);
}

TEST(API, Test2SilenceRawFp)
{
	short zeroes[1024];
	std::fill(zeroes, zeroes + 1024, 0);

	ChromaprintContext *ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_TEST2);
	ASSERT_NE(nullptr, ctx);
	SCOPE_EXIT(chromaprint_free(ctx));

	ASSERT_EQ(1, chromaprint_start(ctx, 44100, 1));
	for (int i = 0; i < 130; i++) {
		ASSERT_EQ(1, chromaprint_feed(ctx, zeroes, 1024));
	}

	uint32_t *fp;
	int length;

	ASSERT_EQ(1, chromaprint_finish(ctx));
	ASSERT_EQ(1, chromaprint_get_raw_fingerprint(ctx, &fp, &length));
	SCOPE_EXIT(chromaprint_dealloc(fp));

	ASSERT_EQ(3, length);
	EXPECT_EQ(627964279, fp[0]);
	EXPECT_EQ(627964279, fp[1]);
	EXPECT_EQ(627964279, fp[2]);
}

TEST(API, TestEncodeFingerprint)
{
	uint32_t fingerprint[] = { 1, 0 };
	char expected[] = { 55, 0, 0, 2, 65, 0 };

	char *encoded;
	int encoded_size;
	ASSERT_EQ(1, chromaprint_encode_fingerprint(fingerprint, 2, 55, &encoded, &encoded_size, 0));
	SCOPE_EXIT(chromaprint_dealloc(encoded));

	ASSERT_EQ(6, encoded_size);
	for (int i = 0; i < encoded_size; i++) {
		ASSERT_EQ(expected[i], encoded[i]) << "Different at " << i;
	}
}

TEST(API, TestEncodeFingerprintBase64)
{
	uint32_t fingerprint[] = { 1, 0 };
	char expected[] = "NwAAAkEA";

	char *encoded;
	int encoded_size;
	ASSERT_EQ(1, chromaprint_encode_fingerprint(fingerprint, 2, 55, &encoded, &encoded_size, 1));
	SCOPE_EXIT(chromaprint_dealloc(encoded));

	ASSERT_EQ(8, encoded_size);
	ASSERT_STREQ(expected, encoded);
}

TEST(API, TestDecodeFingerprint)
{
	char data[] = { 55, 0, 0, 2, 65, 0 };

	uint32_t *fingerprint;
	int size;
	int algorithm;
	ASSERT_EQ(1, chromaprint_decode_fingerprint(data, 6, &fingerprint, &size, &algorithm, 0));
	SCOPE_EXIT(chromaprint_dealloc(fingerprint));

	ASSERT_EQ(2, size);
	ASSERT_EQ(55, algorithm);
	ASSERT_EQ(1, fingerprint[0]);
	ASSERT_EQ(0, fingerprint[1]);
}

TEST(API, TestHashFingerprint)
{
	uint32_t fingerprint[] = { 19681, 22345, 312312, 453425 };
	uint32_t hash;

	ASSERT_EQ(0, chromaprint_hash_fingerprint(NULL, 4, &hash));
	ASSERT_EQ(0, chromaprint_hash_fingerprint(fingerprint, -1, &hash));
	ASSERT_EQ(0, chromaprint_hash_fingerprint(fingerprint, 4, NULL));

	ASSERT_EQ(1, chromaprint_hash_fingerprint(fingerprint, 4, &hash));
	ASSERT_EQ(17249, hash);
}

TEST(API, TestDecodeFingerprintOutOfRange)
{
	uint32_t *fp;
	int length, algorithm;

	const char *encoded = "AQABz9GSJEkkKYmS4Tp6xvjx-tBpXMfRpUP1w3yGHmUPcWTwhnhgiYNeeBcu5HhbxA2PZuEFnSd-9FECq-gJPbtQ_fiLfkf3wvnRl9AO__jxCj7-QjzyoFek4xcmPnA2-qjUDz9yTsR56PFRJp4GP8ePLj2qY-NRfkL2oXEx6cFThPCh7_iRVnzQbX9wHc0UT0I_5Duao1KVGOsPlpdQLcqFHxf5oqfRFVpySvjR_Ch7_NCO87h05ChL7Do65vDIFC2H8zhyKnhqXJ6GdsuLw4c276h-TGEcfAfjmWgeYzy6HT6u5UhvhCceZSp0PD5iZzka9Zh05NOEZmsgxZTx50ifa8hxpRv0E1vHKciFHk0cFY0moRKzabgbDb3gH_2RH8k3lGuEKc_x3_hRPehX5ODRfDX6JCtOIVyOMFyP-4XyHWkaF9fRt0VzD7sOJlUkHX2Q9Ed-9PnR8EpwijfCJJFi9PmhLySPhmOC-sIZiI8m_ML3INJ3pI6UknCNnkHzyNDEpchz400d9IN99PiSo9kyDf0xXTrS_HCC79D64Tx4o1YSIReeCKH84Q0ORT7CNMKPmQma62inD7xoodlEJNOJXEHzo3qCMEn4omWg_UPKqBN69EYz5kPyDrkeCb2OKd2i4RJ6PB3CQ5Wo5shTMUH7Hf6IHsd5NNcQnjFGo8oP__DJQz8OJk9s5BROotERSmSOjyaUv0LqsziV4zmaK5jIFLXAxEk0xIfeIcyFqiEVXL_hGLnQUOXRJ8SHSLnRr_jx5IU35CH0N0gfHtdQ_Wj0ZniT7ENZXPDRrijZIMxn7Dkqo0nIoG_QVDsO_mi6yOibDJqyHf6DSiX2fdAyVEyCXDyqhD2aRyf2H09EMJkchE-g60GYK3grNHHwHE9I9DrSMoJ2ZMd7_Ch7WEftI24DqTrCEteio9ePLxIVIj-uW1CjK6goVoYnhYZuuElwNDfyIT4e4lGWKWhEyUF5XKic47xQ0ZygO0XXhAgTMzP-FN_x4_lQiynCxBlxPgivIEyNL0e64zu-hCR-fMzBieLxIzy0MEZ-MNcc-Bqc53gYUfCPh0c6MmFI6DWaijk-K8g_PNXRhKjTD72UBSXzgT_-YIx27EsUJ6iThkJTZUd-aM_R03iaCk-CUwITPUcPVVfwLA8m0UbTrLgSHR3-Y8-DUsKVB6EkJWh-_HgPTv3AZs_R23CcBfWRM1B1hB1K63iipLg9XETcZ0I3jynO8PiF64YtH1klhDvxhThXzFex3kG1HPZu9Dmu47mh5UGtHKp1POjyUMWfDJo6of8Gtgvy5EDfo1bgnWiX6wCGhEEAKADJIUAbILJzTAgDBDBCAgQQMEQQJABBwgJgBBMAAGaEAMwAh5wAikDkhQBGCIGkAgQASghihAmlEBDEMCOUAYYJ4pRVwhAEAiXEYWAUMchQBpQVhkwhgDUMAQAoMkgxJRUwjgFBBRBCEGM0BgBAAIhQRBJkkALCCAUMQIAIIQBQRDGDBAECAAGY4MYQIhSgBCEkKAHEggG8cAIYJIgUwJAHpCHAMAaEQEYo6IgAADgEAUIIBAQcMEIgYBkECEABCEYKAYYcA4oowBBABAAvBJJKMGIAEAYwA5QADAAGCINMaAKQJcQYjCCzxBBACAcGMM4II8IQKIghgFgABTOEUAegMQQ4wYCBSjgAiAFCEGCAYwZIIAA0QChAhBROUAQAEo4YR4AQhhhliAAMEAQUBtooAg5xjCADgCGAOCQIQgQghigwQAikBBAEAA";

	chromaprint_decode_fingerprint(encoded, strlen(encoded), &fp, &length, &algorithm, 1);
}

TEST(API, TestDecodeFingerprintEmpty)
{
	uint32_t *fp;
	int length, algorithm;

	const char *encoded = "AQAAAA";

	auto ret = chromaprint_decode_fingerprint(encoded, strlen(encoded), &fp, &length, &algorithm, 1);
	ASSERT_EQ(1, ret);
	ASSERT_EQ(0, length);
	ASSERT_EQ(1, algorithm);
}

TEST(API, TestDecodeFingerprintNull)
{
	uint32_t *fp;
	int length, algorithm;

	const char *encoded = "null";

	auto ret = chromaprint_decode_fingerprint(encoded, strlen(encoded), &fp, &length, &algorithm, 1);
	ASSERT_EQ(0, ret);
	ASSERT_EQ(0, length);
	ASSERT_EQ(0, algorithm);
}

TEST(API, TestDecodeFingerprintCoustidFingerprint)
{
	uint32_t *fp;
	int length, algorithm;

	const char *encoded = "coustid Fingerprint";

	auto ret = chromaprint_decode_fingerprint(encoded, strlen(encoded), &fp, &length, &algorithm, 1);
	ASSERT_EQ(0, ret);
	ASSERT_EQ(0, length);
	ASSERT_EQ(0, algorithm);
}

}; // namespace chromaprint
