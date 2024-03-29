/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Timeetadestination_H_
#define	_Timeetadestination_H_


#include <asn_application.h>

/* Including external dependencies */
#include "ASNTime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Timeetadestination */
typedef Time_t	 Timeetadestination_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Timeetadestination;
asn_struct_free_f Timeetadestination_free;
asn_struct_print_f Timeetadestination_print;
asn_constr_check_f Timeetadestination_constraint;
ber_type_decoder_f Timeetadestination_decode_ber;
der_type_encoder_f Timeetadestination_encode_der;
xer_type_decoder_f Timeetadestination_decode_xer;
xer_type_encoder_f Timeetadestination_encode_xer;
per_type_decoder_f Timeetadestination_decode_uper;
per_type_encoder_f Timeetadestination_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _Timeetadestination_H_ */
#include <asn_internal.h>
