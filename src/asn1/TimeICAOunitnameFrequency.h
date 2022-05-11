/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_TimeICAOunitnameFrequency_H_
#define	_TimeICAOunitnameFrequency_H_


#include <asn_application.h>

/* Including external dependencies */
#include "ASNTime.h"
#include "ICAOunitname.h"
#include "Frequency.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TimeICAOunitnameFrequency */
typedef struct TimeICAOunitnameFrequency {
	Time_t	 time;
	ICAOunitname_t	 iCAOunitname;
	Frequency_t	 frequency;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} TimeICAOunitnameFrequency_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_TimeICAOunitnameFrequency;

#ifdef __cplusplus
}
#endif

#endif	/* _TimeICAOunitnameFrequency_H_ */
#include <asn_internal.h>
