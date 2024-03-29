/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Time_H_
#define	_Time_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Timehours.h"
#include "Timeminutes.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Time */
typedef struct Time {
	Timehours_t	 timehours;
	Timeminutes_t	 timeminutes;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Time_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Time;

#ifdef __cplusplus
}
#endif

#endif	/* _Time_H_ */
#include <asn_internal.h>
