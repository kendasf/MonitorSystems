/* $Id: bigdtypes.h $ */

/******************** SHORT COPYRIGHT NOTICE**************************
This source code is part of the BigDigits multiple-precision
arithmetic library Version 2.2 originally written by David Ireland,
copyright (c) 2001-8 D.I. Management Services Pty Limited, all rights
reserved. It is provided "as is" with no warranties. You may use
this software under the terms of the full copyright notice
"bigdigitsCopyright.txt" that should have been included with this
library or can be obtained from <www.di-mgt.com.au/bigdigits.html>.
This notice must always be retained in any copy.
******************* END OF COPYRIGHT NOTICE***************************/
/*
	Last updated:
	$Date: 2008-05-04 13:05:00 $
	$Revision: 2.2.0 $
	$Author: dai $
*/

#ifndef BIGDTYPES_H_
#define BIGDTYPES_H_ 1

#include <stddef.h>

/*
The following PP instructions assume that all Linux systems have a C99-conforming 
<stdint.h>; that other Unix systems have the uint32_t definitions in <sys/types.h>;
and that MS et al don't have them at all. This version assumes that a long is 32 bits.
Adjust if necessary to suit your system. 
You can override by defining HAVE_C99INCLUDES or HAVE_SYS_TYPES.
*/
#define NO_ALLOCS
 
#define uint32_t unsigned long 
#define uint16_t unsigned short 


#endif /* BIGDTYPES_H_ */
