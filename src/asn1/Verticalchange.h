/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Verticalchange_H_
#define	_Verticalchange_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Verticaldirection.h"
#include "Verticalrate.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Verticalchange */
typedef struct Verticalchange {
	Verticaldirection_t	 verticaldirection;
	Verticalrate_t	 verticalrate;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Verticalchange_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Verticalchange;

#ifdef __cplusplus
}
#endif

#endif	/* _Verticalchange_H_ */
#include <asn_internal.h>