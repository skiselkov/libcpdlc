/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Verticalrate_H_
#define	_Verticalrate_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Verticalrateenglish.h"
#include "Verticalratemetric.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum Verticalrate_PR {
	Verticalrate_PR_NOTHING,	/* No components present */
	Verticalrate_PR_verticalrateenglish,
	Verticalrate_PR_verticalratemetric
} Verticalrate_PR;

/* Verticalrate */
typedef struct Verticalrate {
	Verticalrate_PR present;
	union Verticalrate_u {
		Verticalrateenglish_t	 verticalrateenglish;
		Verticalratemetric_t	 verticalratemetric;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Verticalrate_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Verticalrate;

#ifdef __cplusplus
}
#endif

#endif	/* _Verticalrate_H_ */
#include <asn_internal.h>
