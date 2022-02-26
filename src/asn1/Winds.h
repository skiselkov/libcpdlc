/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Winds_H_
#define	_Winds_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Winddirection.h"
#include "Windspeed.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Winds */
typedef struct Winds {
	Winddirection_t	 winddirection;
	Windspeed_t	 windspeed;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Winds_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Winds;

#ifdef __cplusplus
}
#endif

#endif	/* _Winds_H_ */
#include <asn_internal.h>
