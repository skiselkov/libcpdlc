/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_DM11PositionAltitude_H_
#define	_DM11PositionAltitude_H_


#include <asn_application.h>

/* Including external dependencies */
#include "PositionAltitude.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DM11PositionAltitude */
typedef PositionAltitude_t	 DM11PositionAltitude_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_DM11PositionAltitude;
asn_struct_free_f DM11PositionAltitude_free;
asn_struct_print_f DM11PositionAltitude_print;
asn_constr_check_f DM11PositionAltitude_constraint;
ber_type_decoder_f DM11PositionAltitude_decode_ber;
der_type_encoder_f DM11PositionAltitude_encode_der;
xer_type_decoder_f DM11PositionAltitude_decode_xer;
xer_type_encoder_f DM11PositionAltitude_encode_xer;
per_type_decoder_f DM11PositionAltitude_decode_uper;
per_type_encoder_f DM11PositionAltitude_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _DM11PositionAltitude_H_ */
#include <asn_internal.h>