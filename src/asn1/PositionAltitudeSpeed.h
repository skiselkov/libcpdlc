/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_PositionAltitudeSpeed_H_
#define	_PositionAltitudeSpeed_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Position.h"
#include "Altitude.h"
#include "Speed.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PositionAltitudeSpeed */
typedef struct PositionAltitudeSpeed {
	Position_t	 position;
	Altitude_t	 altitude;
	Speed_t	 speed;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} PositionAltitudeSpeed_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_PositionAltitudeSpeed;

#ifdef __cplusplus
}
#endif

#endif	/* _PositionAltitudeSpeed_H_ */
#include <asn_internal.h>
