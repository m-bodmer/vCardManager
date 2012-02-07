/********
vcftool.h -- header file for vcftool.c in Asmt 2
Last updated:  February 04, 2011 10:32:16 AM       

Marc Bodmer
0657005
********/
#ifndef VCFTOOL_H_
#define VCFTOOL_H_ A2

#include "vcutil.h"

int vcfInfo( FILE *const outfile, const VcFile *filep );
int vcfCanon( VcFile *const filep );
int vcfSelect( VcFile *const filep, const char *which );
int vcfSort( VcFile *const filep );

int vcfCanProp( VcProp *const propp );

#endif
