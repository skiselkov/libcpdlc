/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_UM71Time_H_
#define	_UM71Time_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UM71Time */
typedef Time_t	 UM71Time_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_UM71Time;
asn_struct_free_f UM71Time_free;
asn_struct_print_f UM71Time_print;
asn_constr_check_f UM71Time_constraint;
ber_type_decoder_f UM71Time_decode_ber;
der_type_encoder_f UM71Time_encode_der;
xer_type_decoder_f UM71Time_decode_xer;
xer_type_encoder_f UM71Time_encode_xer;
per_type_decoder_f UM71Time_decode_uper;
per_type_encoder_f UM71Time_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _UM71Time_H_ */
#include <asn_internal.h>