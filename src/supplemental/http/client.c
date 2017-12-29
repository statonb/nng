//
// Copyright 2017 Staysail Systems, Inc. <info@staysail.tech>
// Copyright 2017 Capitar IT Group BV <info@capitar.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core/nng_impl.h"
#include "supplemental/tls/tls.h"

#include "http.h"

struct nni_http_client {
	nng_sockaddr     addr;
	nni_list         aios;
	nni_mtx          mtx;
	bool             closed;
	nng_tls_config * tls;
	nni_aio *        connaio;
	nni_plat_tcp_ep *tep;
};

static void
http_conn_start(nni_http_client *c)
{
	nni_plat_tcp_ep_connect(c->tep, c->connaio);
}

static void
http_conn_done(void *arg)
{
	nni_http_client *  c = arg;
	nni_aio *          aio;
	int                rv;
	nni_plat_tcp_pipe *p;
	nni_http *         http;

	nni_mtx_lock(&c->mtx);
	rv = nni_aio_result(c->connaio);
	p  = rv == 0 ? nni_aio_get_pipe(c->connaio) : NULL;
	if ((aio = nni_list_first(&c->aios)) == NULL) {
		if (p != NULL) {
			nni_plat_tcp_pipe_fini(p);
		}
		nni_mtx_unlock(&c->mtx);
		return;
	}
	nni_aio_list_remove(aio);

	if (rv != 0) {
		nni_aio_finish_error(aio, rv);
		nni_mtx_unlock(&c->mtx);
		return;
	}

	if (c->tls != NULL) {
		rv = nni_http_init_tls(&http, c->tls, p);
	} else {
		rv = nni_http_init_tcp(&http, p);
	}
	if (rv != 0) {
		nni_aio_finish_error(aio, rv);
		nni_mtx_unlock(&c->mtx);
		return;
	}

	nni_aio_set_output(aio, 0, http);
	nni_aio_finish(aio, 0, 0);

	if (!nni_list_empty(&c->aios)) {
		http_conn_start(c);
	}
	nni_mtx_unlock(&c->mtx);
}

void
nni_http_client_fini(nni_http_client *c)
{
	nni_aio_fini(c->connaio);
	nni_plat_tcp_ep_fini(c->tep);
	nni_mtx_fini(&c->mtx);
#ifdef NNG_SUPP_TLS
	if (c->tls != NULL) {
		nng_tls_config_fini(c->tls);
	}
#endif
	NNI_FREE_STRUCT(c);
}

int
nni_http_client_init(nni_http_client **cp, nng_sockaddr *sa)
{
	int rv;

	nni_http_client *c;
	if ((c = NNI_ALLOC_STRUCT(c)) == NULL) {
		return (NNG_ENOMEM);
	}
	c->addr = *sa;
	rv = nni_plat_tcp_ep_init(&c->tep, NULL, &c->addr, NNI_EP_MODE_DIAL);
	if (rv != 0) {
		NNI_FREE_STRUCT(c);
		return (rv);
	}
	nni_mtx_init(&c->mtx);
	nni_aio_list_init(&c->aios);

	if ((rv = nni_aio_init(&c->connaio, http_conn_done, c)) != 0) {
		nni_http_client_fini(c);
		return (rv);
	}
	*cp = c;
	return (0);
}

#ifdef NNG_SUPP_TLS
int
nni_http_client_set_tls(nni_http_client *c, nng_tls_config *tls)
{
	nng_tls_config *old;
	nni_mtx_lock(&c->mtx);
	old    = c->tls;
	c->tls = tls;
	if (tls != NULL) {
		nni_tls_config_hold(tls);
	}
	nni_mtx_unlock(&c->mtx);
	if (old != NULL) {
		nng_tls_config_fini(old);
	}
	return (0);
}
#endif

static void
http_connect_cancel(nni_aio *aio, int rv)
{
	nni_http_client *c = aio->a_prov_data;
	nni_mtx_lock(&c->mtx);
	if (nni_aio_list_active(aio)) {
		nni_aio_list_remove(aio);
		nni_aio_finish_error(aio, rv);
	}
	if (nni_list_empty(&c->aios)) {
		nni_aio_cancel(c->connaio, rv);
	}
	nni_mtx_unlock(&c->mtx);
}

void
nni_http_client_connect(nni_http_client *c, nni_aio *aio)
{
	if (nni_aio_start(aio, http_connect_cancel, aio) != 0) {
		return;
	}
	nni_mtx_lock(&c->mtx);
	nni_list_append(&c->aios, aio);
	if (nni_list_first(&c->aios) == aio) {
		http_conn_start(c);
	}
	nni_mtx_unlock(&c->mtx);
}