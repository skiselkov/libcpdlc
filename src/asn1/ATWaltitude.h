/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_ATWaltitude_H_
#define	_ATWaltitude_H_


#include <asn_application.h>

/* Including external dependencies */
#include "ATWaltitudetolerance.h"
#include "Altitude.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ATWaltitude */
typedef struct ATWaltitude {
	ATWaltitudetolerance_t	 aTWaltitudetolerance;
	Altitude_t	 altitude;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ATWaltitude_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ATWaltitude;

#ifdef __cplusplus
}
#endif

#endif	/* _ATWaltitude_H_ */
#include <asn_internal.h>