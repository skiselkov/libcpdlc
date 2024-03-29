/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Distanceoffset_H_
#define	_Distanceoffset_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Distanceoffsetnm.h"
#include "Distanceoffsetkm.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum Distanceoffset_PR {
	Distanceoffset_PR_NOTHING,	/* No components present */
	Distanceoffset_PR_distanceoffsetnm,
	Distanceoffset_PR_distanceoffsetkm
} Distanceoffset_PR;

/* Distanceoffset */
typedef struct Distanceoffset {
	Distanceoffset_PR present;
	union Distanceoffset_u {
		Distanceoffsetnm_t	 distanceoffsetnm;
		Distanceoffsetkm_t	 distanceoffsetkm;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Distanceoffset_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Distanceoffset;

#ifdef __cplusplus
}
#endif

#endif	/* _Distanceoffset_H_ */
#include <asn_internal.h>
