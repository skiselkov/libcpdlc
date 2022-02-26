/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Tofrom_H_
#define	_Tofrom_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum Tofrom {
	Tofrom_to	= 0,
	Tofrom_from	= 1
} e_Tofrom;

/* Tofrom */
typedef long	 Tofrom_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Tofrom;
asn_struct_free_f Tofrom_free;
asn_struct_print_f Tofrom_print;
asn_constr_check_f Tofrom_constraint;
ber_type_decoder_f Tofrom_decode_ber;
der_type_encoder_f Tofrom_encode_der;
xer_type_decoder_f Tofrom_decode_xer;
xer_type_encoder_f Tofrom_encode_xer;
per_type_decoder_f Tofrom_decode_uper;
per_type_encoder_f Tofrom_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _Tofrom_H_ */
#include <asn_internal.h>
