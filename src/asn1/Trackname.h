/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Trackname_H_
#define	_Trackname_H_


#include <asn_application.h>

/* Including external dependencies */
#include <IA5String.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Trackname */
typedef IA5String_t	 Trackname_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Trackname;
asn_struct_free_f Trackname_free;
asn_struct_print_f Trackname_print;
asn_constr_check_f Trackname_constraint;
ber_type_decoder_f Trackname_decode_ber;
der_type_encoder_f Trackname_encode_der;
xer_type_decoder_f Trackname_decode_xer;
xer_type_encoder_f Trackname_encode_xer;
per_type_decoder_f Trackname_decode_uper;
per_type_encoder_f Trackname_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _Trackname_H_ */
#include <asn_internal.h>
