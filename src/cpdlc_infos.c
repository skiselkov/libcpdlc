/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * CDDL HEADER END
*/
/*
 * Copyright 2019 Saso Kiselkov. All rights reserved.
 */

#include "cpdlc.h"

#define	NUM_UL_DENY_REASONS	3
#define	UL_DENY_REASONS \
	CPDLC_UL166_DUE_TO_TFC, \
	CPDLC_UL167_DUE_TO_AIRSPACE_RESTR, \
	CPDLC_UL169_FREETEXT_NORMAL_text

const cpdlc_msg_info_t cpdlc_ul_infos[] = {
    { .msg_type = CPDLC_UL0_UNABLE, .text = "UNABLE", .resp = CPDLC_RESP_NE },
    { .msg_type = CPDLC_UL1_STANDBY, .text = "STANDBY", .resp = CPDLC_RESP_NE },
    {
	.msg_type = CPDLC_UL2_REQ_DEFERRED,
	.text = "REQUEST DEFERRED",
	.resp = CPDLC_RESP_NE
    },
    { .msg_type = CPDLC_UL3_ROGER, .text = "ROGER", .resp = CPDLC_RESP_NE },
    { .msg_type = CPDLC_UL4_AFFIRM, .text = "AFFIRM", .resp = CPDLC_RESP_NE },
    {
	.msg_type = CPDLC_UL5_NEGATIVE,
	.text = "NEGATIVE",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL6_EXPCT_alt,
	.text = "EXPECT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL7_EXPCT_CLB_AT_time,
	.text = "EXPECT CLIMB AT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL8_EXPCT_CLB_AT_pos,
	.text = "EXPECT CLIMB AT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL9_EXPCT_DES_AT_time,
	.text = "EXPECT DESCENT AT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL10_EXPCT_DES_AT_pos,
	.text = "EXPECT DESCENT AT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL11_EXPCT_CRZ_CLB_AT_time,
	.text = "EXPECT CRUISE CLIMB AT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL12_EXPCT_CRZ_CLB_AT_pos,
	.text = "EXPECT CRUISE CLIMB AT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL13_AT_time_EXPCT_CLB_TO_alt,
	.text = "AT [time] EXPECT CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL14_AT_pos_EXPCT_CLB_TO_alt,
	.text = "AT [position] EXPECT CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL15_AT_time_EXPCT_DES_TO_alt,
	.text = "AT [time] EXPECT DESCENT TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL16_AT_pos_EXPCT_DES_TO_alt,
	.text = "AT [position] EXPECT DESCENT TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL17_AT_time_EXPCT_CRZ_CLB_TO_alt,
	.text = "AT [time] EXPECT CRUISE CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL18_AT_pos_EXPCT_CRZ_CLB_TO_alt,
	.text = "AT [position] EXPECT CRUISE CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL19_MAINT_alt,
	.text = "MAINTAIN [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL20_CLB_TO_alt,
	.text = "CLIMB TO AND MAINTAIN [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL21_AT_time_CLB_TO_alt,
	.text = "AT [time] CLIMB TO AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL22_AT_pos_CLB_TO_alt,
	.text = "AT [position] CLIMB TO AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL23_DES_TO_alt,
	.text = "DESCEND TO AND MAINTAIN [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL24_AT_time_DES_TO_alt,
	.text = "AT [time] DESCEND TO AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL25_AT_pos_DES_TO_alt,
	.text = "AT [position] DESCEND TO AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL26_CLB_TO_REACH_alt_BY_time,
	.text = "CLIMB TO REACH [altitude] BY [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL27_CLB_TO_REACH_alt_BY_pos,
	.text = "CLIMB TO REACH [altitude] BY [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL28_DES_TO_REACH_alt_BY_time,
	.text = "DESCEND TO REACH [altitude] BY [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL29_DES_TO_REACH_alt_BY_pos,
	.text = "DESCEND TO REACH [altitude] BY [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL30_MAINT_BLOCK_alt_TO_alt,
	.text = "MAINTAIN BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL31_CLB_TO_MAINT_BLOCK_alt_TO_alt,
	.text = "CLIMB TO AND MAINTAIN BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL32_DES_TO_MAINT_BLOCK_alt_TO_alt,
	.text = "DESCEND TO AND MAINTAIN BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL33_CRZ_alt,
	.text = "CRUISE [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL34_CRZ_CLB_TO_alt,
	.text = "CRUISE CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL35_CRZ_CLB_ABV_alt,
	.text = "CRUISE CLIMB ABOVE [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL36_EXPED_CLB_TO_alt,
	.text = "EXPEDITE CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL37_EXPED_DES_TO_alt,
	.text = "EXPEDITE DESCEND TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL38_IMM_CLB_TO_alt,
	.text = "IMMEDIATELY CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL39_IMM_DES_TO_alt,
	.text = "IMMEDIATELY DESCEND TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL40_IMM_STOP_CLB_AT_alt,
	.text = "IMMEDIATELY STOP CLIMB AT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL41_IMM_STOP_DES_AT_alt,
	.text = "IMMEDIATELY STOP DESCENT AT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL42_EXPCT_CROSS_pos_AT_alt,
	.text = "EXPECT TO CROSS [position] AT [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL43_EXPCT_CROSS_pos_AT_alt_OR_ABV,
	.text = "EXPECT TO CROSS [position] AT [altitude] OR ABOVE",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL44_EXPCT_CROSS_pos_AT_alt_OR_BLW,
	.text = "EXPECT TO CROSS [position] AT [altitude] OR BELOW",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL45_EXPCT_CROSS_pos_AT_AND_MAINT_alt,
	.text = "EXPECT TO CROSS [position] AT AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL46_CROSS_pos_AT_alt,
	.text = "CROSS [position] AT [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL47_CROSS_pos_AT_alt_OR_ABV,
	.text = "CROSS [position] AT OR ABOVE [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL48_CROSS_pos_AT_alt_OR_BLW,
	.text = "CROSS [position] AT OR BELOW [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL49_CROSS_pos_AT_AND_MAINT_alt,
	.text = "CROSS [position] AT AND MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL50_CROSS_pos_BTWN_alt_AND_alt,
	.text = "CROSS POSITION BETWEEN [altitude] AND [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE , CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL51_CROSS_pos_AT_time,
	.text = "CROSS [position] AT [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL52_CROSS_pos_AT_OR_BEFORE_time,
	.text = "CROSS [position] AT OR BEFORE [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL53_CROSS_pos_AT_OR_AFTER_time,
	.text = "CROSS [position] AT OR AFTER [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL54_CROSS_pos_BTWN_time_AND_time,
	.text = "CROSS [position] BETWEEN [time] AND [time]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL55_CROSS_pos_AT_spd,
	.text = "CROSS [position] AT [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL56_CROSS_pos_AT_OR_LESS_spd,
	.text = "CROSS [position] AT OR LESS THAN [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL57_CROSS_pos_AT_OR_GREATER_spd,
	.text = "CROSS [position] AT OR GREATER THAN [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL58_CROSS_pos_AT_time_AT_alt,
	.text = "CROSS [position] AT [time] AT [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL59_CROSS_pos_AT_OR_BEFORE_time_AT_alt,
	.text = "CROSS [position] AT OR BEFORE [time] AT [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL60_CROSS_pos_AT_OR_AFTER_time_AT_alt,
	.text = "CROSS [position] AT OR AFTER [time] AT [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL61_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	.text = "CROSS [position] AT AND MAINTAIN [altitude] AT [speed]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL62_AT_time_CROSS_pos_AT_AND_MAINT_alt,
	.text = "AT [time] CROSS [position] AT AND MAINTAIN [altitude]",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL63_AT_time_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	.text = "AT [time] CROSS [position] AT AND MAINTAIN [altitude] "
	    "AT [speed]",
	.num_args = 4,
	.args = {
	    CPDLC_ARG_TIME, CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE,
	    CPDLC_ARG_SPEED
	},
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL64_OFFSET_dir_dist_OF_ROUTE,
	.text = "OFFSET [direction] [distance offset] OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL65_AT_pos_OFFSET_dir_dist_OF_ROUTE,
	.text = "AT [position] OFFSET [direction] [distance offset] OF ROUTE",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL66_AT_time_OFFSET_dir_dist_OF_ROUTE,
	.text = "AT [time] OFFSET [direction] [distance offset] OF ROUTE",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL67_PROCEED_BACK_ON_ROUTE,
	.text = "PROCEED BACK ON ROUTE",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL68_REJOIN_ROUTE_BY_pos,
	.text = "REJOIN ROUTE BY [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL69_REJOIN_ROUTE_BY_time,
	.text = "REJOIN ROUTE BY [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL70_EXPCT_BACK_ON_ROUTE_BY_pos,
	.text = "EXPECT BACK ON ROUTE BY [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL71_EXPCT_BACK_ON_ROUTE_BY_time,
	.text = "EXPECT BACK ON ROUTE BY [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL72_RESUME_OWN_NAV,
	.text = "RESUME OWN NAVIGATION",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL73_PDC_route,
	.text = "[route]",
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL74_DIR_TO_pos,
	.text = "PROCEED DIRECT TO [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL75_WHEN_ABL_DIR_TO_pos,
	.text = "WHEN ABLE PROCEED DIRECT TO [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL76_AT_time_DIR_TO_pos,
	.text = "AT [time] PROCEED DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL77_AT_pos_DIR_TO_pos,
	.text = "AT [position] PROCEED DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL78_AT_alt_DIR_TO_pos,
	.text = "AT [altitude] PROCEED DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL79_CLR_TO_pos_VIA_route,
	.text = "CLEARED TO [position] VIA [route clearance]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL80_CLR_route,
	.text = "CLEARED [route clearance]",
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL81_CLR_proc,
	.text = "CLEARED [procedure name]",
	.num_args = 1,
	.args = { CPDLC_ARG_PROCEDURE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL82_CLR_DEVIATE_UP_TO_dir_dist_OF_ROUTE,
	.text = "CLEARED TO DEVIATE UP TO [direction] [distance offset] "
	    "OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL83_AT_pos_CLR_route,
	.text = "AT [position] CLEARED [route clearance]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL84_AT_pos_CLR_proc,
	.text = "AT [position] CLEARED [procedure name]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_PROCEDURE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL85_EXPCT_route,
	.text = "EXPECT [route clearance]",
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL86_AT_pos_EXPCT_route,
	.text = "AT [position] EXPECT [route clearance]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL87_EXPCT_DIR_TO_pos,
	.text = "EXPECT DIRECT TO [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL88_AT_pos_EXPCT_DIR_TO_pos,
	.text = "AT [position] EXPECT DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL89_AT_time_EXPCT_DIR_TO_pos,
	.text = "AT [time] EXPECT DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL90_AT_alt_EXPCT_DIR_TO_pos,
	.text = "AT [altitude] EXPECT DIRECT TO [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type =
	    CPDLC_UL91_HOLD_AT_pos_MAINT_alt_INBOUND_deg_TURN_dir_LEG_TIME_time,
	.text = "HOLD AT [position] MAINTAIN [altitude] INBOUND TRACK"
	    "[degrees] [direction] TURN LEG TIME [leg type]",
	.num_args = 5,
	.args = {
	    CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE, CPDLC_ARG_DEGREES,
	    CPDLC_ARG_DIRECTION, CPDLC_ARG_TIME
	},
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL92_HOLD_AT_pos_AS_PUBLISHED_MAINT_alt,
	.text = "HOLD AT [position] AS PUBLISHED MAINTAIN [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL93_EXPCT_FURTHER_CLR_AT_time,
	.text = "EXPECT FURTHER CLEARANCE AT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL94_TURN_dir_HDG_deg,
	.text = "TURN [direction] HEADING [degrees]",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL95_TURN_dir_GND_TRK_deg,
	.text = "TURN [direction] GROUND TRACK [degrees]",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL96_FLY_PRESENT_HDG,
	.text = "FLY PRESENT HEADING",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL97_AT_pos_FLY_HDG_deg,
	.text = "AT [position] FLY HEADING [degrees]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL98_IMM_TURN_dir_HDG_deg,
	.text = "IMMEDIATELY TURN [direction] HEADING [degrees]",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL99_EXPCT_proc,
	.text = "EXPECT [procedure name]",
	.num_args = 1,
	.args = { CPDLC_ARG_PROCEDURE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL100_AT_time_EXPCT_spd,
	.text = "AT [time] EXPECT [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL101_AT_pos_EXPCT_spd,
	.text = "AT [position] EXPECT [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL102_AT_alt_EXPCT_spd,
	.text = "AT [altitude] EXPECT [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL103_AT_time_EXPCT_spd_TO_spd,
	.text = "AT [time] EXPECT [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL104_AT_pos_EXPCT_spd_TO_spd,
	.text = "AT [position] EXPECT [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL105_AT_alt_EXPCT_spd_TO_spd,
	.text = "AT [altitude] EXPECT [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL106_MAINT_spd,
	.text = "MAINTAIN [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL107_MAINT_PRESENT_SPD,
	.text = "MAINTAIN PRESENT SPEED",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL108_MAINT_spd_OR_GREATER,
	.text = "MAINTAIN [speed] OR GREATER",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL109_MAINT_spd_OR_LESS,
	.text = "MAINTAIN [speed] OR LESS",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL110_MAINT_spd_TO_spd,
	.text = "MAINTAIN [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL111_INCR_SPD_TO_spd,
	.text = "INCREASE SPEED TO [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL112_INCR_SPD_TO_spd_OR_GREATER,
	.text = "INCREASE SPEED TO [speed] OR GREATER",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL113_RED_SPD_TO_spd,
	.text = "REDUCE SPEED TO [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL114_RED_SPD_TO_spd_OR_LESS,
	.text = "REDUCE SPEED TO [speed] OR LESS",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL115_DO_NOT_EXCEED_spd,
	.text = "DO NOT EXCEED [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL116_RESUME_NORMAL_SPD,
	.text = "RESUME NORMAL SPEED",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL117_CTC_icaounitname_freq,
	.text = "CONTACT [icaounitname] [frequency]",
	.num_args = 2,
	.args = { CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL118_AT_pos_CONTACT_icaounitname_freq,
	.text = "AT [position] CONTACT [icaounitname] [frequency]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL119_AT_time_CONTACT_icaounitname_freq,
	.text = "AT [time] CONTACT [icaounitname] [frequency]",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL120_MONITOR_icaounitname_freq,
	.text = "MONITOR [icaounitname] [frequency]",
	.num_args = 2,
	.args = { CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL121_AT_pos_MONITOR_icaounitname_freq,
	.text = "AT [position] MONITOR [icaounitname] [frequency]",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL122_AT_time_MONITOR_icaounitname_freq,
	.text = "AT [time] CONTACT [icaounitname] [frequency]",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL123_SQUAWK_code,
	.text = "SQUAWK [beacon code]",
	.num_args = 1,
	.args = { CPDLC_ARG_SQUAWK },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL124_STOP_SQUAWK,
	.text = "STOP SQUAWK",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL125_SQUAWK_ALT,
	.text = "SQUAWK ALTITUDE",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL126_STOP_ALT_SQUAWK,
	.text = "STOP ALTITUDE SQUAWK",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL127_REPORT_BACK_ON_ROUTE,
	.text = "REPORT BACK ON ROUTE",
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL128_REPORT_LEAVING_alt,
	.text = "REPORT LEAVING [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL129_REPORT_LEVEL_alt,
	.text = "REPORT LEVEL [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL130_REPORT_PASSING_pos,
	.text = "REPORT PASSING [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL131_REPORT_RMNG_FUEL_SOULS_ON_BOARD,
	.text = "REPORT REMAINING FUEL AND SOULS ON BOARD",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL68_FREETEXT_DISTRESS_text }
    },
    {
	.msg_type = CPDLC_UL132_CONFIRM_POSITION,
	.text = "CONFIRM POSITION",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL33_PRESENT_POS_pos }
    },
    {
	.msg_type = CPDLC_UL133_CONFIRM_ALT,
	.text = "CONFIRM ALTITUDE",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL32_PRESENT_ALT_alt }
    },
    {
	.msg_type = CPDLC_UL134_CONFIRM_SPD,
	.text = "CONFIRM SPEED",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL34_PRESENT_SPD_spd }
    },
    {
	.msg_type = CPDLC_UL135_CONFIRM_ASSIGNED_ALT,
	.text = "CONFIRM ASSIGNED ALTITUDE",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL38_ASSIGNED_ALT_alt }
    },
    {
	.msg_type = CPDLC_UL136_CONFIRM_ASSIGNED_SPD,
	.text = "CONFIRM ASSIGNED SPEED",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL39_ASSIGNED_SPD_spd }
    },
    {
	.msg_type = CPDLC_UL137_CONFIRM_ASSIGNED_ROUTE,
	.text = "CONFIRM ASSIGNED ROUTE",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL40_ASSIGNED_ROUTE_route }
    },
    {
	.msg_type = CPDLC_UL138_CONFIRM_TIME_OVER_REPORTED_WPT,
	.text = "CONFIRM TIME OVER REPORTED WAYPOINT",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL139_CONFIRM_REPORTED_WPT,
	.text = "CONFIRM REPORTED WAYPOINT",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL140_CONFIRM_NEXT_WPT,
	.text = "CONFIRM NEXT WAYPOINT",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL42_NEXT_WPT_pos }
    },
    {
	.msg_type = CPDLC_UL141_CONFIRM_NEXT_WPT_ETA,
	.text = "CONFIRM NEXT WAYPOINT ETA",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL43_NEXT_WPT_ETA_time }
    },
    {
	.msg_type = CPDLC_UL142_CONFIRM_ENSUING_WPT,
	.text = "CONFIRM ENSUING WAYPOINT",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL44_ENSUING_WPT_pos }
    },
    {
	.msg_type = CPDLC_UL143_CONFIRM_REQ,
	.text = "CONFIRM REQUEST",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL144_CONFIRM_SQUAWK,
	.text = "CONFIRM SQUAWK",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL47_SQUAWKING_code }
    },
    {
	.msg_type = CPDLC_UL145_CONFIRM_HDG,
	.text = "CONFIRM HEADING",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL35_PRESENT_HDG_deg }
    },
    {
	.msg_type = CPDLC_UL146_CONFIRM_GND_TRK,
	.text = "CONFIRM GROUND TRACK",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL36_PRESENT_GND_TRK_deg }
    },
    {
	.msg_type = CPDLC_UL147_REQUEST_POS_REPORT,
	.text = "REQUEST POSITION REPORT",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL48_POS_REPORT_posreport }
    },
    {
	.msg_type = CPDLC_UL148_WHEN_CAN_YOU_ACPT_alt,
	.text = "WHEN CAN YOU ACCEPT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 2,
	.resp_msg_types = { 67, 67 },
	.resp_msg_subtypes = {
	    CPDLC_DL67b_WE_CAN_ACPT_alt_AT_time,
	    CPDLC_DL67e_WE_CANNOT_ACPT_alt
	}
    },
    {
	.msg_type = CPDLC_UL149_CAN_YOU_ACPT_alt_AT_pos,
	.text = "CAN YOU ACCEPT [altitude] AT [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_AN
    },
    {
	.msg_type = CPDLC_UL150_CAN_YOU_ACPT_alt_AT_time,
	.text = "CAN YOU ACCEPT [altitude] AT [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_AN
    },
    {
	.msg_type = CPDLC_UL151_WHEN_CAN_YOU_ACPT_spd,
	.text = "WHEN CAN YOU ACCEPT [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 2,
	.resp_msg_types = { 67, 67 },
	.resp_msg_subtypes = {
	    CPDLC_DL67c_WE_CAN_ACPT_spd_AT_time,
	    CPDLC_DL67f_WE_CANNOT_ACPT_spd
	}
    },
    {
	.msg_type = CPDLC_UL152_WHEN_CAN_YOU_ACPT_dir_dist_OFFSET,
	.text = "WHEN CAN YOU ACCEPT [direction] [distance] OFFSET",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 2,
	.resp_msg_types = { 67, 67 },
	.resp_msg_subtypes = {
	    CPDLC_DL67d_WE_CAN_ACPT_dir_dist_AT_time,
	    CPDLC_DL67g_WE_CANNOT_ACPT_dir_dist
	}
    },
    {
	.msg_type = CPDLC_UL153_ALTIMETER_baro,
	.text = "ALTIMETER [altimeter]",
	.num_args = 1,
	.args = { CPDLC_ARG_BARO },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL154_RDR_SVC_TERM,
	.text = "RADAR SERVICES TERMINATED",
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL155_RDR_CTC_pos,
	.text = "RADAR CONTACT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL155_RDR_CTC_pos,
	.text = "RADAR CONTACT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL156_RDR_CTC_LOST,
	.text = "RADAR CONTACT LOST",
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL157_CHECK_STUCK_MIC,
	.text = "CHECK STUCK MICROPHONE",
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL158_ATIS_code,
	.text = "ATIS [atis code]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL159_ERROR_description,
	.text = "ERROR [error information]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL160_NEXT_DATA_AUTHORITY_id,
	.text = "NEXT DATA AUTHORITY [facility designation]",
	.num_args = 1,
	.args = { CPDLC_ARG_ICAONAME },
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL161_END_SVC,
	.text = "END SERVICE",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL162_SVC_UNAVAIL,
	.text = "SERVICE UNAVAILABLE",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL163_FACILITY_designation_tp4table,
	.text = "[icao facility designation] [tp4Table]",
	.num_args = 2,
	.args = { CPDLC_ARG_ICAONAME, CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL164_WHEN_RDY,
	.text = "WHEN READY",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL165_THEN,
	.text = "THEN",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL166_DUE_TO_TFC,
	.text = "DUE TO TRAFFIC",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL167_DUE_TO_AIRSPACE_RESTR,
	.text = "DUE TO AIRSPACE RESTRICTION",
	.resp = CPDLC_RESP_NE
    },
    {
	.msg_type = CPDLC_UL168_DISREGARD,
	.text = "DISREGARD",
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL169_FREETEXT_NORMAL_text,
	.text = "[freetext]",
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL170_FREETEXT_DISTRESS_text,
	.text = "[freetext]",
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL171_CLB_AT_vvi_MIN,
	.text = "CLIMB AT [vertical rate] MINIMUM",
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL172_CLB_AT_vvi_MAX,
	.text = "CLIMB AT [vertical rate] MAXIMUM",
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL173_DES_AT_vvi_MIN,
	.text = "DESCEND AT [vertical rate] MINIMUM",
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL174_DES_AT_vvi_MAX,
	.text = "DESCEND AT [vertical rate] MAXIMUM",
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL175_REPORT_REACHING_alt,
	.text = "REPORT REACHING [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL176_MAINT_OWN_SEPARATION_AND_VMC,
	.text = "MAINTAIN OWN SEPARATION AND VMC",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL177_AT_PILOTS_DISCRETION,
	.text = "AT PILOTS DISCRETION",
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL179_SQUAWK_IDENT,
	.text = "SQUAWK IDENT",
	.resp = CPDLC_RESP_WU
    },
    {
	.msg_type = CPDLC_UL180_REPORT_REACHING_BLOCK_alt_TO_alt,
	.text = "REPORT REACHING BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R
    },
    {
	.msg_type = CPDLC_UL181_REPORT_DISTANCE_tofrom_pos,
	.text = "REPORT DISTANCE [to/from] [position]",
	.num_args = 2,
	.args = { CPDLC_ARG_TOFROM, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL78_AT_time_dist_tofrom_pos }
    },
    {
	.msg_type = CPDLC_UL182_CONFIRM_ATIS_CODE,
	.text = "CONFIRM ATIS CODE",
	.resp = CPDLC_RESP_NE,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_DL79_ATIS_code }
    },
    { .msg_type = -1 }	/* List terminator */
};

const cpdlc_msg_info_t cpdlc_dl_infos[] = {
    {
	.msg_type = CPDLC_DL0_WILCO,
	.text = "WILCO",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL1_UNABLE,
	.text = "UNABLE",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL2_STANDBY,
	.text = "STANDBY",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL3_ROGER,
	.text = "ROGER",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL4_AFFIRM,
	.text = "AFFIRM",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL5_NEGATIVE,
	.text = "NEGATIVE",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL6_REQ_alt,
	.text = "REQUEST [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 2,
	.resp_msg_types = { CPDLC_UL19_MAINT_alt }
    },
    {
	.msg_type = CPDLC_DL7_REQ_BLOCK_alt_TO_alt,
	.text = "REQUEST BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL30_MAINT_BLOCK_alt_TO_alt }
    },
    {
	.msg_type = CPDLC_DL8_REQ_CRZ_CLB_TO_alt,
	.text = "REQUEST CRUISE CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL34_CRZ_CLB_TO_alt }
    },
    {
	.msg_type = CPDLC_DL9_REQ_CLB_TO_alt,
	.text = "REQUEST CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL20_CLB_TO_alt }
    },
    {
	.msg_type = CPDLC_DL10_REQ_DES_TO_alt,
	.text = "REQUEST DESCENT TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL23_DES_TO_alt }
    },
    {
	.msg_type = CPDLC_DL11_AT_pos_REQ_CLB_TO_alt,
	.text = "AT [position] REQUEST CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL22_AT_pos_CLB_TO_alt }
    },
    {
	.msg_type = CPDLC_DL12_AT_pos_REQ_DES_TO_alt,
	.text = "AT [position] REQUEST DESCENT TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL25_AT_pos_DES_TO_alt }
    },
    {
	.msg_type = CPDLC_DL13_AT_time_REQ_CLB_TO_alt,
	.text = "AT [time] REQUEST CLIMB TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL21_AT_time_CLB_TO_alt }
    },
    {
	.msg_type = CPDLC_DL14_AT_time_REQ_DES_TO_alt,
	.text = "AT [time] REQUEST DESCENT TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL24_AT_time_DES_TO_alt }
    },
    {
	.msg_type = CPDLC_DL15_REQ_OFFSET_dir_dist_OF_ROUTE,
	.text = "REQUEST OFFSET [direction] [distance] OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL64_OFFSET_dir_dist_OF_ROUTE }
    },
    {
	.msg_type = CPDLC_DL16_AT_pos_REQ_OFFSET_dir_dist_OF_ROUTE,
	.text = "AT [position] REQUEST OFFSET [direction] [distance] OF ROUTE",
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL65_AT_pos_OFFSET_dir_dist_OF_ROUTE }
    },
    {
	.msg_type = CPDLC_DL17_AT_time_REQ_OFFSET_dir_dist_OF_ROUTE,
	.text = "AT [time] REQUEST OFFSET [direction] [distance] OF ROUTE",
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL66_AT_time_OFFSET_dir_dist_OF_ROUTE }
    },
    {
	.msg_type = CPDLC_DL18_REQ_spd,
	.text = "REQUEST [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL106_MAINT_spd }
    },
    {
	.msg_type = CPDLC_DL19_REQ_spd_TO_spd,
	.text = "REQUEST [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL110_MAINT_spd_TO_spd }
    },
    {
	.msg_type = CPDLC_DL20_REQ_VOICE_CTC,
	.text = "REQUEST VOICE CONTACT",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL117_CTC_icaounitname_freq }
    },
    {
	.msg_type = CPDLC_DL21_REQ_VOICE_CTC_ON_freq,
	.text = "REQUEST VOICE CONTACT ON [frequency]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL117_CTC_icaounitname_freq }
    },
    {
	.msg_type = CPDLC_DL22_REQ_DIR_TO_pos,
	.text = "REQUEST DIRECT TO [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL74_DIR_TO_pos }
    },
    {
	.msg_type = CPDLC_DL23_REQ_proc,
	.text = "REQUEST [procedure name]",
	.num_args = 1,
	.args = { CPDLC_ARG_PROCEDURE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL81_CLR_proc }
    },
    {
	.msg_type = CPDLC_DL24_REQ_route,
	.text = "REQUEST [route clearance]",
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL80_CLR_route }
    },
    {
	.msg_type = CPDLC_DL25_REQ_PDC,
	.text = "REQUEST CLEARANCE",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL73_PDC_route }
    },
    {
	.msg_type = CPDLC_DL26_REQ_WX_DEVIATION_TO_pos_VIA_route,
	.text = "REQUEST WEATHER DEVIATION TO [position] VIA [route clearance]",
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL79_CLR_TO_pos_VIA_route }
    },
    {
	.msg_type = CPDLC_DL27_REQ_WX_DEVIATION_UP_TO_dir_dist_OF_ROUTE,
	.text = "REQUEST WEATHER DEVIATION UP TO [direction] "
	    "[distance offset] OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL82_CLR_DEVIATE_UP_TO_dir_dist_OF_ROUTE }
    },
    {
	.msg_type = CPDLC_DL28_LEAVING_alt,
	.text = "LEAVING [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL29_CLIMBING_TO_alt,
	.text = "CLIMBING TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL30_DESCENDING_TO_alt,
	.text = "DESCENDING TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL31_PASSING_pos,
	.text = "PASSING [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL32_PRESENT_ALT_alt,
	.text = "PASSING [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL33_PRESENT_POS_pos,
	.text = "PRESENT POSITION [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL34_PRESENT_SPD_spd,
	.text = "PRESENT SPEED [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL35_PRESENT_HDG_deg,
	.text = "PRESENT HEADING [degrees]",
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL36_PRESENT_GND_TRK_deg,
	.text = "PRESENT GROUND TRACK [degrees]",
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL37_LEVEL_alt,
	.text = "LEVEL [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL38_ASSIGNED_ALT_alt,
	.text = "ASSIGNED ALTITUDE [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL39_ASSIGNED_SPD_spd,
	.text = "ASSIGNED SPEED [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL40_ASSIGNED_ROUTE_route,
	.text = "ASSIGNED ROUTE [route]",
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL41_BACK_ON_ROUTE,
	.text = "BACK ON ROUTE",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL42_NEXT_WPT_pos,
	.text = "NEXT WAYPOINT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL43_NEXT_WPT_ETA_time,
	.text = "NEXT WAYPOINT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL44_ENSUING_WPT_pos,
	.text = "ENSUING WAYPOINT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL45_REPORTED_WPT_pos,
	.text = "REPORTED WAYPOINT [position]",
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL46_REPORTED_WPT_time,
	.text = "REPORTED WAYPOINT [time]",
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL47_SQUAWKING_code,
	.text = "SQUAWKING [beacon code]",
	.num_args = 1,
	.args = { CPDLC_ARG_SQUAWK },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL48_POS_REPORT_posreport,
	.text = "POSITION REPORT [posreport]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL49_WHEN_CAN_WE_EXPCT_spd,
	.text = "WHEN CAN WE EXPECT [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UL100_AT_time_EXPCT_spd,
	    CPDLC_UL101_AT_pos_EXPCT_spd,
	    CPDLC_UL102_AT_alt_EXPCT_spd
	}
    },
    {
	.msg_type = CPDLC_DL50_WHEN_CAN_WE_EXPCT_spd_TO_spd,
	.text = "WHEN CAN WE EXPECT [speed] TO [speed]",
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UL103_AT_time_EXPCT_spd_TO_spd,
	    CPDLC_UL104_AT_pos_EXPCT_spd_TO_spd,
	    CPDLC_UL105_AT_alt_EXPCT_spd_TO_spd
	}
    },
    {
	.msg_type = CPDLC_DL51_WHEN_CAN_WE_EXPCT_BACK_ON_ROUTE,
	.text = "WHEN CAN WE EXPECT BACK ON ROUTE",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UL70_EXPCT_BACK_ON_ROUTE_BY_pos,
	    CPDLC_UL71_EXPCT_BACK_ON_ROUTE_BY_time,
	    CPDLC_UL67_PROCEED_BACK_ON_ROUTE
	}
    },
    {
	.msg_type = CPDLC_DL52_WHEN_CAN_WE_EXPECT_LOWER_ALT,
	.text = "WHEN CAN WE EXPECT LOWER ALTITUDE",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UL9_EXPCT_DES_AT_time,
	    CPDLC_UL10_EXPCT_DES_AT_pos,
	    CPDLC_UL23_DES_TO_alt
	}
    },
    {
	.msg_type = CPDLC_DL53_WHEN_CAN_WE_EXPECT_HIGHER_ALT,
	.text = "WHEN CAN WE EXPECT HIGHER ALTITUDE",
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UL7_EXPCT_CLB_AT_time,
	    CPDLC_UL8_EXPCT_CLB_AT_pos,
	    CPDLC_UL20_CLB_TO_alt
	}
    },
    {
	.msg_type = CPDLC_DL54_WHEN_CAN_WE_EXPECT_CRZ_CLB_TO_alt,
	.text = "WHEN CAN WE EXPECT CRUISE CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 3,
	.resp_msg_types = {
	    CPDLC_UL17_AT_time_EXPCT_CRZ_CLB_TO_alt,
	    CPDLC_UL18_AT_pos_EXPCT_CRZ_CLB_TO_alt,
	    CPDLC_UL34_CRZ_CLB_TO_alt
	},
    },
    {
	.msg_type = CPDLC_DL62_ERROR_errorinfo,
	.text = "ERROR [error information]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL63_NOT_CURRENT_DATA_AUTHORITY,
	.text = "NOT CURRENT DATA AUTHORITY",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL64_CURRENT_DATA_AUTHORITY_id,
	.text = "[icao facility designation]",
	.num_args = 1,
	.args = { CPDLC_ARG_ICAONAME },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL65_DUE_TO_WX,
	.text = "DUE TO WEATHER",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL66_DUE_TO_ACFT_PERF,
	.text = "DUE TO AIRCRAFT PERFORMANCE",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL67_FREETEXT_NORMAL_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = 67,
	.msg_subtype = CPDLC_DL67b_WE_CAN_ACPT_alt_AT_time,
	.text = "WE CAN ACCEPT [altitude] AT [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = 67,
	.msg_subtype = CPDLC_DL67c_WE_CAN_ACPT_spd_AT_time,
	.text = "WE CAN ACCEPT [speed] AT [time]",
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = 67,
	.msg_subtype = CPDLC_DL67d_WE_CAN_ACPT_dir_dist_AT_time,
	.text = "WE CAN ACCEPT [direction] [distance offset] AT [time]",
	.num_args = 3,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = 67,
	.msg_subtype = CPDLC_DL67e_WE_CANNOT_ACPT_alt,
	.text = "WE CANNOT ACCEPT [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = 67,
	.msg_subtype = CPDLC_DL67f_WE_CANNOT_ACPT_spd,
	.text = "WE CANNOT ACCEPT [speed]",
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = 67,
	.msg_subtype = CPDLC_DL67g_WE_CANNOT_ACPT_dir_dist,
	.text = "WE CANNOT ACCEPT [direction] [distance offset]",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = 67,
	.msg_subtype = CPDLC_DL67h_WHEN_CAN_WE_EXPCT_CLB_TO_alt,
	.text = "WHEN CAN WE EXPECT CLIMB TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = 67,
	.msg_subtype = CPDLC_DL67i_WHEN_CAN_WE_EXPCT_DES_TO_alt,
	.text = "WHEN CAN WE EXPECT DESCENT TO [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL68_FREETEXT_DISTRESS_text,
	.text = "[freetext]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL70_REQ_HDG_deg,
	.text = "REQUEST HEADING [degrees]",
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL94_TURN_dir_HDG_deg }
    },
    {
	.msg_type = CPDLC_DL71_REQ_GND_TRK_deg,
	.text = "REQUEST GROUND TRACK [degrees]",
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_Y,
	.num_resp_msgs = 1,
	.resp_msg_types = { CPDLC_UL95_TURN_dir_GND_TRK_deg }
    },
    {
	.msg_type = CPDLC_DL72_REACHING_alt,
	.text = "REACHING [altitude]",
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL73_VERSION_number,
	.text = "[version nr]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL74_MAINT_OWN_SEPARATION_AND_VMC,
	.text = "MAINTAIN OWN SEPARATION AND VMC",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL75_AT_PILOTS_DISCRETION,
	.text = "AT PILOTS DISCRETION",
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL76_REACHING_BLOCK_alt_TO_alt,
	.text = "REACHING BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL77_ASSIGNED_BLOCK_alt_TO_alt,
	.text = "ASSIGNED BLOCK [altitude] TO [altitude]",
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL78_AT_time_dist_tofrom_pos,
	.text = "AT [time] [distance] [to/from] [position]",
	.num_args = 4,
	.args = {
	    CPDLC_ARG_TIME, CPDLC_ARG_DISTANCE, CPDLC_ARG_TOFROM,
	    CPDLC_ARG_POSITION
	},
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL79_ATIS_code,
	.text = "ATIS [atis code]",
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N
    },
    {
	.msg_type = CPDLC_DL80_DEVIATING_dir_dist_OF_ROUTE,
	.text = "DEVIATING [direction] [distance offset] OF ROUTE",
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_N
    },
    { .msg_type = -1 }	/* List terminator */
};
