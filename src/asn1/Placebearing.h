/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Placebearing_H_
#define	_Placebearing_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Fixname.h"
#include "Degrees.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct LatitudeLongitude;

/* Placebearing */
typedef struct Placebearing {
	Fixname_t	 fixname;
	struct LatitudeLongitude	*latitudeLongitude	/* OPTIONAL */;
	Degrees_t	 degrees;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Placebearing_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Placebearing;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "LatitudeLongitude.h"

#endif	/* _Placebearing_H_ */
#include <asn_internal.h>
