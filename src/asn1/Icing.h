/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Icing_H_
#define	_Icing_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum Icing {
	Icing_trace	= 0,
	Icing_light	= 1,
	Icing_moderate	= 2,
	Icing_severe	= 3
} e_Icing;

/* Icing */
typedef long	 Icing_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Icing;
asn_struct_free_f Icing_free;
asn_struct_print_f Icing_print;
asn_constr_check_f Icing_constraint;
ber_type_decoder_f Icing_decode_ber;
der_type_encoder_f Icing_encode_der;
xer_type_decoder_f Icing_decode_xer;
xer_type_encoder_f Icing_encode_xer;
per_type_decoder_f Icing_decode_uper;
per_type_encoder_f Icing_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _Icing_H_ */
#include <asn_internal.h>
