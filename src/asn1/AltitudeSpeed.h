/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_AltitudeSpeed_H_
#define	_AltitudeSpeed_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Altitude.h"
#include "Speed.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AltitudeSpeed */
typedef struct AltitudeSpeed {
	Altitude_t	 altitude;
	Speed_t	 speed;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} AltitudeSpeed_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_AltitudeSpeed;

#ifdef __cplusplus
}
#endif

#endif	/* _AltitudeSpeed_H_ */
#include <asn_internal.h>
