/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_UM21TimeAltitude_H_
#define	_UM21TimeAltitude_H_


#include <asn_application.h>

/* Including external dependencies */
#include "TimeAltitude.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UM21TimeAltitude */
typedef TimeAltitude_t	 UM21TimeAltitude_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_UM21TimeAltitude;
asn_struct_free_f UM21TimeAltitude_free;
asn_struct_print_f UM21TimeAltitude_print;
asn_constr_check_f UM21TimeAltitude_constraint;
ber_type_decoder_f UM21TimeAltitude_decode_ber;
der_type_encoder_f UM21TimeAltitude_encode_der;
xer_type_decoder_f UM21TimeAltitude_decode_xer;
xer_type_encoder_f UM21TimeAltitude_encode_xer;
per_type_decoder_f UM21TimeAltitude_decode_uper;
per_type_encoder_f UM21TimeAltitude_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _UM21TimeAltitude_H_ */
#include <asn_internal.h>