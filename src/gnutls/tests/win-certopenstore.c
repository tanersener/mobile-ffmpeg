/*
 * Copyright (C) 2018 Hugo Beauzée-Luyssen
 *
 * Author: Hugo Beauzée-Luyssen
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* 
 * This test verifies the assumptions about CertOpenStore and 
 * CertOpenSystemStore to be equivalent when passed some specific flags
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _WIN32
#error "This test shouldn't have been included"
#endif

#include <windows.h>
#include <wincrypt.h>
#include <assert.h>

void doit(void)
{
    HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER , L"ROOT");
    assert(hStore != NULL);
    HCERTSTORE hSystemStore = CertOpenSystemStore(0, "ROOT");
    assert(hSystemStore != NULL);
    
    PCCERT_CONTEXT prevCtx = NULL;
    PCCERT_CONTEXT ctx = NULL;
    PCCERT_CONTEXT sysPrevCtx = NULL;
    PCCERT_CONTEXT sysCtx = NULL;

    while (1)
    {
        ctx = CertEnumCertificatesInStore(hStore, prevCtx);
        sysCtx = CertEnumCertificatesInStore(hSystemStore, sysPrevCtx);
        if (ctx == NULL || sysCtx == NULL)
            break;
        if (CertCompareIntegerBlob(&ctx->pCertInfo->SerialNumber,
                                   &sysCtx->pCertInfo->SerialNumber) != TRUE)
            assert(0);

        prevCtx = ctx;
        sysPrevCtx = sysCtx;
    }
    assert(ctx == NULL && sysCtx == NULL);

    CertCloseStore(hStore, 0);
    CertCloseStore(hSystemStore, 0);
}

