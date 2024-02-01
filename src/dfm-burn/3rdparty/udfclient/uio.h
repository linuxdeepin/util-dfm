/* $NetBSD$ */

/*
 * File "uio.h" is part of the UDFclient toolkit.
 * File $Id: uio.h,v 1.6 2011/02/01 20:43:41 reinoud Exp $ $Name:  $
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

#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_

#include <sys/types.h>
#include <inttypes.h>


/* define an uio structure for fragmenting reading in and writing data out */
/* modelled after BSD's kernel structures */
enum uio_rw {
	UIO_READ,
	UIO_WRITE
};


struct iovec {
	void	*iov_base;	/* base if this chunk		*/
	size_t	 iov_len;	/* length			*/
};


/* this is consumed !! i.e. keep a copy */
struct uio {
	struct	iovec *uio_iov;	/* pointer to array of iovecs	*/
	int	uio_iovcnt;	/* number of iovecs in array	*/
	off_t	uio_offset;	/* current offset		*/
	size_t	uio_resid;	/* residual i/o count		*/
	enum	uio_rw uio_rw;	/* read/write direction		*/
	/* ... rest obmitted ... */
};


#endif	/* _SYS_UIO_H_ */

/* allways declare the functions */


/* move data from/to a uio structure to a blob */
extern int uiomove(void *buf, size_t amount, struct uio *uio);


/* end of uio.h */
