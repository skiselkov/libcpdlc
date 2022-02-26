/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_ATWDistancetolerance_H_
#define	_ATWDistancetolerance_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum ATWDistancetolerance {
	ATWDistancetolerance_plus	= 0,
	ATWDistancetolerance_minus	= 1
} e_ATWDistancetolerance;

/* ATWDistancetolerance */
typedef long	 ATWDistancetolerance_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ATWDistancetolerance;
asn_struct_free_f ATWDistancetolerance_free;
asn_struct_print_f ATWDistancetolerance_print;
asn_constr_check_f ATWDistancetolerance_constraint;
ber_type_decoder_f ATWDistancetolerance_decode_ber;
der_type_encoder_f ATWDistancetolerance_encode_der;
xer_type_decoder_f ATWDistancetolerance_decode_xer;
xer_type_encoder_f ATWDistancetolerance_encode_xer;
per_type_decoder_f ATWDistancetolerance_decode_uper;
per_type_encoder_f ATWDistancetolerance_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _ATWDistancetolerance_H_ */
#include <asn_internal.h>
