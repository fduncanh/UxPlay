/**
 *  Copyright (C) 2018  Juho Vähä-Herttua
 *  Copyright (C) 2020  Jaslo Ziska
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *==================================================================
 * modified by fduncanh 2021, 2023
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <openssl/sha.h> // for SHA512_DIGEST_LENGTH


#include "pairing.h"
#include "crypto.h"
#include "srp.h"

#define SALT_KEY "Pair-Verify-AES-Key"
#define SALT_IV "Pair-Verify-AES-IV"

typedef struct srp_s {
    unsigned char salt[SRP_SALT_SIZE];
    unsigned char verifier[SRP_VERIFIER_SIZE];
    unsigned char session_key[SRP_SESSION_KEY_SIZE];
    unsigned char private_key[SRP_PRIVATE_KEY_SIZE];
} srp_t;

struct pairing_s {
    ed25519_key_t *ed;
};

typedef enum {
    STATUS_INITIAL,
    STATUS_SETUP,
    STATUS_HANDSHAKE,
    STATUS_FINISHED
} status_t;

struct pairing_session_s {
    status_t status;

    ed25519_key_t *ed_ours;
    ed25519_key_t *ed_theirs;

    x25519_key_t *ecdh_ours;
    x25519_key_t *ecdh_theirs;
    unsigned char ecdh_secret[X25519_KEY_SIZE];

    char username[SRP_USERNAME_SIZE + 1]; 
    unsigned char client_pk[ED25519_KEY_SIZE];
    bool pair_setup;
  
    /* srp items */
    srp_t *srp;
};

static int
derive_key_internal(pairing_session_t *session, const unsigned char *salt, unsigned int saltlen, unsigned char *key, unsigned int keylen)
{
    unsigned char hash[SHA512_DIGEST_LENGTH];

    if (keylen > sizeof(hash)) {
        return -1;
    }

    sha_ctx_t *ctx = sha_init();
    sha_update(ctx, salt, saltlen);
    sha_update(ctx, session->ecdh_secret, X25519_KEY_SIZE);
    sha_final(ctx, hash, NULL);
    sha_destroy(ctx);

    memcpy(key, hash, keylen);
    return 0;
}

pairing_t *
pairing_init_generate(const char * keyfile)
{
    pairing_t *pairing;

    pairing = calloc(1, sizeof(pairing_t));
    if (!pairing) {
        return NULL;
    }

    pairing->ed = ed25519_key_generate(keyfile);

    return pairing;
}

void
pairing_get_public_key(pairing_t *pairing, unsigned char public_key[ED25519_KEY_SIZE])
{
    assert(pairing);
    ed25519_key_get_raw(public_key, pairing->ed);
}

int
pairing_get_ecdh_secret_key(pairing_session_t *session, unsigned char ecdh_secret[X25519_KEY_SIZE])
{
    assert(session);
    switch (session->status) {
    case STATUS_INITIAL:
        return 0;
    default:
        memcpy(ecdh_secret, session->ecdh_secret, X25519_KEY_SIZE);
        return 1;
    }
}

pairing_session_t *
pairing_session_init(pairing_t *pairing)
{
    pairing_session_t *session;

    if (!pairing) {
        return NULL;
    }

    session = calloc(1, sizeof(pairing_session_t));
    if (!session) {
        return NULL;
    }

    session->ed_ours = ed25519_key_copy(pairing->ed);

    session->status = STATUS_INITIAL;

    return session;
}

void
pairing_session_set_setup_status(pairing_session_t *session)
{
    assert(session);
    session->status = STATUS_SETUP;
}

int
pairing_session_check_handshake_status(pairing_session_t *session)
{
    assert(session);
    switch (session->status) {
    case STATUS_SETUP:
    case STATUS_HANDSHAKE:
        return 0;
    default:
        return -1;
    }
}

int
pairing_session_handshake(pairing_session_t *session, const unsigned char ecdh_key[X25519_KEY_SIZE],
                          const unsigned char ed_key[ED25519_KEY_SIZE])
{
    assert(session);

    if (session->status == STATUS_FINISHED) {
        return -1;
    }

    session->ecdh_theirs = x25519_key_from_raw(ecdh_key);
    session->ed_theirs = ed25519_key_from_raw(ed_key);

    session->ecdh_ours = x25519_key_generate();

    x25519_derive_secret(session->ecdh_secret, session->ecdh_ours, session->ecdh_theirs);

    session->status = STATUS_HANDSHAKE;
    return 0;
}

int
pairing_session_get_public_key(pairing_session_t *session, unsigned char ecdh_key[X25519_KEY_SIZE])
{
    assert(session);

    if (session->status != STATUS_HANDSHAKE) {
        return -1;
    }

    x25519_key_get_raw(ecdh_key, session->ecdh_ours);

    return 0;
}

int
pairing_session_get_signature(pairing_session_t *session, unsigned char signature[PAIRING_SIG_SIZE])
{
    unsigned char sig_msg[PAIRING_SIG_SIZE];
    unsigned char key[AES_128_BLOCK_SIZE];
    unsigned char iv[AES_128_BLOCK_SIZE];
    aes_ctx_t *aes_ctx;

    assert(session);

    if (session->status != STATUS_HANDSHAKE) {
        return -1;
    }

    /* First sign the public ECDH keys of both parties */
    x25519_key_get_raw(sig_msg, session->ecdh_ours);
    x25519_key_get_raw(sig_msg + X25519_KEY_SIZE, session->ecdh_theirs);

    ed25519_sign(signature, PAIRING_SIG_SIZE, sig_msg, PAIRING_SIG_SIZE, session->ed_ours);

    /* Then encrypt the result with keys derived from the shared secret */
    derive_key_internal(session, (const unsigned char *) SALT_KEY, strlen(SALT_KEY), key, sizeof(key));
    derive_key_internal(session, (const unsigned char *) SALT_IV, strlen(SALT_IV), iv, sizeof(iv));

    aes_ctx = aes_ctr_init(key, iv);
    aes_ctr_encrypt(aes_ctx, signature, signature, PAIRING_SIG_SIZE);
    aes_ctr_destroy(aes_ctx);

    return 0;
}

int
pairing_session_finish(pairing_session_t *session, const unsigned char signature[PAIRING_SIG_SIZE])
{
    unsigned char sig_buffer[PAIRING_SIG_SIZE];
    unsigned char sig_msg[PAIRING_SIG_SIZE];
    unsigned char key[AES_128_BLOCK_SIZE];
    unsigned char iv[AES_128_BLOCK_SIZE];
    aes_ctx_t *aes_ctx;

    assert(session);

    if (session->status != STATUS_HANDSHAKE) {
        return -1;
    }

    /* First decrypt the signature with keys derived from the shared secret */
    derive_key_internal(session, (const unsigned char *) SALT_KEY, strlen(SALT_KEY), key, sizeof(key));
    derive_key_internal(session, (const unsigned char *) SALT_IV, strlen(SALT_IV), iv, sizeof(iv));

    aes_ctx = aes_ctr_init(key, iv);
    /* One fake round for the initial handshake encryption */
    aes_ctr_encrypt(aes_ctx, sig_buffer, sig_buffer, PAIRING_SIG_SIZE);
    aes_ctr_encrypt(aes_ctx, signature, sig_buffer, PAIRING_SIG_SIZE);
    aes_ctr_destroy(aes_ctx);

    /* Then verify the signature with public ECDH keys of both parties */
    x25519_key_get_raw(sig_msg, session->ecdh_theirs);
    x25519_key_get_raw(sig_msg + X25519_KEY_SIZE, session->ecdh_ours);

    if (!ed25519_verify(sig_buffer, PAIRING_SIG_SIZE, sig_msg, PAIRING_SIG_SIZE, session->ed_theirs)) {
        return -2;
    }

    session->status = STATUS_FINISHED;
    return 0;
}

void
pairing_session_destroy(pairing_session_t *session)
{
    if (session) {
        ed25519_key_destroy(session->ed_ours);
        ed25519_key_destroy(session->ed_theirs);

        x25519_key_destroy(session->ecdh_ours);
        x25519_key_destroy(session->ecdh_theirs);
        free(session);
    }
}

void
pairing_destroy(pairing_t *pairing)
{
    if (pairing) {
        ed25519_key_destroy(pairing->ed);
        free(pairing);
    }
}

int
random_pin() {
    unsigned char random_bytes[2] = { 0 };
    unsigned short random_short = 0;
    int ret;
    /* create a random unsigned short in range 1-9999 */
    while (!random_short) {
        if ((ret = get_random_bytes(random_bytes, sizeof(random_bytes))  < 1)) {
            return -1;
        }
         memcpy(&random_short, random_bytes, sizeof(random_bytes));
         random_short = random_short % 10000;
    }
    return (int) random_short; 
}

int
srp_new_user(pairing_session_t *session, pairing_t *pairing, const char *device_id, const char *pin,
             const char **salt, int *len_salt, const char **pk, int *len_pk) {
    if (strlen(device_id) > SRP_USERNAME_SIZE) {
        return -1;
    }

    strncpy(session->username, device_id, SRP_USERNAME_SIZE);
    
    if (session->srp) {
        free (session->srp);
    }
    session->srp = (srp_t *) calloc(1, sizeof(srp_t));
    if (!session->srp) {
        return -2;
    }

    get_random_bytes(session->srp->private_key, SRP_PRIVATE_KEY_SIZE);
    
    const unsigned char *srp_b = session->srp->private_key;
    unsigned char * srp_B;
    unsigned char * srp_s;
    unsigned char * srp_v;
    int len_b = SRP_PRIVATE_KEY_SIZE;
    int len_B;
    int len_s;
    int len_v;
    srp_create_salted_verification_key(SRP_SHA, SRP_NG, device_id,
                                       (const unsigned char *) pin, strlen (pin),
                                       (const unsigned char **) &srp_s, &len_s,
                                       (const unsigned char **) &srp_v, &len_v,
                                       NULL, NULL);
    if (len_s != SRP_SALT_SIZE || len_v != SRP_VERIFIER_SIZE) {
        return -3;
    }

    memcpy(session->srp->salt, srp_s, SRP_SALT_SIZE);
    memcpy(session->srp->verifier, srp_v, SRP_VERIFIER_SIZE);

    *salt = (char *) session->srp->salt;
    *len_salt = len_s;

    srp_create_server_ephemeral_key(SRP_SHA, SRP_NG,
                                    srp_v, len_v,
                                    srp_b, len_b,
                                    (const unsigned char **) &srp_B, &len_B,
                                    NULL, NULL, 1);

    *pk = (char *) srp_B;
    *len_pk = len_B;

    return 0;
}

int
srp_validate_proof(pairing_session_t *session, pairing_t *pairing, const unsigned char *A,
                    int len_A, unsigned char *proof, int client_proof_len, int proof_len) {
    int authenticated  = 0;
    const unsigned char *B =  NULL;
    const unsigned char *b = session->srp->private_key;
    int len_b = SRP_PRIVATE_KEY_SIZE;
    int len_B = 0;
    int len_K = 0;
    const unsigned char *session_key  = NULL;
    const unsigned char *M2 = NULL;

    struct SRPVerifier *verifier = srp_verifier_new(SRP_SHA, SRP_NG, (const char *) session->username,
                                                    (const unsigned char *) session->srp->salt, SRP_SALT_SIZE,
                                                    (const unsigned char *) session->srp->verifier, SRP_VERIFIER_SIZE,
                                                    A, len_A,
                                                    b, len_b,
                                                    &B, &len_B, NULL, NULL, 1);

    srp_verifier_verify_session(verifier, proof, &M2);
    authenticated = srp_verifier_is_authenticated(verifier);
    if (authenticated == 0) {
        /* HTTP 470 should be sent to client if not verified.*/
        srp_verifier_delete(verifier);
        free (session->srp);
	session->srp = NULL;
        return -1;
    }
    session_key = srp_verifier_get_session_key(verifier, &len_K);
    if (len_K != SRP_SESSION_KEY_SIZE) {
        return -2;
    }
    memcpy(session->srp->session_key, session_key, len_K);
    memcpy(proof, M2, proof_len);
    srp_verifier_delete(verifier);
    return 0;
}
int
srp_confirm_pair_setup(pairing_session_t *session, pairing_t *pairing,
                       unsigned char *epk, unsigned char *auth_tag) {
    unsigned char aesKey[16], aesIV[16];
    unsigned char hash[SHA512_DIGEST_LENGTH];
    unsigned char pk[ED25519_KEY_SIZE];
    int pk_len_client, epk_len;

    /* decrypt client epk to get client pk, authenticate with auth_tag*/ 

    const char *salt = "Pair-Setup-AES-Key";
    sha_ctx_t *ctx = sha_init();
    sha_update(ctx, (const unsigned char *) salt, strlen(salt));
    sha_update(ctx, session->srp->session_key, SRP_SESSION_KEY_SIZE);
    sha_final(ctx, hash, NULL);
    sha_destroy(ctx);
    memcpy(aesKey, hash, 16);

    salt = "Pair-Setup-AES-IV";
    ctx = sha_init();
    sha_update(ctx, (const unsigned char *) salt, strlen(salt));
    sha_update(ctx, session->srp->session_key, SRP_SESSION_KEY_SIZE);
    sha_final(ctx, hash, NULL);
    sha_destroy(ctx);
    memcpy(aesIV, hash, 16);
    aesIV[15]++;

    /* SRP6a data is no longer needed */
    free(session->srp);

    /* decrypt client epk to authenticate client using auth_tag */
    pk_len_client  = gcm_decrypt(epk, ED25519_KEY_SIZE, pk, aesKey, aesIV, auth_tag);
    if (pk_len_client <= 0) {
       /* authentication failed */
         return pk_len_client;
    }

    /* success, from server viewpoint */
    memcpy(session->client_pk, pk, ED25519_KEY_SIZE);
    session->pair_setup = true;

    /* encrypt server epk so client can also authenticate server using auth_tag */
    pairing_get_public_key(pairing, pk);

    /* encryption  needs this previously undocumented additional "nonce" */
    aesIV[15]++;
    epk_len = gcm_encrypt(pk, ED25519_KEY_SIZE, epk, aesKey, aesIV, auth_tag);
    return epk_len;    
}

void access_client_session_data(pairing_session_t *session, char **username, unsigned char **client_pk, bool *setup) {
    *username = session->username;
    *client_pk = session->client_pk;
    setup = &(session->pair_setup);
}
