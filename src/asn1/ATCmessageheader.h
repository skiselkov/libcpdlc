/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_ATCmessageheader_H_
#define	_ATCmessageheader_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Msgidentificationnumber.h"
#include "Msgreferencenumber.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct Timestamp;

/* ATCmessageheader */
typedef struct ATCmessageheader {
	Msgidentificationnumber_t	 msgidentificationnumber;
	Msgreferencenumber_t	*msgreferencenumber	/* OPTIONAL */;
	struct Timestamp	*timestamp	/* OPTIONAL */;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ATCmessageheader_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ATCmessageheader;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "Timestamp.h"

#endif	/* _ATCmessageheader_H_ */
#include <asn_internal.h>