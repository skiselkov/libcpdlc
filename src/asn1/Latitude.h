/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Latitude_H_
#define	_Latitude_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Latitudedegrees.h"
#include "Minuteslatlon.h"
#include "Latitudedirection.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Latitude */
typedef struct Latitude {
	Latitudedegrees_t	 latitudedegrees;
	Minuteslatlon_t	*minuteslatlon	/* OPTIONAL */;
	Latitudedirection_t	 latitudedirection;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Latitude_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Latitude;

#ifdef __cplusplus
}
#endif

#endif	/* _Latitude_H_ */
#include <asn_internal.h>
