/* $NetBSD$ */

/*
 * File "uio.c" is part of the UDFclient toolkit.
 * File $Id: uio.c,v 1.7 2011/02/01 20:43:41 reinoud Exp $ $Name:  $
 *
 * Copyright (c) 2004, 2005, 2006, 2011
 * 	Reinoud Zandijk <reinoud@netbsd.org>
 * All rights reserved.
 *
 * The UDFclient toolkit is distributed under the Clarified Artistic Licence.
 * A copy of the licence is included in the distribution as
 * `LICENCE.clearified.artistic' and a copy of the licence can also be
 * requested at the GNU foundantion's website.
 *
 * Visit the UDFclient toolkit homepage http://www.13thmonkey.org/udftoolkit/
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <assert.h>
#include <string.h>
#include "uio.h"


/* modelled after NetBSD's uiomove */
int uiomove(void *buf, size_t amount, struct uio *uio) {
	struct	iovec *iov;
	char	*cp = buf;
	size_t	cnt;

	assert(buf);
	assert(uio);
	assert(uio->uio_iov);

	while (amount > 0 && uio->uio_resid > 0) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			assert(uio->uio_iovcnt > 0);
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt == 0) return 0;	/* end of buffers */
			continue;
		}

		/* not more than requested please */
		if (cnt > amount) cnt = amount;

		/* move the information */
		if (uio->uio_rw == UIO_READ) {
			/* read into the uio	*/
			memcpy(cp, iov->iov_base, cnt);
		} else {
			/* write from the uio	*/
			memcpy(iov->iov_base, cp, cnt);
		}

		/* update the uio structure to reflect move */
		iov->iov_base	 = (caddr_t) iov->iov_base + cnt;
		iov->iov_len	-= cnt;
		uio->uio_resid  -= cnt;
		uio->uio_offset += cnt;
		cp		+= cnt;

		assert(cnt <= amount);

		amount -= cnt;
	}

	return 0;
}


/* end of uio.c */

