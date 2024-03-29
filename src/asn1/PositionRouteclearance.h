/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_PositionRouteclearance_H_
#define	_PositionRouteclearance_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Position.h"
#include "Routeclearance.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PositionRouteclearance */
typedef struct PositionRouteclearance {
	Position_t	 position;
	Routeclearance_t	 routeclearance;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} PositionRouteclearance_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_PositionRouteclearance;

#ifdef __cplusplus
}
#endif

#endif	/* _PositionRouteclearance_H_ */
#include <asn_internal.h>
