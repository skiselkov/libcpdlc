/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_UM31AltitudeAltitude_H_
#define	_UM31AltitudeAltitude_H_


#include <asn_application.h>

/* Including external dependencies */
#include <asn_SEQUENCE_OF.h>
#include <constr_SEQUENCE_OF.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct Altitude;

/* UM31AltitudeAltitude */
typedef struct UM31AltitudeAltitude {
	A_SEQUENCE_OF(struct Altitude) list;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} UM31AltitudeAltitude_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_UM31AltitudeAltitude;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "Altitude.h"

#endif	/* _UM31AltitudeAltitude_H_ */
#include <asn_internal.h>
