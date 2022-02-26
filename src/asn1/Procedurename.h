/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Procedurename_H_
#define	_Procedurename_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Proceduretype.h"
#include "Procedure.h"
#include "Proceduretransition.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Procedurename */
typedef struct Procedurename {
	Proceduretype_t	 proceduretype;
	Procedure_t	 procedure;
	Proceduretransition_t	*proceduretransition	/* OPTIONAL */;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Procedurename_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Procedurename;

#ifdef __cplusplus
}
#endif

#endif	/* _Procedurename_H_ */
#include <asn_internal.h>
