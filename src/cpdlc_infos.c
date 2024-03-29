/*
 * Copyright 2022 Saso Kiselkov
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdbool.h>
#include <stddef.h>

#include "asn1/ATCdownlinkmessage.h"
#include "asn1/ATCuplinkmessage.h"

#include "cpdlc_msg.h"

#define	LONG_TIMEOUT		300	/* seconds */
#define	MED_TIMEOUT		200	/* seconds */
#define	SHORT_TIMEOUT		100	/* seconds */

#define	ASN_UPLINK_INFO_01(typename) \
	.asn_elem_id = ATCuplinkmsgelementid_PR_uM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename) }, \
	}

#define	ASN_UPLINK_INFO_2(typename, arg1, arg2) \
	.asn_elem_id = ATCuplinkmsgelementid_PR_uM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg1) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg2) } \
	}

#define	ASN_UPLINK_INFO_SEQ_2(typename, seq_name) \
	.asn_elem_id = ATCuplinkmsgelementid_PR_uM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCuplinkmsgelementid_t, \
		choice.uM ## typename.seq_name), true, 0 }, \
	    { offsetof(ATCuplinkmsgelementid_t, \
		choice.uM ## typename.seq_name), true, 1 } \
	}

#define	ASN_UPLINK_INFO_1_SEQ_2(typename, arg1, seq_name) \
	.asn_elem_id = ATCuplinkmsgelementid_PR_uM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg1) }, \
	    { offsetof(ATCuplinkmsgelementid_t, \
		choice.uM ## typename.seq_name), true, 0 }, \
	    { offsetof(ATCuplinkmsgelementid_t, \
		choice.uM ## typename.seq_name), true, 1 } \
	}

#define	ASN_UPLINK_INFO_3(typename, arg1, arg2, arg3) \
	.asn_elem_id = ATCuplinkmsgelementid_PR_uM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg1) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg2) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg3) } \
	}

#define	ASN_UPLINK_INFO_4(typename, arg1, arg2, arg3, arg4) \
	.asn_elem_id = ATCuplinkmsgelementid_PR_uM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg1) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg2) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg3) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg4) } \
	}

#define	ASN_UPLINK_INFO_5(typename, arg1, arg2, arg3, arg4, arg5) \
	.asn_elem_id = ATCuplinkmsgelementid_PR_uM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg1) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg2) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg3) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg4) }, \
	    { offsetof(ATCuplinkmsgelementid_t, choice.uM ## typename.arg5) } \
	}

#define	ASN_DOWNLINK_INFO_01(typename) \
	.asn_elem_id = ATCdownlinkmsgelementid_PR_dM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCdownlinkmsgelementid_t, choice.dM ## typename) }, \
	}

#define	ASN_DOWNLINK_INFO_2(typename, arg1, arg2) \
	.asn_elem_id = ATCdownlinkmsgelementid_PR_dM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.arg1) }, \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.arg2) } \
	}

#define	ASN_DOWNLINK_INFO_SEQ_2(typename, seq_name) \
	.asn_elem_id = ATCdownlinkmsgelementid_PR_dM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.seq_name), true, 0 }, \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.seq_name), true, 1 } \
	}

#define	ASN_DOWNLINK_INFO_3(typename, arg1, arg2, arg3) \
	.asn_elem_id = ATCdownlinkmsgelementid_PR_dM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.arg1) }, \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.arg2) }, \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.arg3) } \
	}

#define	ASN_DOWNLINK_INFO_4(typename, arg1, arg2, arg3, arg4) \
	.asn_elem_id = ATCdownlinkmsgelementid_PR_dM ## typename, \
	.asn_arg_info = { \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.arg1) }, \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.arg2) }, \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.arg3) }, \
	    { offsetof(ATCdownlinkmsgelementid_t, \
		choice.dM ## typename.arg4) } \
	}

static const cpdlc_msg_info_t ul_infos[] = {
    {
	.msg_type = CPDLC_UM0_UNABLE,
	.text = "UNABLE",
	ASN_UPLINK_INFO_01(0NULL),
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM1_STANDBY,
	.text = "STANDBY",
	ASN_UPLINK_INFO_01(1NULL),
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM2_REQ_DEFERRED,
	.text = "REQUEST DEFERRED",
	ASN_UPLINK_INFO_01(2NULL),
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM3_ROGER,
	.text = "ROGER",
	ASN_UPLINK_INFO_01(3NULL),
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM4_AFFIRM,
	.text = "AFFIRM",
	ASN_UPLINK_INFO_01(4NULL),
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM5_NEGATIVE,
	.text = "NEGATIVE",
	ASN_UPLINK_INFO_01(5NULL),
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM6_EXPCT_alt,
	.text = "EXPECT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(6Altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM7_EXPCT_CLB_AT_time,
	.text = "EXPECT CLIMB AT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_01(7Time),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM8_EXPCT_CLB_AT_pos,
	.text = "EXPECT CLIMB AT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_01(8Position),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM9_EXPCT_DES_AT_time,
	.text = "EXPECT DESCENT AT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_01(9Time),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM10_EXPCT_DES_AT_pos,
	.text = "EXPECT DESCENT AT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_01(10Position),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM11_EXPCT_CRZ_CLB_AT_time,
	.text = "EXPECT CRUISE CLIMB AT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_01(11Time),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM12_EXPCT_CRZ_CLB_AT_pos,
	.text = "EXPECT CRUISE CLIMB AT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_01(12Position),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM13_AT_time_EXPCT_CLB_TO_alt,
	.text = "AT [time] EXPECT CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(13TimeAltitude, time, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM14_AT_pos_EXPCT_CLB_TO_alt,
	.text = "AT [position] EXPECT CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(14PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM15_AT_time_EXPCT_DES_TO_alt,
	.text = "AT [time] EXPECT DESCENT TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(15TimeAltitude, time, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM16_AT_pos_EXPCT_DES_TO_alt,
	.text = "AT [position] EXPECT DESCENT TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(16PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM17_AT_time_EXPCT_CRZ_CLB_TO_alt,
	.text = "AT [time] EXPECT CRUISE CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(17TimeAltitude, time, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM18_AT_pos_EXPCT_CRZ_CLB_TO_alt,
	.text = "AT [position] EXPECT CRUISE CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(18PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM19_MAINT_alt,
	.text = "MAINTAIN [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(19Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM20_CLB_TO_alt,
	.text = "CLIMB TO AND MAINTAIN [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(20Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM21_AT_time_CLB_TO_alt,
	.text = "AT [time] CLIMB TO AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(21TimeAltitude, time, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM22_AT_pos_CLB_TO_alt,
	.text = "AT [position] CLIMB TO AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(22PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM23_DES_TO_alt,
	.text = "DESCEND TO AND MAINTAIN [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(23Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM24_AT_time_DES_TO_alt,
	.text = "AT [time] DESCEND TO AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(24TimeAltitude, time, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM25_AT_pos_DES_TO_alt,
	.text = "AT [position] DESCEND TO AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(25PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM26_CLB_TO_REACH_alt_BY_time,
	.text = "CLIMB TO REACH [altitude] BY [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_2(26AltitudeTime, altitude, time),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM27_CLB_TO_REACH_alt_BY_pos,
	.text = "CLIMB TO REACH [altitude] BY [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_2(27AltitudePosition, altitude, position),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM28_DES_TO_REACH_alt_BY_time,
	.text = "DESCEND TO REACH [altitude] BY [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_2(28AltitudeTime, altitude, time),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM29_DES_TO_REACH_alt_BY_pos,
	.text = "DESCEND TO REACH [altitude] BY [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_2(29AltitudePosition, altitude, position),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM30_MAINT_BLOCK_alt_TO_alt,
	.text = "MAINTAIN BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_SEQ_2(30AltitudeAltitude, list),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM31_CLB_TO_MAINT_BLOCK_alt_TO_alt,
	.text = "CLIMB TO AND MAINTAIN BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_SEQ_2(31AltitudeAltitude, list),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM32_DES_TO_MAINT_BLOCK_alt_TO_alt,
	.text = "DESCEND TO AND MAINTAIN BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_SEQ_2(32AltitudeAltitude, list),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM33_CRZ_alt,
	.text = "CRUISE [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(33Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM34_CRZ_CLB_TO_alt,
	.text = "CRUISE CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(34Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM35_CRZ_CLB_ABV_alt,
	.text = "CRUISE CLIMB ABOVE [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(35Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM36_EXPED_CLB_TO_alt,
	.text = "EXPEDITE CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(36Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM37_EXPED_DES_TO_alt,
	.text = "EXPEDITE DESCEND TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(37Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM38_IMM_CLB_TO_alt,
	.text = "IMMEDIATELY CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(38Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM39_IMM_DES_TO_alt,
	.text = "IMMEDIATELY DESCEND TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(39Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM40_IMM_STOP_CLB_AT_alt,
	.text = "IMMEDIATELY STOP CLIMB AT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(40Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM41_IMM_STOP_DES_AT_alt,
	.text = "IMMEDIATELY STOP DESCENT AT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(41Altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM42_EXPCT_CROSS_pos_AT_alt,
	.text = "EXPECT TO CROSS [position] AT [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(42PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM43_EXPCT_CROSS_pos_AT_alt_OR_ABV,
	.text = "EXPECT TO CROSS [position] AT [altitude] OR ABOVE",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(43PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM44_EXPCT_CROSS_pos_AT_alt_OR_BLW,
	.text = "EXPECT TO CROSS [position] AT [altitude] OR BELOW",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(44PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM45_EXPCT_CROSS_pos_AT_AND_MAINT_alt,
	.text = "EXPECT TO CROSS [position] AT AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(45PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM46_CROSS_pos_AT_alt,
	.text = "CROSS [position] AT [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(46PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM47_CROSS_pos_AT_alt_OR_ABV,
	.text = "CROSS [position] AT OR ABOVE [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(47PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM48_CROSS_pos_AT_alt_OR_BLW,
	.text = "CROSS [position] AT OR BELOW [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(48PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM49_CROSS_pos_AT_AND_MAINT_alt,
	.text = "CROSS [position] AT AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(49PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM50_CROSS_pos_BTWN_alt_AND_alt,
	.text = "CROSS [position] BETWEEN [altitude] AND [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE , CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_1_SEQ_2(50PositionAltitudeAltitude, position,
	    altitude_seqOf.list),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM51_CROSS_pos_AT_time,
	.text = "CROSS [position] AT [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_2(51PositionTime, position, time),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM52_CROSS_pos_AT_OR_BEFORE_time,
	.text = "CROSS [position] AT OR BEFORE [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_2(52PositionTime, position, time),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM53_CROSS_pos_AT_OR_AFTER_time,
	.text = "CROSS [position] AT OR AFTER [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_2(53PositionTime, position, time),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM54_CROSS_pos_BTWN_time_AND_time,
	.text = "CROSS [position] BETWEEN [time] AND [time]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_1_SEQ_2(54PositionTimeTime, position, time_seqOf.list),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM55_CROSS_pos_AT_spd,
	.text = "CROSS [position] AT [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_2(55PositionSpeed, position, speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM56_CROSS_pos_AT_OR_LESS_spd,
	.text = "CROSS [position] AT OR LESS THAN [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_2(56PositionSpeed, position, speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM57_CROSS_pos_AT_OR_GREATER_spd,
	.text = "CROSS [position] AT OR GREATER THAN [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_2(57PositionSpeed, position, speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM58_CROSS_pos_AT_time_AT_alt,
	.text = "CROSS [position] AT [time] AT [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_3(58PositionTimeAltitude, position, time, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM59_CROSS_pos_AT_OR_BEFORE_time_AT_alt,
	.text = "CROSS [position] AT OR BEFORE [time] AT [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_3(59PositionTimeAltitude, position, time, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM60_CROSS_pos_AT_OR_AFTER_time_AT_alt,
	.text = "CROSS [position] AT OR AFTER [time] AT [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_3(60PositionTimeAltitude, position, time, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM61_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	.text = "CROSS [position] AT AND MAINTAIN [altitude] AT [speed]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_3(61PositionAltitudeSpeed, position, altitude, speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM62_AT_time_CROSS_pos_AT_AND_MAINT_alt,
	.text = "AT [time] CROSS [position] AT AND MAINTAIN [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_3(62TimePositionAltitude, time, position, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM63_AT_time_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	.text = "AT [time] CROSS [position] AT AND MAINTAIN [altitude] "
	    "AT [speed]",
	.num_args = 4,
	.args = {
	    CPDLC_ARG_TIME, CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE,
	    CPDLC_ARG_SPEED
	},
	ASN_UPLINK_INFO_4(63TimePositionAltitudeSpeed, time, position,
	    altitude, speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM64_OFFSET_dist_dir_OF_ROUTE,
	.text = "OFFSET [distance offset] [direction] OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DISTANCE_OFFSET, CPDLC_ARG_DIRECTION },
	ASN_UPLINK_INFO_2(64DistanceoffsetDirection, distanceoffset, direction),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM65_AT_pos_OFFSET_dist_dir_OF_ROUTE,
	.text = "AT [position] OFFSET [distance offset] [direction] OF ROUTE",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_DISTANCE_OFFSET,
	    CPDLC_ARG_DIRECTION },
	ASN_UPLINK_INFO_3(65PositionDistanceoffsetDirection, position,
	    distanceoffset, direction),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM66_AT_time_OFFSET_dist_dir_OF_ROUTE,
	.text = "AT [time] OFFSET [distance offset] [direction] OF ROUTE",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_DISTANCE_OFFSET,
	    CPDLC_ARG_DIRECTION },
	ASN_UPLINK_INFO_3(66TimeDistanceoffsetDirection, time,
	    distanceoffset, direction),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM67_PROCEED_BACK_ON_ROUTE,
	.text = "PROCEED BACK ON ROUTE",
	ASN_UPLINK_INFO_01(67NULL),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM68_REJOIN_ROUTE_BY_pos,
	.text = "REJOIN ROUTE BY [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_01(68Position),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM69_REJOIN_ROUTE_BY_time,
	.text = "REJOIN ROUTE BY [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_01(69Time),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM70_EXPCT_BACK_ON_ROUTE_BY_pos,
	.text = "EXPECT BACK ON ROUTE BY [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_01(70Position),
	.resp = CPDLC_RESP_R,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM71_EXPCT_BACK_ON_ROUTE_BY_time,
	.text = "EXPECT BACK ON ROUTE BY [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_01(71Time),
	.resp = CPDLC_RESP_R,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM72_RESUME_OWN_NAV,
	.text = "RESUME OWN NAVIGATION",
	ASN_UPLINK_INFO_01(72NULL),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM73_PDC_predepclx,
	.text = "[predeparture clearance]",
	.num_args = 1,
	.args = { CPDLC_ARG_PDC },
	ASN_UPLINK_INFO_01(73Predepartureclearance),
	.resp = CPDLC_RESP_WU,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM74_DIR_TO_pos,
	.text = "PROCEED DIRECT TO [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_01(74Position),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM75_WHEN_ABL_DIR_TO_pos,
	.text = "WHEN ABLE PROCEED DIRECT TO [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_01(75Position),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM76_AT_time_DIR_TO_pos,
	.text = "AT [time] PROCEED DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_2(76TimePosition, time, position),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM77_AT_pos_DIR_TO_pos,
	.text = "AT [position] PROCEED DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_SEQ_2(77PositionPosition, list),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM78_AT_alt_DIR_TO_pos,
	.text = "AT [altitude] PROCEED DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_2(78AltitudePosition, altitude, position),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM79_CLR_TO_pos_VIA_route,
	.text = "CLEARED TO [position] VIA [route clearance]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	ASN_UPLINK_INFO_2(79PositionRouteclearance, position, routeclearance),
	.resp = CPDLC_RESP_WU,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM80_CLR_route,
	.text = "CLEARED [route clearance]",
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	ASN_UPLINK_INFO_01(80Routeclearance),
	.resp = CPDLC_RESP_WU,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM81_CLR_proc,
	.text = "CLEARED [procedure name]",
	.num_args = 1,
	.args = { CPDLC_ARG_PROCEDURE },
	ASN_UPLINK_INFO_01(81Procedurename),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM82_CLR_DEVIATE_UP_TO_dist_dir_OF_ROUTE,
	.text = "CLEARED TO DEVIATE UP TO [distance offset] [direction] "
	    "OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DISTANCE_OFFSET, CPDLC_ARG_DIRECTION },
	ASN_UPLINK_INFO_2(82DistanceoffsetDirection,
	    distanceoffset, direction),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM83_AT_pos_CLR_route,
	.text = "AT [position] CLEARED [route clearance]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	ASN_UPLINK_INFO_2(83PositionRouteclearance, position, routeclearance),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM84_AT_pos_CLR_proc,
	.text = "AT [position] CLEARED [procedure name]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_PROCEDURE },
	ASN_UPLINK_INFO_2(84PositionProcedurename, position, procedurename),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM85_EXPCT_route,
	.text = "EXPECT [route clearance]",
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	ASN_UPLINK_INFO_01(85Routeclearance),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM86_AT_pos_EXPCT_route,
	.text = "AT [position] EXPECT [route clearance]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	ASN_UPLINK_INFO_2(86PositionRouteclearance, position, routeclearance),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM87_EXPCT_DIR_TO_pos,
	.text = "EXPECT DIRECT TO [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_01(87Position),
	.resp = CPDLC_RESP_WU,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM88_AT_pos_EXPCT_DIR_TO_pos,
	.text = "AT [position] EXPECT DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_SEQ_2(88PositionPosition, list),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM89_AT_time_EXPCT_DIR_TO_pos,
	.text = "AT [time] EXPECT DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_2(89TimePosition, time, position),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM90_AT_alt_EXPCT_DIR_TO_pos,
	.text = "AT [altitude] EXPECT DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_2(90AltitudePosition, altitude, position),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type =
	    CPDLC_UM91_HOLD_AT_pos_MAINT_alt_INBD_deg_TURN_dir_LEG_TIME_time,
	.text = "HOLD AT [position] MAINTAIN [altitude] INBOUND TRACK "
	    "[degrees] [direction] TURN LEG TIME [minutes]",
	.num_args = 5,
	.args = {
	    CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE, CPDLC_ARG_DEGREES,
	    CPDLC_ARG_DIRECTION, CPDLC_ARG_LEGTYPE
	},
	ASN_UPLINK_INFO_5(91Holdclearance, position, altitude, degrees,
	    direction, legtype),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM92_HOLD_AT_pos_AS_PUBLISHED_MAINT_alt,
	.text = "HOLD AT [position] AS PUBLISHED MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_2(92PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM93_EXPCT_FURTHER_CLR_AT_time,
	.text = "EXPECT FURTHER CLEARANCE AT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_01(93Time),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM94_TURN_dir_HDG_deg,
	.text = "TURN [direction] HEADING [degrees]",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DEGREES },
	ASN_UPLINK_INFO_2(94DirectionDegrees, direction, degrees),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM95_TURN_dir_GND_TRK_deg,
	.text = "TURN [direction] GROUND TRACK [degrees]",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DEGREES },
	ASN_UPLINK_INFO_2(95DirectionDegrees, direction, degrees),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM96_FLY_PRESENT_HDG,
	.text = "FLY PRESENT HEADING",
	ASN_UPLINK_INFO_01(96NULL),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM97_AT_pos_FLY_HDG_deg,
	.text = "AT [position] FLY HEADING [degrees]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_DEGREES },
	ASN_UPLINK_INFO_2(97PositionDegrees, position, degrees),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM98_IMM_TURN_dir_HDG_deg,
	.text = "IMMEDIATELY TURN [direction] HEADING [degrees]",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DEGREES },
	ASN_UPLINK_INFO_2(98DirectionDegrees, direction, degrees),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM99_EXPCT_proc,
	.text = "EXPECT [procedure name]",
	.num_args = 1,
	.args = { CPDLC_ARG_PROCEDURE },
	ASN_UPLINK_INFO_01(99Procedurename),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM100_AT_time_EXPCT_spd,
	.text = "AT [time] EXPECT [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_2(100TimeSpeed, time, speed),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM101_AT_pos_EXPCT_spd,
	.text = "AT [position] EXPECT [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_2(101PositionSpeed, position, speed),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM102_AT_alt_EXPCT_spd,
	.text = "AT [altitude] EXPECT [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_2(102AltitudeSpeed, altitude, speed),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM103_AT_time_EXPCT_spd_TO_spd,
	.text = "AT [time] EXPECT [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_1_SEQ_2(103TimeSpeedSpeed, time, speed_seqOf.list),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM104_AT_pos_EXPCT_spd_TO_spd,
	.text = "AT [position] EXPECT [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_1_SEQ_2(104PositionSpeedSpeed, position,
	    speed_seqOf.list),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM105_AT_alt_EXPCT_spd_TO_spd,
	.text = "AT [altitude] EXPECT [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_1_SEQ_2(105AltitudeSpeedSpeed, altitude,
	    speed_seqOf.list),
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM106_MAINT_spd,
	.text = "MAINTAIN [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_01(106Speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM107_MAINT_PRESENT_SPD,
	.text = "MAINTAIN PRESENT SPEED",
	ASN_UPLINK_INFO_01(107NULL),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM108_MAINT_spd_OR_GREATER,
	.text = "MAINTAIN [speed] OR GREATER",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_01(108Speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM109_MAINT_spd_OR_LESS,
	.text = "MAINTAIN [speed] OR LESS",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_01(109Speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM110_MAINT_spd_TO_spd,
	.text = "MAINTAIN [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_SEQ_2(110SpeedSpeed, list),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM111_INCR_SPD_TO_spd,
	.text = "INCREASE SPEED TO [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_01(111Speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM112_INCR_SPD_TO_spd_OR_GREATER,
	.text = "INCREASE SPEED TO [speed] OR GREATER",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_01(112Speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM113_RED_SPD_TO_spd,
	.text = "REDUCE SPEED TO [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_01(113Speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM114_RED_SPD_TO_spd_OR_LESS,
	.text = "REDUCE SPEED TO [speed] OR LESS",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_01(114Speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM115_DO_NOT_EXCEED_spd,
	.text = "DO NOT EXCEED [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_01(115Speed),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM116_RESUME_NORMAL_SPD,
	.text = "RESUME NORMAL SPEED",
	ASN_UPLINK_INFO_01(116NULL),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM117_CTC_icaounitname_freq,
	.text = "CONTACT [icaounitname] [frequency]",
	.num_args = 2,
	.args = { CPDLC_ARG_ICAO_NAME, CPDLC_ARG_FREQUENCY },
	ASN_UPLINK_INFO_2(117ICAOunitnameFrequency, iCAOunitname, frequency),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM118_AT_pos_CONTACT_icaounitname_freq,
	.text = "AT [position] CONTACT [icaounitname] [frequency]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ICAO_NAME,
	    CPDLC_ARG_FREQUENCY },
	ASN_UPLINK_INFO_3(118PositionICAOunitnameFrequency, position,
	    iCAOunitname, frequency),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM119_AT_time_CONTACT_icaounitname_freq,
	.text = "AT [time] CONTACT [icaounitname] [frequency]",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ICAO_NAME, CPDLC_ARG_FREQUENCY },
	ASN_UPLINK_INFO_3(119TimeICAOunitnameFrequency, time,
	    iCAOunitname, frequency),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM120_MONITOR_icaounitname_freq,
	.text = "MONITOR [icaounitname] [frequency]",
	.num_args = 2,
	.args = { CPDLC_ARG_ICAO_NAME, CPDLC_ARG_FREQUENCY },
	ASN_UPLINK_INFO_2(120ICAOunitnameFrequency, iCAOunitname, frequency ),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM121_AT_pos_MONITOR_icaounitname_freq,
	.text = "AT [position] MONITOR [icaounitname] [frequency]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ICAO_NAME,
	    CPDLC_ARG_FREQUENCY },
	ASN_UPLINK_INFO_3(121PositionICAOunitnameFrequency,
	    position, iCAOunitname, frequency),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM122_AT_time_MONITOR_icaounitname_freq,
	.text = "AT [time] CONTACT [icaounitname] [frequency]",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ICAO_NAME, CPDLC_ARG_FREQUENCY },
	ASN_UPLINK_INFO_3(122TimeICAOunitnameFrequency, time,
	    iCAOunitname, frequency),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM123_SQUAWK_code,
	.text = "SQUAWK [beacon code]",
	.num_args = 1,
	.args = { CPDLC_ARG_SQUAWK },
	ASN_UPLINK_INFO_01(123Beaconcode),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM124_STOP_SQUAWK,
	.text = "STOP SQUAWK",
	ASN_UPLINK_INFO_01(124NULL),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM125_SQUAWK_ALT,
	.text = "SQUAWK ALTITUDE",
	ASN_UPLINK_INFO_01(125NULL),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM126_STOP_ALT_SQUAWK,
	.text = "STOP ALTITUDE SQUAWK",
	ASN_UPLINK_INFO_01(126NULL),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM127_REPORT_BACK_ON_ROUTE,
	.text = "REPORT BACK ON ROUTE",
	ASN_UPLINK_INFO_01(127NULL),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM128_REPORT_LEAVING_alt,
	.text = "REPORT LEAVING [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(128Altitude),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM129_REPORT_LEVEL_alt,
	.text = "REPORT LEVEL [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(129Altitude),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM130_REPORT_PASSING_pos,
	.text = "REPORT PASSING [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_01(130Position),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM131_REPORT_RMNG_FUEL_SOULS_ON_BOARD,
	.text = "REPORT REMAINING FUEL AND SOULS ON BOARD",
	ASN_UPLINK_INFO_01(131NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM68_FREETEXT_DISTRESS_text },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM132_CONFIRM_POSITION,
	.text = "CONFIRM POSITION",
	ASN_UPLINK_INFO_01(132NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM33_PRESENT_POS_pos },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM133_CONFIRM_ALT,
	.text = "CONFIRM ALTITUDE",
	ASN_UPLINK_INFO_01(133NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM32_PRESENT_ALT_alt },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM134_CONFIRM_SPD,
	.text = "CONFIRM SPEED",
	ASN_UPLINK_INFO_01(134NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM34_PRESENT_SPD_spd },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM135_CONFIRM_ASSIGNED_ALT,
	.text = "CONFIRM ASSIGNED ALTITUDE",
	ASN_UPLINK_INFO_01(135NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM38_ASSIGNED_ALT_alt },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM136_CONFIRM_ASSIGNED_SPD,
	.text = "CONFIRM ASSIGNED SPEED",
	ASN_UPLINK_INFO_01(136NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM39_ASSIGNED_SPD_spd },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM137_CONFIRM_ASSIGNED_ROUTE,
	.text = "CONFIRM ASSIGNED ROUTE",
	ASN_UPLINK_INFO_01(137NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM40_ASSIGNED_ROUTE_route },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM138_CONFIRM_TIME_OVER_REPORTED_WPT,
	.text = "CONFIRM TIME OVER REPORTED WAYPOINT",
	ASN_UPLINK_INFO_01(138NULL),
	.resp = CPDLC_RESP_NE,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM139_CONFIRM_REPORTED_WPT,
	.text = "CONFIRM REPORTED WAYPOINT",
	ASN_UPLINK_INFO_01(139NULL),
	.resp = CPDLC_RESP_NE,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM140_CONFIRM_NEXT_WPT,
	.text = "CONFIRM NEXT WAYPOINT",
	ASN_UPLINK_INFO_01(140NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM42_NEXT_WPT_pos },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM141_CONFIRM_NEXT_WPT_ETA,
	.text = "CONFIRM NEXT WAYPOINT ETA",
	ASN_UPLINK_INFO_01(141NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM43_NEXT_WPT_ETA_time },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM142_CONFIRM_ENSUING_WPT,
	.text = "CONFIRM ENSUING WAYPOINT",
	ASN_UPLINK_INFO_01(142NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM44_ENSUING_WPT_pos },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM143_CONFIRM_REQ,
	.text = "CONFIRM REQUEST",
	ASN_UPLINK_INFO_01(143NULL),
	.resp = CPDLC_RESP_NE,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM144_CONFIRM_SQUAWK,
	.text = "CONFIRM SQUAWK",
	ASN_UPLINK_INFO_01(144NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM47_SQUAWKING_code },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM145_CONFIRM_HDG,
	.text = "CONFIRM HEADING",
	ASN_UPLINK_INFO_01(145NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM35_PRESENT_HDG_deg },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM146_CONFIRM_GND_TRK,
	.text = "CONFIRM GROUND TRACK",
	ASN_UPLINK_INFO_01(146NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM36_PRESENT_GND_TRK_deg },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM147_REQUEST_POS_REPORT,
	.text = "REQUEST POSITION REPORT",
	ASN_UPLINK_INFO_01(147NULL),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM48_POS_REPORT_posreport },
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM148_WHEN_CAN_YOU_ACPT_alt,
	.text = "WHEN CAN YOU ACCEPT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(148Altitude),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 2,
	.resp_msg_types = { 67, 67 },
	.resp_msg_subtypes = {
	    CPDLC_DM67b_WE_CAN_ACPT_alt_AT_time,
	    CPDLC_DM67e_WE_CANNOT_ACPT_alt
	},
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM149_CAN_YOU_ACPT_alt_AT_pos,
	.text = "CAN YOU ACCEPT [altitude] AT [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_2(149AltitudePosition, altitude, position),
	.resp = CPDLC_RESP_AN,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM150_CAN_YOU_ACPT_alt_AT_time,
	.text = "CAN YOU ACCEPT [altitude] AT [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	ASN_UPLINK_INFO_2(150AltitudeTime, altitude, time),
	.resp = CPDLC_RESP_AN,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM151_WHEN_CAN_YOU_ACPT_spd,
	.text = "WHEN CAN YOU ACCEPT [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_UPLINK_INFO_01(151Speed),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 2,
	.resp_msg_types = { 67, 67 },
	.resp_msg_subtypes = {
	    CPDLC_DM67c_WE_CAN_ACPT_spd_AT_time,
	    CPDLC_DM67f_WE_CANNOT_ACPT_spd
	},
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM152_WHEN_CAN_YOU_ACPT_dist_dir_OFFSET,
	.text = "WHEN CAN YOU ACCEPT [distance offset] [direction] OFFSET",
	.num_args = 2,
	.args = { CPDLC_ARG_DISTANCE_OFFSET, CPDLC_ARG_DIRECTION },
	ASN_UPLINK_INFO_2(152DistanceoffsetDirection, distanceoffset,
	    direction),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 2,
	.resp_msg_types = { 67, 67 },
	.resp_msg_subtypes = {
	    CPDLC_DM67d_WE_CAN_ACPT_dist_dir_AT_time,
	    CPDLC_DM67g_WE_CANNOT_ACPT_dist_dir
	},
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM153_ALTIMETER_baro,
	.text = "ALTIMETER [altimeter]",
	ASN_UPLINK_INFO_01(153Altimeter),
	.num_args = 1,
	.args = { CPDLC_ARG_BARO },
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM154_RDR_SVC_TERM,
	.text = "RADAR SERVICES TERMINATED",
	ASN_UPLINK_INFO_01(154NULL),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM155_RDR_CTC_pos,
	.text = "RADAR CONTACT [position]",
	ASN_UPLINK_INFO_01(155Position),
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM156_RDR_CTC_LOST,
	.text = "RADAR CONTACT LOST",
	ASN_UPLINK_INFO_01(156NULL),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM157_CHECK_STUCK_MIC,
	.text = "CHECK STUCK MICROPHONE [frequency]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREQUENCY },
	ASN_UPLINK_INFO_01(157Frequency),
	.resp = CPDLC_RESP_R,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM158_ATIS_code,
	.text = "ATIS [atis code]",
	.num_args = 1,
	.args = { CPDLC_ARG_ATIS_CODE },
	.resp = CPDLC_RESP_R,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM159_ERROR_description,
	.text = "ERROR [error information]",
	.num_args = 1,
	.args = { CPDLC_ARG_ERRINFO },
	ASN_UPLINK_INFO_01(159Errorinformation),
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM160_NEXT_DATA_AUTHORITY_id,
	.text = "NEXT DATA AUTHORITY [facility designation]",
	.num_args = 1,
	.args = { CPDLC_ARG_ICAO_ID },
	ASN_UPLINK_INFO_01(160ICAOfacilitydesignation),
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM161_END_SVC,
	ASN_UPLINK_INFO_01(161NULL),
	.text = "END SERVICE",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM162_SVC_UNAVAIL,
	ASN_UPLINK_INFO_01(162NULL),
	.text = "SERVICE UNAVAILABLE",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM163_FACILITY_designation_tp4table,
	.text = "[icao facility designation] [tp4Table]",
	.num_args = 2,
	.args = { CPDLC_ARG_ICAO_ID, CPDLC_ARG_TP4TABLE },
	ASN_UPLINK_INFO_2(163ICAOfacilitydesignationTp4table,
	    iCAOfacilitydesignation, tp4table),
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM164_WHEN_RDY,
	ASN_UPLINK_INFO_01(164NULL),
	.text = "WHEN READY",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM165_THEN,
	ASN_UPLINK_INFO_01(165NULL),
	.text = "THEN",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM166_DUE_TO_TFC,
	ASN_UPLINK_INFO_01(166NULL),
	.text = "DUE TO TRAFFIC",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM167_DUE_TO_AIRSPACE_RESTR,
	ASN_UPLINK_INFO_01(167NULL),
	.text = "DUE TO AIRSPACE RESTRICTION",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UM168_DISREGARD,
	ASN_UPLINK_INFO_01(168NULL),
	.text = "DISREGARD",
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UM169_FREETEXT_NORMAL_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	ASN_UPLINK_INFO_01(169Freetext),
	.resp = CPDLC_RESP_R,
	.timeout = MED_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM170_FREETEXT_DISTRESS_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	ASN_UPLINK_INFO_01(170Freetext),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM171_CLB_AT_vvi_MIN,
	.text = "CLIMB AT [vertical rate] MINIMUM",
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	ASN_UPLINK_INFO_01(171Verticalrate),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM172_CLB_AT_vvi_MAX,
	.text = "CLIMB AT [vertical rate] MAXIMUM",
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	ASN_UPLINK_INFO_01(171Verticalrate),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM173_DES_AT_vvi_MIN,
	.text = "DESCEND AT [vertical rate] MINIMUM",
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	ASN_UPLINK_INFO_01(173Verticalrate),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM174_DES_AT_vvi_MAX,
	.text = "DESCEND AT [vertical rate] MAXIMUM",
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	ASN_UPLINK_INFO_01(174Verticalrate),
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM175_REPORT_REACHING_alt,
	.text = "REPORT REACHING [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_01(175Altitude),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM176_MAINT_OWN_SEPARATION_AND_VMC,
	ASN_UPLINK_INFO_01(176NULL),
	.text = "MAINTAIN OWN SEPARATION AND VMC",
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM177_AT_PILOTS_DISCRETION,
	ASN_UPLINK_INFO_01(177NULL),
	.text = "AT PILOTS DISCRETION",
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM179_SQUAWK_IDENT,
	ASN_UPLINK_INFO_01(179NULL),
	.text = "SQUAWK IDENT",
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM180_REPORT_REACHING_BLOCK_alt_TO_alt,
	.text = "REPORT REACHING BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	ASN_UPLINK_INFO_SEQ_2(180AltitudeAltitude, list),
	.resp = CPDLC_RESP_R,
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM181_REPORT_DISTANCE_tofrom_pos,
	.text = "REPORT DISTANCE [to/from] [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_TOFROM, CPDLC_ARG_POSITION },
	ASN_UPLINK_INFO_2(181TofromPosition, tofrom, position),
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM78_AT_time_dist_tofrom_pos },
	.timeout = SHORT_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM182_CONFIRM_ATIS_CODE,
	ASN_UPLINK_INFO_01(182NULL),
	.text = "CONFIRM ATIS CODE",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DM79_ATIS_code },
	.timeout = SHORT_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM183_FREETEXT_NORM_URG_MED_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.timeout = MED_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM187_FREETEXT_LOW_URG_NORM_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.timeout = LONG_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM194_FREETEXT_NORM_URG_LOW_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_Y,
	.timeout = MED_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM195_FREETEXT_LOW_URG_LOW_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_R,
	.timeout = LONG_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM196_FREETEXT_NORM_URG_MED_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_WU,
	.timeout = MED_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM197_FREETEXT_HIGH_URG_MED_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM198_FREETEXT_DISTR_URG_HIGH_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_WU,
	.timeout = SHORT_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM199_FREETEXT_NORM_URG_LOW_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.timeout = MED_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM201_FREETEXT_LOW_URG_LOW_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.timeout = LONG_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM202_FREETEXT_LOW_URG_LOW_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.timeout = LONG_TIMEOUT
    },
    {
	.msg_type = CPDLC_UM203_FREETEXT_NORM_URG_MED_ALERT_text,
	/* not supported in ASN.1 encoding */
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_R,
	.timeout = MED_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM204_FREETEXT_NORM_URG_MED_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_Y,
	.timeout = MED_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM205_FREETEXT_NORM_URG_MED_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_AN,
	.timeout = MED_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM206_FREETEXT_LOW_URG_NORM_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_Y,
	.timeout = LONG_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM207_FREETEXT_LOW_URG_LOW_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_Y,
	.timeout = LONG_TIMEOUT
    },
    {
	/* not supported in ASN.1 encoding */
	.msg_type = CPDLC_UM208_FREETEXT_LOW_URG_LOW_ALERT_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.timeout = LONG_TIMEOUT
    },
    { .msg_type = -1 }	/* List terminator */
};

static const cpdlc_msg_info_t dl_infos[] = {
    {
	.is_dl = true,
	.msg_type = CPDLC_DM0_WILCO,
	ASN_DOWNLINK_INFO_01(0NULL),
	.text = "WILCO",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM1_UNABLE,
	ASN_DOWNLINK_INFO_01(1NULL),
	.text = "UNABLE",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM2_STANDBY,
	ASN_DOWNLINK_INFO_01(2NULL),
	.text = "STANDBY",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM3_ROGER,
	ASN_DOWNLINK_INFO_01(3NULL),
	.text = "ROGER",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM4_AFFIRM,
	ASN_DOWNLINK_INFO_01(4NULL),
	.text = "AFFIRM",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM5_NEGATIVE,
	ASN_DOWNLINK_INFO_01(5NULL),
	.text = "NEGATIVE",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM6_REQ_alt,
	.text = "REQUEST [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(6Altitude),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 2,
	.resp_msg_types = { CPDLC_UM19_MAINT_alt }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM7_REQ_BLOCK_alt_TO_alt,
	.text = "REQUEST BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_SEQ_2(7AltitudeAltitude, list),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM30_MAINT_BLOCK_alt_TO_alt }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM8_REQ_CRZ_CLB_TO_alt,
	.text = "REQUEST CRUISE CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(8Altitude),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM34_CRZ_CLB_TO_alt }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM9_REQ_CLB_TO_alt,
	.text = "REQUEST CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(9Altitude),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM20_CLB_TO_alt }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM10_REQ_DES_TO_alt,
	.text = "REQUEST DESCENT TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(10Altitude),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM23_DES_TO_alt }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM11_AT_pos_REQ_CLB_TO_alt,
	.text = "AT [position] REQUEST CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_2(11PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM22_AT_pos_CLB_TO_alt }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM12_AT_pos_REQ_DES_TO_alt,
	.text = "AT [position] REQUEST DESCENT TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_2(12PositionAltitude, position, altitude),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM25_AT_pos_DES_TO_alt }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM13_AT_time_REQ_CLB_TO_alt,
	.text = "AT [time] REQUEST CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_2(13TimeAltitude, time, altitude),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM21_AT_time_CLB_TO_alt }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM14_AT_time_REQ_DES_TO_alt,
	.text = "AT [time] REQUEST DESCENT TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_2(14TimeAltitude, time, altitude),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM24_AT_time_DES_TO_alt }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM15_REQ_OFFSET_dist_dir_OF_ROUTE,
	.text = "REQUEST OFFSET [distance offset] [direction] OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DISTANCE_OFFSET, CPDLC_ARG_DIRECTION },
	ASN_DOWNLINK_INFO_2(15DistanceoffsetDirection, distanceoffset,
	    direction),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM64_OFFSET_dist_dir_OF_ROUTE }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM16_AT_pos_REQ_OFFSET_dist_dir_OF_ROUTE,
	.text = "AT [position] REQUEST OFFSET [distance offset] "
	    "[direction] OF ROUTE",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_DISTANCE_OFFSET,
	    CPDLC_ARG_DIRECTION },
	ASN_DOWNLINK_INFO_3(16PositionDistanceoffsetDirection,
	    position, distanceoffset, direction),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM65_AT_pos_OFFSET_dist_dir_OF_ROUTE }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM17_AT_time_REQ_OFFSET_dist_dir_OF_ROUTE,
	.text = "AT [time] REQUEST OFFSET [distance offset] "
	    "[direction] OF ROUTE",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_DISTANCE_OFFSET,
	    CPDLC_ARG_DIRECTION },
	ASN_DOWNLINK_INFO_3(17TimeDistanceoffsetDirection, time,
	    distanceoffset, direction),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM66_AT_time_OFFSET_dist_dir_OF_ROUTE }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM18_REQ_spd,
	.text = "REQUEST [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_DOWNLINK_INFO_01(18Speed),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM106_MAINT_spd }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM19_REQ_spd_TO_spd,
	.text = "REQUEST [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	ASN_DOWNLINK_INFO_SEQ_2(19SpeedSpeed, list),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM110_MAINT_spd_TO_spd }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM20_REQ_VOICE_CTC,
	ASN_DOWNLINK_INFO_01(20NULL),
	.text = "REQUEST VOICE CONTACT",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM117_CTC_icaounitname_freq }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM21_REQ_VOICE_CTC_ON_freq,
	.text = "REQUEST VOICE CONTACT ON [frequency]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREQUENCY },
	ASN_DOWNLINK_INFO_01(21Frequency),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM117_CTC_icaounitname_freq }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM22_REQ_DIR_TO_pos,
	.text = "REQUEST DIRECT TO [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_DOWNLINK_INFO_01(22Position),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM74_DIR_TO_pos }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM23_REQ_proc,
	.text = "REQUEST [procedure name]",
	.num_args = 1,
	.args = { CPDLC_ARG_PROCEDURE },
	ASN_DOWNLINK_INFO_01(23Procedurename),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM81_CLR_proc }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM24_REQ_route,
	.text = "REQUEST [route clearance]",
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	ASN_DOWNLINK_INFO_01(24Routeclearance),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM80_CLR_route }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM25_REQ_PDC,
	ASN_DOWNLINK_INFO_01(25NULL),
	.text = "REQUEST CLEARANCE",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM73_PDC_predepclx }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM26_REQ_WX_DEVIATION_TO_pos_VIA_route,
	.text = "REQUEST WEATHER DEVIATION TO [position] VIA [route clearance]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	ASN_DOWNLINK_INFO_2(26PositionRouteclearance, position, routeclearance),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM79_CLR_TO_pos_VIA_route }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM27_REQ_WX_DEVIATION_UP_TO_dist_dir_OF_ROUTE,
	.text = "REQUEST WEATHER DEVIATION UP TO [distance offset] "
	    "[direction] OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DISTANCE_OFFSET, CPDLC_ARG_DIRECTION },
	ASN_DOWNLINK_INFO_2(27DistanceoffsetDirection, distanceoffset,
	    direction),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM82_CLR_DEVIATE_UP_TO_dist_dir_OF_ROUTE }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM28_LEAVING_alt,
	.text = "LEAVING [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(28Altitude),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM29_CLIMBING_TO_alt,
	.text = "CLIMBING TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(29Altitude),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM30_DESCENDING_TO_alt,
	.text = "DESCENDING TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(30Altitude),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM31_PASSING_pos,
	.text = "PASSING [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_DOWNLINK_INFO_01(31Position),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM32_PRESENT_ALT_alt,
	.text = "PASSING [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(32Altitude),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM33_PRESENT_POS_pos,
	.text = "PRESENT POSITION [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_DOWNLINK_INFO_01(33Position),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM34_PRESENT_SPD_spd,
	.text = "PRESENT SPEED [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_DOWNLINK_INFO_01(34Speed),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM35_PRESENT_HDG_deg,
	.text = "PRESENT HEADING [degrees]",
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	ASN_DOWNLINK_INFO_01(35Degrees),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM36_PRESENT_GND_TRK_deg,
	.text = "PRESENT GROUND TRACK [degrees]",
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	ASN_DOWNLINK_INFO_01(36Degrees),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM37_LEVEL_alt,
	.text = "LEVEL [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(37Altitude),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM38_ASSIGNED_ALT_alt,
	.text = "ASSIGNED ALTITUDE [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(38Altitude),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM39_ASSIGNED_SPD_spd,
	.text = "ASSIGNED SPEED [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_DOWNLINK_INFO_01(39Speed),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM40_ASSIGNED_ROUTE_route,
	.text = "ASSIGNED ROUTE [route]",
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	ASN_DOWNLINK_INFO_01(40Routeclearance),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM41_BACK_ON_ROUTE,
	ASN_DOWNLINK_INFO_01(41NULL),
	.text = "BACK ON ROUTE",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM42_NEXT_WPT_pos,
	.text = "NEXT WAYPOINT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_DOWNLINK_INFO_01(42Position),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM43_NEXT_WPT_ETA_time,
	.text = "NEXT WAYPOINT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	ASN_DOWNLINK_INFO_01(43Time),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM44_ENSUING_WPT_pos,
	.text = "ENSUING WAYPOINT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_DOWNLINK_INFO_01(44Position),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM45_REPORTED_WPT_pos,
	.text = "REPORTED WAYPOINT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	ASN_DOWNLINK_INFO_01(45Position),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM46_REPORTED_WPT_time,
	.text = "REPORTED WAYPOINT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	ASN_DOWNLINK_INFO_01(46Time),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM47_SQUAWKING_code,
	.text = "SQUAWKING [beacon code]",
	.num_args = 1,
	.args = { CPDLC_ARG_SQUAWK },
	ASN_DOWNLINK_INFO_01(47Beaconcode),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM48_POS_REPORT_posreport,
	.text = "POSITION REPORT [posreport]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSREPORT },
	ASN_DOWNLINK_INFO_01(48Positionreport),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM49_WHEN_CAN_WE_EXPCT_spd,
	.text = "WHEN CAN WE EXPECT [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	ASN_DOWNLINK_INFO_01(49Speed),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UM100_AT_time_EXPCT_spd,
	    CPDLC_UM101_AT_pos_EXPCT_spd,
	    CPDLC_UM102_AT_alt_EXPCT_spd
	}
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM50_WHEN_CAN_WE_EXPCT_spd_TO_spd,
	.text = "WHEN CAN WE EXPECT [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	ASN_DOWNLINK_INFO_SEQ_2(50SpeedSpeed, list),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UM103_AT_time_EXPCT_spd_TO_spd,
	    CPDLC_UM104_AT_pos_EXPCT_spd_TO_spd,
	    CPDLC_UM105_AT_alt_EXPCT_spd_TO_spd
	}
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM51_WHEN_CAN_WE_EXPCT_BACK_ON_ROUTE,
	ASN_DOWNLINK_INFO_01(51NULL),
	.text = "WHEN CAN WE EXPECT BACK ON ROUTE",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UM70_EXPCT_BACK_ON_ROUTE_BY_pos,
	    CPDLC_UM71_EXPCT_BACK_ON_ROUTE_BY_time,
	    CPDLC_UM67_PROCEED_BACK_ON_ROUTE
	}
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM52_WHEN_CAN_WE_EXPECT_LOWER_ALT,
	ASN_DOWNLINK_INFO_01(52NULL),
	.text = "WHEN CAN WE EXPECT LOWER ALTITUDE",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UM9_EXPCT_DES_AT_time,
	    CPDLC_UM10_EXPCT_DES_AT_pos,
	    CPDLC_UM23_DES_TO_alt
	}
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM53_WHEN_CAN_WE_EXPECT_HIGHER_ALT,
	ASN_DOWNLINK_INFO_01(53NULL),
	.text = "WHEN CAN WE EXPECT HIGHER ALTITUDE",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UM7_EXPCT_CLB_AT_time,
	    CPDLC_UM8_EXPCT_CLB_AT_pos,
	    CPDLC_UM20_CLB_TO_alt
	}
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM54_WHEN_CAN_WE_EXPECT_CRZ_CLB_TO_alt,
	.text = "WHEN CAN WE EXPECT CRUISE CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(54Altitude),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UM17_AT_time_EXPCT_CRZ_CLB_TO_alt,
	    CPDLC_UM18_AT_pos_EXPCT_CRZ_CLB_TO_alt,
	    CPDLC_UM34_CRZ_CLB_TO_alt
	},
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM55_PAN_PAN_PAN,
	ASN_DOWNLINK_INFO_01(55NULL),
	.text = "PAN PAN PAN",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM56_MAYDAY_MAYDAY_MAYDAY,
	ASN_DOWNLINK_INFO_01(56NULL),
	.text = "MAYDAY MAYDAY MAYDAY",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM57_RMNG_FUEL_AND_POB,
	.text = "[fuel] OF FUEL REMAINING AND [persons] PERSONS ON BOARD",
	.resp = CPDLC_RESP_N,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME_DUR, CPDLC_ARG_PERSONS },
	ASN_DOWNLINK_INFO_2(57RemainingfuelRemainingsouls, remainingfuel,
	    remainingsouls),
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM58_CANCEL_EMERG,
	ASN_DOWNLINK_INFO_01(58NULL),
	.text = "CANCEL EMERGENCY",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM59_DIVERTING_TO_pos_VIA_route,
	.text = "DIVERTING TO [pos] VIA [route]",
	.resp = CPDLC_RESP_N,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	ASN_DOWNLINK_INFO_2(59PositionRouteclearance, position, routeclearance)
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM60_OFFSETTING_dist_dir_OF_ROUTE,
	.text = "OFFSETTING [distance offset] [direction] OF ROUTE",
	.resp = CPDLC_RESP_N,
	.num_args = 2,
	.args = { CPDLC_ARG_DISTANCE_OFFSET, CPDLC_ARG_DIRECTION },
	ASN_DOWNLINK_INFO_2(60DistanceoffsetDirection, distanceoffset,
	    direction),
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM61_DESCENDING_TO_alt,
	.text = "DESCENDING TO [alt]",
	.resp = CPDLC_RESP_N,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(61Altitude),
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM62_ERROR_errorinfo,
	.text = "ERROR [error information]",
	.num_args = 1,
	.args = { CPDLC_ARG_ERRINFO },
	ASN_DOWNLINK_INFO_01(62Errorinformation),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM63_NOT_CURRENT_DATA_AUTHORITY,
	ASN_DOWNLINK_INFO_01(63NULL),
	.text = "NOT CURRENT DATA AUTHORITY",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM64_CURRENT_DATA_AUTHORITY_id,
	.text = "[icao facility designation]",
	.num_args = 1,
	.args = { CPDLC_ARG_ICAO_ID },
	ASN_DOWNLINK_INFO_01(64ICAOfacilitydesignation),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM65_DUE_TO_WX,
	ASN_DOWNLINK_INFO_01(65NULL),
	.text = "DUE TO WEATHER",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM66_DUE_TO_ACFT_PERF,
	ASN_DOWNLINK_INFO_01(66NULL),
	.text = "DUE TO AIRCRAFT PERFORMANCE",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM67_FREETEXT_NORMAL_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	ASN_DOWNLINK_INFO_01(67Freetext),
	.resp = CPDLC_RESP_N
    },
    {
	/* not supported in ASN.1 encoding */
	.is_dl = true,
	.msg_type = 67,
	.msg_subtype = CPDLC_DM67b_WE_CAN_ACPT_alt_AT_time,
	.text = "WE CAN ACCEPT [altitude] AT [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N
    },
    {
	/* not supported in ASN.1 encoding */
	.is_dl = true,
	.msg_type = 67,
	.msg_subtype = CPDLC_DM67c_WE_CAN_ACPT_spd_AT_time,
	.text = "WE CAN ACCEPT [speed] AT [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N
    },
    {
	/* not supported in ASN.1 encoding */
	.is_dl = true,
	.msg_type = 67,
	.msg_subtype = CPDLC_DM67d_WE_CAN_ACPT_dist_dir_AT_time,
	.text = "WE CAN ACCEPT [distance offset] [direction] AT [time]",
	.num_args = 3,
	.args = { CPDLC_ARG_DISTANCE_OFFSET, CPDLC_ARG_DIRECTION,
	    CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N
    },
    {
	/* not supported in ASN.1 encoding */
	.is_dl = true,
	.msg_type = 67,
	.msg_subtype = CPDLC_DM67e_WE_CANNOT_ACPT_alt,
	.text = "WE CANNOT ACCEPT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	/* not supported in ASN.1 encoding */
	.is_dl = true,
	.msg_type = 67,
	.msg_subtype = CPDLC_DM67f_WE_CANNOT_ACPT_spd,
	.text = "WE CANNOT ACCEPT [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_N
    },
    {
	/* not supported in ASN.1 encoding */
	.is_dl = true,
	.msg_type = 67,
	.msg_subtype = CPDLC_DM67g_WE_CANNOT_ACPT_dist_dir,
	.text = "WE CANNOT ACCEPT [distance offset] [direction]",
	.num_args = 2,
	.args = { CPDLC_ARG_DISTANCE_OFFSET, CPDLC_ARG_DIRECTION },
	.resp = CPDLC_RESP_N
    },
    {
	/* not supported in ASN.1 encoding */
	.is_dl = true,
	.msg_type = 67,
	.msg_subtype = CPDLC_DM67h_WHEN_CAN_WE_EXPCT_CLB_TO_alt,
	.text = "WHEN CAN WE EXPECT CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	/* not supported in ASN.1 encoding */
	.is_dl = true,
	.msg_type = 67,
	.msg_subtype = CPDLC_DM67i_WHEN_CAN_WE_EXPCT_DES_TO_alt,
	.text = "WHEN CAN WE EXPECT DESCENT TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM68_FREETEXT_DISTRESS_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	ASN_DOWNLINK_INFO_01(68Freetext),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM69_REQ_VMC_DES,
	ASN_DOWNLINK_INFO_01(69NULL),
	.text = "REQUEST VMC DESCENT",
	.resp = CPDLC_RESP_Y
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM70_REQ_HDG_deg,
	.text = "REQUEST HEADING [degrees]",
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	ASN_DOWNLINK_INFO_01(70Degrees),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM94_TURN_dir_HDG_deg }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM71_REQ_GND_TRK_deg,
	.text = "REQUEST GROUND TRACK [degrees]",
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	ASN_DOWNLINK_INFO_01(71Degrees),
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UM95_TURN_dir_GND_TRK_deg }
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM72_REACHING_alt,
	.text = "REACHING [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_01(72Altitude),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM73_VERSION_number,
	.text = "[version nr]",
	.num_args = 1,
	.args = { CPDLC_ARG_VERSION },
	ASN_DOWNLINK_INFO_01(73Versionnumber),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM74_MAINT_OWN_SEPARATION_AND_VMC,
	ASN_DOWNLINK_INFO_01(74NULL),
	.text = "MAINTAIN OWN SEPARATION AND VMC",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM75_AT_PILOTS_DISCRETION,
	ASN_DOWNLINK_INFO_01(75NULL),
	.text = "AT PILOTS DISCRETION",
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM76_REACHING_BLOCK_alt_TO_alt,
	.text = "REACHING BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_SEQ_2(76AltitudeAltitude, list),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM77_ASSIGNED_BLOCK_alt_TO_alt,
	.text = "ASSIGNED BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	ASN_DOWNLINK_INFO_SEQ_2(77AltitudeAltitude, list),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM78_AT_time_dist_tofrom_pos,
	.text = "AT [time] [distance] [to/from] [position]",
	.num_args = 4,
	.args = {
	    CPDLC_ARG_TIME, CPDLC_ARG_DISTANCE, CPDLC_ARG_TOFROM,
	    CPDLC_ARG_POSITION
	},
	ASN_DOWNLINK_INFO_4(78TimeDistanceTofromPosition, time,
	    distance, tofrom, position),
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM79_ATIS_code,
	.text = "ATIS [atis code]",
	.num_args = 1,
	.args = { CPDLC_ARG_ATIS_CODE },
	.resp = CPDLC_RESP_N
    },
    {
	.is_dl = true,
	.msg_type = CPDLC_DM80_DEVIATING_dist_dir_OF_ROUTE,
	.text = "DEVIATING [distance offset] [direction] OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DISTANCE_OFFSET, CPDLC_ARG_DIRECTION },
	ASN_DOWNLINK_INFO_2(80DistanceoffsetDirection, distanceoffset,
	    direction),
	.resp = CPDLC_RESP_N
    },
    { .msg_type = -1 }	/* List terminator */
};

const cpdlc_msg_info_t *cpdlc_ul_infos = ul_infos;
const cpdlc_msg_info_t *cpdlc_dl_infos = dl_infos;

