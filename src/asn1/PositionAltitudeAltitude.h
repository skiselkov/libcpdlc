/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_PositionAltitudeAltitude_H_
#define	_PositionAltitudeAltitude_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Position.h"
#include <asn_SEQUENCE_OF.h>
#include <constr_SEQUENCE_OF.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct Altitude;

/* PositionAltitudeAltitude */
typedef struct PositionAltitudeAltitude {
	Position_t	 position;
	struct PositionAltitudeAltitude__altitude_seqOf {
		A_SEQUENCE_OF(struct Altitude) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} altitude_seqOf;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} PositionAltitudeAltitude_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_PositionAltitudeAltitude;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "Altitude.h"

#endif	/* _PositionAltitudeAltitude_H_ */
#include <asn_internal.h>
