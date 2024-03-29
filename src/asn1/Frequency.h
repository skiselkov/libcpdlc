/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Frequency_H_
#define	_Frequency_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Frequencyhf.h"
#include "Frequencyvhf.h"
#include "Frequencyuhf.h"
#include "Frequencysatchannel.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum Frequency_PR {
	Frequency_PR_NOTHING,	/* No components present */
	Frequency_PR_frequencyhf,
	Frequency_PR_frequencyvhf,
	Frequency_PR_frequencyuhf,
	Frequency_PR_frequencysatchannel
} Frequency_PR;

/* Frequency */
typedef struct Frequency {
	Frequency_PR present;
	union Frequency_u {
		Frequencyhf_t	 frequencyhf;
		Frequencyvhf_t	 frequencyvhf;
		Frequencyuhf_t	 frequencyuhf;
		Frequencysatchannel_t	 frequencysatchannel;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Frequency_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Frequency;

#ifdef __cplusplus
}
#endif

#endif	/* _Frequency_H_ */
#include <asn_internal.h>
