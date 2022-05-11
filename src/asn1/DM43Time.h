/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_DM43Time_H_
#define	_DM43Time_H_


#include <asn_application.h>

/* Including external dependencies */
#include "ASNTime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DM43Time */
typedef Time_t	 DM43Time_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_DM43Time;
asn_struct_free_f DM43Time_free;
asn_struct_print_f DM43Time_print;
asn_constr_check_f DM43Time_constraint;
ber_type_decoder_f DM43Time_decode_ber;
der_type_encoder_f DM43Time_encode_der;
xer_type_decoder_f DM43Time_decode_xer;
xer_type_encoder_f DM43Time_encode_xer;
per_type_decoder_f DM43Time_decode_uper;
per_type_encoder_f DM43Time_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _DM43Time_H_ */
#include <asn_internal.h>
