static int
aes_gcm_aead_encrypt(void *ctx,
			const void *nonce, size_t nonce_size,
			const void *auth, size_t auth_size,
			size_t tag_size,
			const void *plain, size_t plain_size,
		   	void *encr, size_t encr_size)
{
	/* proper AEAD cipher */
	if (encr_size < plain_size + tag_size)
		return gnutls_assert_val(GNUTLS_E_SHORT_MEMORY_BUFFER);

	aes_gcm_setiv(ctx, nonce, nonce_size);
	aes_gcm_auth(ctx, auth, auth_size);

	aes_gcm_encrypt(ctx, plain, plain_size, encr, encr_size);

	aes_gcm_tag(ctx, ((uint8_t*)encr) + plain_size, tag_size);
	return 0;
}

static int
aes_gcm_aead_decrypt(void *ctx,
			const void *nonce, size_t nonce_size,
			const void *auth, size_t auth_size,
			size_t tag_size,
		   	const void *encr, size_t encr_size,
			void *plain, size_t plain_size)
{
	uint8_t tag[MAX_HASH_SIZE];

	if (encr_size < tag_size)
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	aes_gcm_setiv(ctx, nonce, nonce_size);
	aes_gcm_auth(ctx, auth, auth_size);

	encr_size -= tag_size;
	aes_gcm_decrypt(ctx, encr, encr_size, plain, plain_size);

	aes_gcm_tag(ctx, tag, tag_size);

	if (gnutls_memcmp(((uint8_t*)encr)+encr_size, tag, tag_size) != 0)
		return gnutls_assert_val(GNUTLS_E_DECRYPTION_FAILED);

	return 0;
}

