/* @(#)s_logb.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/*
 * double logb(x)
 * IEEE 754 logb. Included to pass IEEE test suite. Not recommend.
 * Use ilogb instead.
 */

#include "fdlibm.h"

double logb(double x)
{
        int lx,ix;
        __getHI(ix,x);ix=ix&0x7fffffff;     /* high |x| */
        __getLO(lx,x);                  /* low x */
        if((ix|lx)==0) return -1.0/fabs(x);
        if(ix>=0x7ff00000) return x*x;
        if((ix>>=20)==0)                        /* IEEE 754 logb */
                return -1022.0; 
        else
                return (double) (ix-1023); 
}
