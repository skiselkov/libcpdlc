/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Fixnext_H_
#define	_Fixnext_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Position.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fixnext */
typedef Position_t	 Fixnext_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Fixnext;
asn_struct_free_f Fixnext_free;
asn_struct_print_f Fixnext_print;
asn_constr_check_f Fixnext_constraint;
ber_type_decoder_f Fixnext_decode_ber;
der_type_encoder_f Fixnext_encode_der;
xer_type_decoder_f Fixnext_decode_xer;
xer_type_encoder_f Fixnext_encode_xer;
per_type_decoder_f Fixnext_decode_uper;
per_type_encoder_f Fixnext_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _Fixnext_H_ */
#include <asn_internal.h>
