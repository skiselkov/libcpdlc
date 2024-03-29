/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_ATWdistance_H_
#define	_ATWdistance_H_


#include <asn_application.h>

/* Including external dependencies */
#include "ATWDistancetolerance.h"
#include "Distance.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ATWdistance */
typedef struct ATWdistance {
	ATWDistancetolerance_t	 aTWDistancetolerance;
	Distance_t	 distance;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ATWdistance_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ATWdistance;

#ifdef __cplusplus
}
#endif

#endif	/* _ATWdistance_H_ */
#include <asn_internal.h>
