/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_TofromPosition_H_
#define	_TofromPosition_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Tofrom.h"
#include "Position.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TofromPosition */
typedef struct TofromPosition {
	Tofrom_t	 tofrom;
	Position_t	 position;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} TofromPosition_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_TofromPosition;

#ifdef __cplusplus
}
#endif

#endif	/* _TofromPosition_H_ */
#include <asn_internal.h>