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

const cpdlc_msg_info_t cpdlc_ul_infos[] = {
    { .msg_nr = CPDLC_UL0_UNABLE, .resp = CPDLC_RESP_NE, .text = "UNABLE" },
    { .msg_nr = CPDLC_UL1_STANDBY, .resp = CPDLC_RESP_NE, .text = "STANDBY" },
    {
	.msg_nr = CPDLC_UL2_REQ_DEFERRED,
	.resp = CPDLC_RESP_NE,
	.text = "REQUEST DEFERRED"
    },
    { .msg_nr = CPDLC_UL3_ROGER, .resp = CPDLC_RESP_NE, .text = "ROGER" },
    { .msg_nr = CPDLC_UL4_AFFIRM, .resp = CPDLC_RESP_NE, .text = "AFFIRM" },
    { .msg_nr = CPDLC_UL5_NEGATIVE, .resp = CPDLC_RESP_NE, .text = "NEGATIVE" },
    {
	.msg_nr = CPDLC_UL6_EXPCT_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT [altitude]"
    },
    {
	.msg_nr = CPDLC_UL7_EXPCT_CLB_AT_time,
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT CLIMB AT [time]"
    },
    {
	.msg_nr = CPDLC_UL8_EXPCT_CLB_AT_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT CLIMB AT [position]"
    },
    {
	.msg_nr = CPDLC_UL9_EXPCT_DES_AT_time,
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT DESCENT AT [time]"
    },
    {
	.msg_nr = CPDLC_UL10_EXPCT_DES_AT_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT DESCENT AT [position]"
    },
    {
	.msg_nr = CPDLC_UL11_EXPCT_CRZ_CLB_AT_time,
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT CRUISE CLIMB AT [time]"
    },
    {
	.msg_nr = CPDLC_UL12_EXPCT_CRZ_CLB_AT_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT CRUISE CLIMB AT [position]"
    },
    {
	.msg_nr = CPDLC_UL13_AT_time_EXPCT_CLB_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "AT [time] EXPECT CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL14_AT_pos_EXPCT_CLB_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "AT [position] EXPECT CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL15_AT_time_EXPCT_DES_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "AT [time] EXPECT DESCENT TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL16_AT_pos_EXPCT_DES_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "AT [position] EXPECT DESCENT TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL17_AT_time_EXPCT_CRZ_CLB_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "AT [time] EXPECT CRUISE CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL18_AT_pos_EXPCT_CRZ_CLB_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "AT [position] EXPECT CRUISE CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL19_MAINT_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL20_CLB_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CLIMB TO AND MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL21_AT_time_CLB_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "AT [time] CLIMB TO AND MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL22_AT_pos_CLB_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "AT [position] CLIMB TO AND MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL23_DES_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "DESCEND TO AND MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL24_AT_time_DES_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "AT [time] DESCEND TO AND MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL25_AT_pos_DES_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "AT [position] DESCEND TO AND MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL26_CLB_TO_REACH_alt_BY_time,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU,
	.text = "CLIMB TO REACH [altitude] BY [time]"
    },
    {
	.msg_nr = CPDLC_UL27_CLB_TO_REACH_alt_BY_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU,
	.text = "CLIMB TO REACH [altitude] BY [position]"
    },
    {
	.msg_nr = CPDLC_UL28_DES_TO_REACH_alt_BY_time,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU,
	.text = "DESCEND TO REACH [altitude] BY [time]"
    },
    {
	.msg_nr = CPDLC_UL29_DES_TO_REACH_alt_BY_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU,
	.text = "DESCEND TO REACH [altitude] BY [position]"
    },
    {
	.msg_nr = CPDLC_UL30_MAINT_BLOCK_alt_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "MAINTAIN BLOCK [altitude] TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL31_CLB_TO_MAINT_BLOCK_alt_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CLIMB TO AND MAINTAIN BLOCK [altitude] TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL32_DES_TO_MAINT_BLOCK_alt_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "DESCEND TO AND MAINTAIN BLOCK [altitude] TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL33_CRZ_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CRUISE [altitude]"
    },
    {
	.msg_nr = CPDLC_UL34_CRZ_CLB_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CRUISE CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL35_CRZ_CLB_ABV_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CRUISE CLIMB ABOVE [altitude]"
    },
    {
	.msg_nr = CPDLC_UL36_EXPED_CLB_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "EXPEDITE CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL37_EXPED_DES_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "EXPEDITE DESCEND TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL38_IMM_CLB_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "IMMEDIATELY CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL39_IMM_DES_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "IMMEDIATELY DESCEND TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL40_IMM_STOP_CLB_AT_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "IMMEDIATELY STOP CLIMB AT [altitude]"
    },
    {
	.msg_nr = CPDLC_UL41_IMM_STOP_DES_AT_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "IMMEDIATELY STOP DESCENT AT [altitude]"
    },
    {
	.msg_nr = CPDLC_UL42_EXPCT_CROSS_pos_AT_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT TO CROSS [position] AT [altitude]"
    },
    {
	.msg_nr = CPDLC_UL43_EXPCT_CROSS_pos_AT_alt_OR_ABV,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT TO CROSS [position] AT [altitude] OR ABOVE"
    },
    {
	.msg_nr = CPDLC_UL44_EXPCT_CROSS_pos_AT_alt_OR_BLW,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT TO CROSS [position] AT [altitude] OR BELOW"
    },
    {
	.msg_nr = CPDLC_UL45_EXPCT_CROSS_pos_AT_AND_MAINT_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT TO CROSS [position] AT AND MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL46_CROSS_pos_AT_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT [altitude]"
    },
    {
	.msg_nr = CPDLC_UL47_CROSS_pos_AT_alt_OR_ABV,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT OR ABOVE [altitude]"
    },
    {
	.msg_nr = CPDLC_UL48_CROSS_pos_AT_alt_OR_BLW,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT OR BELOW [altitude]"
    },
    {
	.msg_nr = CPDLC_UL49_CROSS_pos_AT_AND_MAINT_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT AND MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL50_CROSS_pos_BTWN_alt_AND_alt,
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE , CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS POSITION BETWEEN [altitude] AND [altitude]"
    },
    {
	.msg_nr = CPDLC_UL51_CROSS_pos_AT_time,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT [time]"
    },
    {
	.msg_nr = CPDLC_UL52_CROSS_pos_AT_OR_BEFORE_time,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT OR BEFORE [time]"
    },
    {
	.msg_nr = CPDLC_UL53_CROSS_pos_AT_OR_AFTER_time,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT OR AFTER [time]"
    },
    {
	.msg_nr = CPDLC_UL54_CROSS_pos_BTWN_time_AND_time,
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] BETWEEN [time] AND [time]"
    },
    {
	.msg_nr = CPDLC_UL55_CROSS_pos_AT_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT [speed]"
    },
    {
	.msg_nr = CPDLC_UL56_CROSS_pos_AT_OR_LESS_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT OR LESS THAN [speed]"
    },
    {
	.msg_nr = CPDLC_UL57_CROSS_pos_AT_OR_GREATER_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT OR GREATER THAN [speed]"
    },
    {
	.msg_nr = CPDLC_UL58_CROSS_pos_AT_time_AT_alt,
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT [time] AT [altitude]"
    },
    {
	.msg_nr = CPDLC_UL59_CROSS_pos_AT_OR_BEFORE_time_AT_alt,
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT OR BEFORE [time] AT [altitude]"
    },
    {
	.msg_nr = CPDLC_UL60_CROSS_pos_AT_OR_AFTER_time_AT_alt,
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT OR AFTER [time] AT [altitude]"
    },
    {
	.msg_nr = CPDLC_UL61_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "CROSS [position] AT AND MAINTAIN [altitude] AT [speed]"
    },
    {
	.msg_nr = CPDLC_UL62_AT_time_CROSS_pos_AT_AND_MAINT_alt,
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "AT [time] CROSS [position] AT AND MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL63_AT_time_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	.num_args = 4,
	.args = {
	    CPDLC_ARG_TIME, CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE,
	    CPDLC_ARG_SPEED
	},
	.resp = CPDLC_RESP_WU,
	.text = "AT [time] CROSS [position] AT AND MAINTAIN [altitude] "
	    "AT [speed]"
    },
    {
	.msg_nr = CPDLC_UL64_OFFSET_dir_dist_OF_ROUTE,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_WU,
	.text = "OFFSET [direction] [distance offset] OF ROUTE"
    },
    {
	.msg_nr = CPDLC_UL65_AT_pos_OFFSET_dir_dist_OF_ROUTE,
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_WU,
	.text = "AT [position] OFFSET [direction] [distance offset] OF ROUTE"
    },
    {
	.msg_nr = CPDLC_UL66_AT_time_OFFSET_dir_dist_OF_ROUTE,
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_WU,
	.text = "AT [time] OFFSET [direction] [distance offset] OF ROUTE"
    },
    {
	.msg_nr = CPDLC_UL67_PROCEED_BACK_ON_ROUTE,
	.resp = CPDLC_RESP_WU,
	.text = "PROCEED BACK ON ROUTE"
    },
    {
	.msg_nr = CPDLC_UL68_REJOIN_ROUTE_BY_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU,
	.text = "REJOIN ROUTE BY [position]"
    },
    {
	.msg_nr = CPDLC_UL69_REJOIN_ROUTE_BY_time,
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_WU,
	.text = "REJOIN ROUTE BY [time]"
    },
    {
	.msg_nr = CPDLC_UL70_EXPCT_BACK_ON_ROUTE_BY_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT BACK ON ROUTE BY [position]"
    },
    {
	.msg_nr = CPDLC_UL71_EXPCT_BACK_ON_ROUTE_BY_time,
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT BACK ON ROUTE BY [time]"
    },
    {
	.msg_nr = CPDLC_UL72_RESUME_OWN_NAV,
	.resp = CPDLC_RESP_WU,
	.text = "RESUME OWN NAVIGATION"
    },
    {
	.msg_nr = CPDLC_UL73_PDC_route,
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_WU,
	.text = "[route]"
    },
    {
	.msg_nr = CPDLC_UL74_DIR_TO_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU,
	.text = "PROCEED DIRECT TO [position]"
    },
    {
	.msg_nr = CPDLC_UL75_WHEN_ABL_DIR_TO_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU,
	.text = "WHEN ABLE PROCEED DIRECT TO [position]"
    },
    {
	.msg_nr = CPDLC_UL76_AT_time_DIR_TO_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU,
	.text = "AT [time] PROCEED DIRECT TO [position]"
    },
    {
	.msg_nr = CPDLC_UL77_AT_pos_DIR_TO_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU,
	.text = "AT [position] PROCEED DIRECT TO [position]"
    },
    {
	.msg_nr = CPDLC_UL78_AT_alt_DIR_TO_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU,
	.text = "AT [altitude] PROCEED DIRECT TO [position]"
    },
    {
	.msg_nr = CPDLC_UL79_CLR_TO_pos_VIA_route,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_WU,
	.text = "CLEARED TO [position] VIA [route clearance]"
    },
    {
	.msg_nr = CPDLC_UL80_CLR_route,
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_WU,
	.text = "CLEARED [route clearance]"
    },
    {
	.msg_nr = CPDLC_UL81_CLR_proc,
	.num_args = 1,
	.args = { CPDLC_ARG_PROCEDURE },
	.resp = CPDLC_RESP_WU,
	.text = "CLEARED [procedure name]"
    },
    {
	.msg_nr = CPDLC_UL82_CLR_DEVIATE_UP_TO_dir_dist_OF_ROUTE,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_WU,
	.text = "CLEARED TO DEVIATE UP TO [direction] [distance offset] "
	    "OF ROUTE"
    },
    {
	.msg_nr = CPDLC_UL83_AT_pos_CLR_route,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_WU,
	.text = "AT [position] CLEARED [route clearance]"
    },
    {
	.msg_nr = CPDLC_UL84_AT_pos_CLR_proc,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_PROCEDURE },
	.resp = CPDLC_RESP_WU,
	.text = "AT [position] CLEARED [procedure name]"
    },
    {
	.msg_nr = CPDLC_UL85_EXPCT_route,
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT [route clearance]"
    },
    {
	.msg_nr = CPDLC_UL86_AT_pos_EXPCT_route,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_R,
	.text = "AT [position] EXPECT [route clearance]"
    },
    {
	.msg_nr = CPDLC_UL87_EXPCT_DIR_TO_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_WU,
	.text = "EXPECT DIRECT TO [position]"
    },
    {
	.msg_nr = CPDLC_UL88_AT_pos_EXPCT_DIR_TO_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.text = "AT [position] EXPECT DIRECT TO [position]"
    },
    {
	.msg_nr = CPDLC_UL89_AT_time_EXPCT_DIR_TO_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.text = "AT [time] EXPECT DIRECT TO [position]"
    },
    {
	.msg_nr = CPDLC_UL90_AT_alt_EXPCT_DIR_TO_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.text = "AT [altitude] EXPECT DIRECT TO [position]"
    },
    {
	.msg_nr =
	    CPDLC_UL91_HOLD_AT_pos_MAINT_alt_INBOUND_deg_TURN_dir_LEG_TIME_time,
	.num_args = 5,
	.args = {
	    CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE, CPDLC_ARG_DEGREES,
	    CPDLC_ARG_DIRECTION, CPDLC_ARG_TIME
	},
	.resp = CPDLC_RESP_WU,
	.text = "HOLD AT [position] MAINTAIN [altitude] INBOUND TRACK"
	    "[degrees] [direction] TURN LEG TIME [leg type]"
    },
    {
	.msg_nr = CPDLC_UL92_HOLD_AT_pos_AS_PUBLISHED_MAINT_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_WU,
	.text = "HOLD AT [position] AS PUBLISHED MAINTAIN [altitude]"
    },
    {
	.msg_nr = CPDLC_UL93_EXPCT_FURTHER_CLR_AT_time,
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT FURTHER CLEARANCE AT [time]"
    },
    {
	.msg_nr = CPDLC_UL94_TURN_dir_HDG_deg,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_WU,
	.text = "TURN [direction] HEADING [degrees]"
    },
    {
	.msg_nr = CPDLC_UL95_TURN_dir_GND_TRK_deg,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_WU,
	.text = "TURN [direction] GROUND TRACK [degrees]"
    },
    {
	.msg_nr = CPDLC_UL96_FLY_PRESENT_HDG,
	.resp = CPDLC_RESP_WU,
	.text = "FLY PRESENT HEADING"
    },
    {
	.msg_nr = CPDLC_UL97_AT_pos_FLY_HDG_deg,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_WU,
	.text = "AT [position] FLY HEADING [degrees]"
    },
    {
	.msg_nr = CPDLC_UL98_IMM_TURN_dir_HDG_deg,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_WU,
	.text = "IMMEDIATELY TURN [direction] HEADING [degrees]"
    },
    {
	.msg_nr = CPDLC_UL99_EXPCT_proc,
	.num_args = 1,
	.args = { CPDLC_ARG_PROCEDURE },
	.resp = CPDLC_RESP_R,
	.text = "EXPECT [procedure name]"
    },
    {
	.msg_nr = CPDLC_UL100_AT_time_EXPCT_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R,
	.text = "AT [time] EXPECT [speed]"
    },
    {
	.msg_nr = CPDLC_UL101_AT_pos_EXPCT_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R,
	.text = "AT [position] EXPECT [speed]"
    },
    {
	.msg_nr = CPDLC_UL102_AT_alt_EXPCT_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R,
	.text = "AT [altitude] EXPECT [speed]"
    },
    {
	.msg_nr = CPDLC_UL103_AT_time_EXPCT_spd_TO_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R,
	.text = "AT [time] EXPECT [speed] TO [speed]"
    },
    {
	.msg_nr = CPDLC_UL104_AT_pos_EXPCT_spd_TO_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R,
	.text = "AT [position] EXPECT [speed] TO [speed]"
    },
    {
	.msg_nr = CPDLC_UL105_AT_alt_EXPCT_spd_TO_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_R,
	.text = "AT [altitude] EXPECT [speed] TO [speed]"
    },
    {
	.msg_nr = CPDLC_UL106_MAINT_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "MAINTAIN [speed]"
    },
    {
	.msg_nr = CPDLC_UL107_MAINT_PRESENT_SPD,
	.resp = CPDLC_RESP_WU,
	.text = "MAINTAIN PRESENT SPEED"
    },
    {
	.msg_nr = CPDLC_UL108_MAINT_spd_OR_GREATER,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "MAINTAIN [speed] OR GREATER"
    },
    {
	.msg_nr = CPDLC_UL109_MAINT_spd_OR_LESS,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "MAINTAIN [speed] OR LESS"
    },
    {
	.msg_nr = CPDLC_UL110_MAINT_spd_TO_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "MAINTAIN [speed] TO [speed]"
    },
    {
	.msg_nr = CPDLC_UL111_INCR_SPD_TO_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "INCREASE SPEED TO [speed]"
    },
    {
	.msg_nr = CPDLC_UL112_INCR_SPD_TO_spd_OR_GREATER,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "INCREASE SPEED TO [speed] OR GREATER"
    },
    {
	.msg_nr = CPDLC_UL113_RED_SPD_TO_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "REDUCE SPEED TO [speed]"
    },
    {
	.msg_nr = CPDLC_UL114_RED_SPD_TO_spd_OR_LESS,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "REDUCE SPEED TO [speed] OR LESS"
    },
    {
	.msg_nr = CPDLC_UL115_DO_NOT_EXCEED_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_WU,
	.text = "DO NOT EXCEED [speed]"
    },
    {
	.msg_nr = CPDLC_UL116_RESUME_NORMAL_SPD,
	.resp = CPDLC_RESP_WU,
	.text = "RESUME NORMAL SPEED"
    },
    {
	.msg_nr = CPDLC_UL117_CONTACT_icaounitname_freq,
	.num_args = 2,
	.args = { CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU,
	.text = "CONTACT [icaounitname] [frequency]"
    },
    {
	.msg_nr = CPDLC_UL118_AT_pos_CONTACT_icaounitname_freq,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU,
	.text = "AT [position] CONTACT [icaounitname] [frequency]"
    },
    {
	.msg_nr = CPDLC_UL119_AT_time_CONTACT_icaounitname_freq,
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU,
	.text = "AT [time] CONTACT [icaounitname] [frequency]"
    },
    {
	.msg_nr = CPDLC_UL120_MONITOR_icaounitname_freq,
	.num_args = 2,
	.args = { CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU,
	.text = "MONITOR [icaounitname] [frequency]"
    },
    {
	.msg_nr = CPDLC_UL121_AT_pos_MONITOR_icaounitname_freq,
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU,
	.text = "AT [position] MONITOR [icaounitname] [frequency]"
    },
    {
	.msg_nr = CPDLC_UL122_AT_time_MONITOR_icaounitname_freq,
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ICAONAME, CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_WU,
	.text = "AT [time] CONTACT [icaounitname] [frequency]"
    },
    {
	.msg_nr = CPDLC_UL123_SQUAWK_code,
	.num_args = 1,
	.args = { CPDLC_ARG_SQUAWK },
	.resp = CPDLC_RESP_WU,
	.text = "SQUAWK [beacon code]"
    },
    {
	.msg_nr = CPDLC_UL124_STOP_SQUAWK,
	.resp = CPDLC_RESP_WU,
	.text = "STOP SQUAWK"
    },
    {
	.msg_nr = CPDLC_UL125_SQUAWK_ALT,
	.resp = CPDLC_RESP_WU,
	.text = "SQUAWK ALTITUDE"
    },
    {
	.msg_nr = CPDLC_UL126_STOP_ALT_SQUAWK,
	.resp = CPDLC_RESP_WU,
	.text = "STOP ALTITUDE SQUAWK"
    },
    {
	.msg_nr = CPDLC_UL127_REPORT_BACK_ON_ROUTE,
	.resp = CPDLC_RESP_R,
	.text = "REPORT BACK ON ROUTE"
    },
    {
	.msg_nr = CPDLC_UL128_REPORT_LEAVING_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "REPORT LEAVING [altitude]"
    },
    {
	.msg_nr = CPDLC_UL129_REPORT_LEVEL_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "REPORT LEVEL [altitude]"
    },
    {
	.msg_nr = CPDLC_UL130_REPORT_PASSING_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "REPORT PASSING [position]"
    },
    {
	.msg_nr = CPDLC_UL131_REPORT_RMNG_FUEL_SOULS_ON_BOARD,
	.resp = CPDLC_RESP_NE,
	.text = "REPORT REMAINING FUEL AND SOULS ON BOARD"
    },
    {
	.msg_nr = CPDLC_UL132_CONFIRM_POSITION,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM POSITION"
    },
    {
	.msg_nr = CPDLC_UL133_CONFIRM_ALT,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM ALTITUDE"
    },
    {
	.msg_nr = CPDLC_UL134_CONFIRM_SPD,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM SPEED"
    },
    {
	.msg_nr = CPDLC_UL135_CONFIRM_ASSIGNED_ALT,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM ASSIGNED ALTITUDE"
    },
    {
	.msg_nr = CPDLC_UL136_CONFIRM_ASSIGNED_SPD,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM ASSIGNED SPEED"
    },
    {
	.msg_nr = CPDLC_UL137_CONFIRM_ASSIGNED_ROUTE,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM ASSIGNED ROUTE"
    },
    {
	.msg_nr = CPDLC_UL138_CONFIRM_TIME_OVER_REPORTED_WPT,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM TIME OVER REPORTED WAYPOINT"
    },
    {
	.msg_nr = CPDLC_UL139_CONFIRM_REPORTED_WPT,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM REPORTED WAYPOINT"
    },
    {
	.msg_nr = CPDLC_UL140_CONFIRM_NEXT_WPT,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM NEXT WAYPOINT"
    },
    {
	.msg_nr = CPDLC_UL141_CONFIRM_NEXT_WPT_ETA,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM NEXT WAYPOINT ETA"
    },
    {
	.msg_nr = CPDLC_UL142_CONFIRM_ENSUING_WPT,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM ENSUING WAYPOINT"
    },
    {
	.msg_nr = CPDLC_UL143_CONFIRM_REQ,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM REQUEST"
    },
    {
	.msg_nr = CPDLC_UL144_CONFIRM_SQUAWK,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM SQUAWK"
    },
    {
	.msg_nr = CPDLC_UL145_CONFIRM_HDG,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM HEADING"
    },
    {
	.msg_nr = CPDLC_UL146_CONFIRM_GND_TRK,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM GROUND TRACK"
    },
    {
	.msg_nr = CPDLC_UL147_REQUEST_POS_REPORT,
	.resp = CPDLC_RESP_NE,
	.text = "REQUEST POSITION REPORT"
    },
    {
	.msg_nr = CPDLC_UL148_WHEN_CAN_YOU_ACPT_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_NE,
	.text = "WHEN CAN YOU ACCEPT [altitude]"
    },
    {
	.msg_nr = CPDLC_UL149_CAN_YOU_ACPT_alt_AT_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_AN,
	.text = "CAN YOU ACCEPT [altitude] AT [position]"
    },
    {
	.msg_nr = CPDLC_UL150_CAN_YOU_ACPT_alt_AT_time,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_AN,
	.text = "CAN YOU ACCEPT [altitude] AT [time]"
    },
    {
	.msg_nr = CPDLC_UL151_WHEN_CAN_YOU_ACPT_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_AN,
	.text = "WHEN CAN YOU ACCEPT [speed]"
    },
    {
	.msg_nr = CPDLC_UL152_WHEN_CAN_YOU_ACPT_dir_dist_OFFSET,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_AN,
	.text = "WHEN CAN YOU ACCEPT [direction] [distance] OFFSET"
    },
    {
	.msg_nr = CPDLC_UL153_ALTIMETER_baro,
	.num_args = 1,
	.args = { CPDLC_ARG_BARO },
	.resp = CPDLC_RESP_R,
	.text = "ALTIMETER [altimeter]"
    },
    {
	.msg_nr = CPDLC_UL154_RDR_SVC_TERM,
	.resp = CPDLC_RESP_R,
	.text = "RADAR SERVICES TERMINATED"
    },
    {
	.msg_nr = CPDLC_UL155_RDR_CTC_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.text = "RADAR CONTACT [position]"
    },
    {
	.msg_nr = CPDLC_UL155_RDR_CTC_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_R,
	.text = "RADAR CONTACT [position]"
    },
    {
	.msg_nr = CPDLC_UL156_RDR_CTC_LOST,
	.resp = CPDLC_RESP_R,
	.text = "RADAR CONTACT LOST"
    },
    {
	.msg_nr = CPDLC_UL157_CHECK_STUCK_MIC,
	.resp = CPDLC_RESP_R,
	.text = "CHECK STUCK MICROPHONE"
    },
    {
	.msg_nr = CPDLC_UL158_ATIS_code,
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_R,
	.text = "ATIS [atis code]"
    },
    {
	.msg_nr = CPDLC_UL159_ERROR_description,
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_NE,
	.text = "ERROR [error information]"
    },
    {
	.msg_nr = CPDLC_UL160_NEXT_DATA_AUTHORITY_id,
	.num_args = 1,
	.args = { CPDLC_ARG_ICAONAME },
	.resp = CPDLC_RESP_NE,
	.text = "NEXT DATA AUTHORITY [facility designation]"
    },
    {
	.msg_nr = CPDLC_UL161_END_SVC,
	.resp = CPDLC_RESP_NE,
	.text = "END SERVICE"
    },
    {
	.msg_nr = CPDLC_UL162_SVC_UNAVAIL,
	.resp = CPDLC_RESP_NE,
	.text = "SERVICE UNAVAILABLE"
    },
    {
	.msg_nr = CPDLC_UL163_FACILITY_designation_tp4table,
	.num_args = 2,
	.args = { CPDLC_ARG_ICAONAME, CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_NE,
	.text = "[icao facility designation] [tp4Table]"
    },
    {
	.msg_nr = CPDLC_UL164_WHEN_RDY,
	.resp = CPDLC_RESP_NE,
	.text = "WHEN READY"
    },
    {
	.msg_nr = CPDLC_UL165_THEN,
	.resp = CPDLC_RESP_NE,
	.text = "THEN"
    },
    {
	.msg_nr = CPDLC_UL166_DUE_TO_TFC,
	.resp = CPDLC_RESP_NE,
	.text = "DUE TO TRAFFIC"
    },
    {
	.msg_nr = CPDLC_UL167_DUE_TO_AIRSPACE_RESTR,
	.resp = CPDLC_RESP_NE,
	.text = "DUE TO AIRSPACE RESTRICTION"
    },
    {
	.msg_nr = CPDLC_UL168_DISREGARD,
	.resp = CPDLC_RESP_R,
	.text = "DISREGARD"
    },
    {
	.msg_nr = CPDLC_UL169_FREETEXT_NORMAL_text,
	.resp = CPDLC_RESP_R,
	.text = "[freetext]"
    },
    {
	.msg_nr = CPDLC_UL170_FREETEXT_DISTRESS_text,
	.resp = CPDLC_RESP_R,
	.text = "[freetext]"
    },
    {
	.msg_nr = CPDLC_UL171_CLB_AT_vvi_MIN,
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	.resp = CPDLC_RESP_WU,
	.text = "CLIMB AT [vertical rate] MINIMUM"
    },
    {
	.msg_nr = CPDLC_UL172_CLB_AT_vvi_MAX,
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	.resp = CPDLC_RESP_WU,
	.text = "CLIMB AT [vertical rate] MAXIMUM"
    },
    {
	.msg_nr = CPDLC_UL173_DES_AT_vvi_MIN,
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	.resp = CPDLC_RESP_WU,
	.text = "DESCEND AT [vertical rate] MINIMUM"
    },
    {
	.msg_nr = CPDLC_UL174_DES_AT_vvi_MAX,
	.num_args = 1,
	.args = { CPDLC_ARG_VVI },
	.resp = CPDLC_RESP_WU,
	.text = "DESCEND AT [vertical rate] MAXIMUM"
    },
    {
	.msg_nr = CPDLC_UL175_REPORT_REACHING_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "REPORT REACHING [altitude]"
    },
    {
	.msg_nr = CPDLC_UL176_MAINT_OWN_SEPARATION_AND_VMC,
	.resp = CPDLC_RESP_WU,
	.text = "MAINTAIN OWN SEPARATION AND VMC"
    },
    {
	.msg_nr = CPDLC_UL177_AT_PILOTS_DISCRETION,
	.resp = CPDLC_RESP_R,
	.text = "AT PILOTS DISCRETION"
    },
    {
	.msg_nr = CPDLC_UL179_SQUAWK_IDENT,
	.resp = CPDLC_RESP_WU,
	.text = "SQUAWK IDENT"
    },
    {
	.msg_nr = CPDLC_UL180_REPORT_REACHING_BLOCK_alt_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_R,
	.text = "REPORT REACHING BLOCK [altitude] TO [altitude]"
    },
    {
	.msg_nr = CPDLC_UL181_REPORT_DISTANCE_tofrom_pos,
	.num_args = 2,
	.args = { CPDLC_ARG_TOFROM, CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_NE,
	.text = "REPORT DISTANCE [to/from] [position]"
    },
    {
	.msg_nr = CPDLC_UL182_CONFIRM_ATIS_CODE,
	.resp = CPDLC_RESP_NE,
	.text = "CONFIRM ATIS CODE"
    }
};

const cpdlc_msg_info_t cpdlc_dl_infos[] = {
    {
	.msg_nr = CPDLC_DL0_WILCO,
	.resp = CPDLC_RESP_N,
	.text = "WILCO"
    },
    {
	.msg_nr = CPDLC_DL1_UNABLE,
	.resp = CPDLC_RESP_N,
	.text = "UNABLE"
    },
    {
	.msg_nr = CPDLC_DL2_STANDBY,
	.resp = CPDLC_RESP_N,
	.text = "STANDBY"
    },
    {
	.msg_nr = CPDLC_DL3_ROGER,
	.resp = CPDLC_RESP_N,
	.text = "ROGER"
    },
    {
	.msg_nr = CPDLC_DL4_AFFIRM,
	.resp = CPDLC_RESP_N,
	.text = "AFFIRMATIVE"
    },
    {
	.msg_nr = CPDLC_DL5_NEGATIVE,
	.resp = CPDLC_RESP_N,
	.text = "NEGATIVE"
    },
    {
	.msg_nr = CPDLC_DL6_REQ_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST [altitude]"
    },
    {
	.msg_nr = CPDLC_DL7_REQ_BLOCK_alt_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST BLOCK [altitude] TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL8_REQ_CRZ_CLB_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST CRUISE CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL9_REQ_CLB_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL10_REQ_DES_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST DESCENT TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL11_AT_pos_REQ_CLB_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "AT [position] REQUEST CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL12_AT_pos_REQ_DES_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "AT [position] REQUEST DESCENT TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL13_AT_time_REQ_CLB_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "AT [time] REQUEST CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL14_AT_time_REQ_DES_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "AT [time] REQUEST DESCENT TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL15_REQ_OFFSET_dir_dist_OF_ROUTE,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST OFFSET [direction] [distance] OF ROUTE"
    },
    {
	.msg_nr = CPDLC_DL16_AT_pos_REQ_OFFSET_dir_dist_OF_ROUTE,
	.num_args = 3,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_Y,
	.text = "AT [position] REQUEST OFFSET [direction] [distance] OF ROUTE"
    },
    {
	.msg_nr = CPDLC_DL17_AT_time_REQ_OFFSET_dir_dist_OF_ROUTE,
	.num_args = 3,
	.args = { CPDLC_ARG_TIME, CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_Y,
	.text = "AT [time] REQUEST OFFSET [direction] [distance] OF ROUTE"
    },
    {
	.msg_nr = CPDLC_DL18_REQ_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST [speed]"
    },
    {
	.msg_nr = CPDLC_DL19_REQ_spd_TO_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST [speed] TO [speed]"
    },
    {
	.msg_nr = CPDLC_DL20_REQ_VOICE_CTC,
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST VOICE CONTACT"
    },
    {
	.msg_nr = CPDLC_DL21_REQ_VOICE_CTC_ON_freq,
	.num_args = 1,
	.args = { CPDLC_ARG_FREQUENCY },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST VOICE CONTACT ON [frequency]"
    },
    {
	.msg_nr = CPDLC_DL22_REQ_DIR_TO_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST DIRECT TO [position]"
    },
    {
	.msg_nr = CPDLC_DL23_REQ_proc,
	.num_args = 1,
	.args = { CPDLC_ARG_PROCEDURE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST [procedure name]"
    },
    {
	.msg_nr = CPDLC_DL24_REQ_route,
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST [route clearance]"
    },
    {
	.msg_nr = CPDLC_DL25_REQ_PDC,
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST CLEARANCE"
    },
    {
	.msg_nr = CPDLC_DL26_REQ_WX_DEVIATION_TO_pos_VIA_route,
	.num_args = 2,
	.args = { CPDLC_ARG_POSITION, CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST WEATHER DEVIATION TO [position] VIA [route clearance]"
    },
    {
	.msg_nr = CPDLC_DL27_REQ_WX_DEVIATION_UP_TO_dir_dist_OF_ROUTE,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST WEATHER DEVIATION UP TO [direction] "
	    "[distance offset] OF ROUTE"
    },
    {
	.msg_nr = CPDLC_DL28_LEAVING_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "LEAVING [altitude]"
    },
    {
	.msg_nr = CPDLC_DL29_CLIMBING_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "CLIMBING TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL30_DESCENDING_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "DESCENDING TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL31_PASSING_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N,
	.text = "PASSING [position]"
    },
    {
	.msg_nr = CPDLC_DL32_PRESENT_ALT_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "PASSING [position]"
    },
    {
	.msg_nr = CPDLC_DL33_PRESENT_POS_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N,
	.text = "PRESENT POSITION [position]"
    },
    {
	.msg_nr = CPDLC_DL34_PRESENT_SPD_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_N,
	.text = "PRESENT SPEED [speed]"
    },
    {
	.msg_nr = CPDLC_DL35_PRESENT_HDG_deg,
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_N,
	.text = "PRESENT HEADING [degrees]"
    },
    {
	.msg_nr = CPDLC_DL36_PRESENT_GND_TRK_deg,
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_N,
	.text = "PRESENT GROUND TRACK [degrees]"
    },
    {
	.msg_nr = CPDLC_DL37_LEVEL_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "LEVEL [altitude]"
    },
    {
	.msg_nr = CPDLC_DL38_ASSIGNED_ALT_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "ASSIGNED ALTITUDE [altitude]"
    },
    {
	.msg_nr = CPDLC_DL39_ASSIGNED_SPD_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_N,
	.text = "ASSIGNED SPEED [speed]"
    },
    {
	.msg_nr = CPDLC_DL40_ASSIGNED_ROUTE_route,
	.num_args = 1,
	.args = { CPDLC_ARG_ROUTE },
	.resp = CPDLC_RESP_N,
	.text = "ASSIGNED ROUTE [route]"
    },
    {
	.msg_nr = CPDLC_DL41_BACK_ON_ROUTE,
	.resp = CPDLC_RESP_N,
	.text = "BACK ON ROUTE"
    },
    {
	.msg_nr = CPDLC_DL42_NEXT_WPT_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N,
	.text = "NEXT WAYPOINT [position]"
    },
    {
	.msg_nr = CPDLC_DL43_NEXT_WPT_ETA_time,
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N,
	.text = "NEXT WAYPOINT [time]"
    },
    {
	.msg_nr = CPDLC_DL44_ENSUING_WPT_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N,
	.text = "ENSUING WAYPOINT [position]"
    },
    {
	.msg_nr = CPDLC_DL45_REPORTED_WPT_pos,
	.num_args = 1,
	.args = { CPDLC_ARG_POSITION },
	.resp = CPDLC_RESP_N,
	.text = "REPORTED WAYPOINT [position]"
    },
    {
	.msg_nr = CPDLC_DL46_REPORTED_WPT_time,
	.num_args = 1,
	.args = { CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N,
	.text = "REPORTED WAYPOINT [time]"
    },
    {
	.msg_nr = CPDLC_DL47_SQUAWKING_code,
	.num_args = 1,
	.args = { CPDLC_ARG_SQUAWK },
	.resp = CPDLC_RESP_N,
	.text = "SQUAWKING [beacon code]"
    },
    {
	.msg_nr = CPDLC_DL48_POS_REPORT_posreport,
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.text = "POSITION REPORT [posreport]"
    },
    {
	.msg_nr = CPDLC_DL49_WHEN_CAN_WE_EXPCT_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_Y,
	.text = "WHEN CAN WE EXPECT [speed]"
    },
    {
	.msg_nr = CPDLC_DL50_WHEN_CAN_WE_EXPCT_spd_TO_spd,
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_Y,
	.text = "WHEN CAN WE EXPECT [speed] TO [speed]"
    },
    {
	.msg_nr = CPDLC_DL51_WHEN_CAN_WE_EXPCT_BACK_ON_ROUTE,
	.resp = CPDLC_RESP_Y,
	.text = "WHEN CAN WE EXPECT BACK ON ROUTE"
    },
    {
	.msg_nr = CPDLC_DL52_WHEN_CAN_WE_EXPECT_LOWER_ALT,
	.resp = CPDLC_RESP_Y,
	.text = "WHEN CAN WE EXPECT LOWER ALTITUDE"
    },
    {
	.msg_nr = CPDLC_DL53_WHEN_CAN_WE_EXPECT_HIGHER_ALT,
	.resp = CPDLC_RESP_Y,
	.text = "WHEN CAN WE EXPECT HIGHER ALTITUDE"
    },
    {
	.msg_nr = CPDLC_DL54_WHEN_CAN_WE_EXPECT_CRZ_CLB_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "WHEN CAN WE EXPECT CRUISE CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL62_ERROR_errorinfo,
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.text = "ERROR [error information]"
    },
    {
	.msg_nr = CPDLC_DL63_NOT_CURRENT_DATA_AUTHORITY,
	.resp = CPDLC_RESP_N,
	.text = "NOT CURRENT DATA AUTHORITY"
    },
    {
	.msg_nr = CPDLC_DL64_CURRENT_DATA_AUTHORITY_id,
	.num_args = 1,
	.args = { CPDLC_ARG_ICAONAME },
	.resp = CPDLC_RESP_N,
	.text = "[icao facility designation]"
    },
    {
	.msg_nr = CPDLC_DL65_DUE_TO_WX,
	.resp = CPDLC_RESP_N,
	.text = "DUE TO WEATHER"
    },
    {
	.msg_nr = CPDLC_DL66_DUE_TO_ACFT_PERF,
	.resp = CPDLC_RESP_N,
	.text = "DUE TO AIRCRAFT PERFORMANCE"
    },
    {
	.msg_nr = CPDLC_DL67_FREETEXT_NORMAL_text,
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.text = "[freetext]"
    },
    {
	.msg_nr = CPDLC_DL67b_WE_CAN_ACPT_alt_AT_time,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N,
	.text = "WE CAN ACCEPT [altitude] AT [time]"
    },
    {
	.msg_nr = CPDLC_DL67c_WE_CAN_ACPT_spd_AT_time,
	.num_args = 2,
	.args = { CPDLC_ARG_SPEED, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N,
	.text = "WE CAN ACCEPT [speed] AT [time]"
    },
    {
	.msg_nr = CPDLC_DL67d_WE_CAN_ACPT_dir_dist_AT_time,
	.num_args = 3,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE, CPDLC_ARG_TIME },
	.resp = CPDLC_RESP_N,
	.text = "WE CAN ACCEPT [direction] [distance offset] AT [time]"
    },
    {
	.msg_nr = CPDLC_DL67e_WE_CANNOT_ACPT_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "WE CANNOT ACCEPT [altitude]"
    },
    {
	.msg_nr = CPDLC_DL67f_WE_CANNOT_ACPT_spd,
	.num_args = 1,
	.args = { CPDLC_ARG_SPEED },
	.resp = CPDLC_RESP_N,
	.text = "WE CANNOT ACCEPT [speed]"
    },
    {
	.msg_nr = CPDLC_DL67g_WE_CANNOT_ACPT_dir_dist,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_N,
	.text = "WE CANNOT ACCEPT [direction] [distance offset]"
    },
    {
	.msg_nr = CPDLC_DL67h_WHEN_CAN_WE_EXPCT_CLB_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "WHEN CAN WE EXPECT CLIMB TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL67i_WHEN_CAN_WE_EXPCT_DES_TO_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "WHEN CAN WE EXPECT DESCENT TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL68_FREETEXT_DISTRESS_text,
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.text = "[freetext]"
    },
    {
	.msg_nr = CPDLC_DL70_REQ_HDG_deg,
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST HEADING [degrees]"
    },
    {
	.msg_nr = CPDLC_DL71_REQ_GND_TRK_deg,
	.num_args = 1,
	.args = { CPDLC_ARG_DEGREES },
	.resp = CPDLC_RESP_Y,
	.text = "REQUEST GROUND TRACK [degrees]"
    },
    {
	.msg_nr = CPDLC_DL72_REACHING_alt,
	.num_args = 1,
	.args = { CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_Y,
	.text = "REACHING [altitude]"
    },
    {
	.msg_nr = CPDLC_DL73_VERSION_number,
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.text = "[version nr]"
    },
    {
	.msg_nr = CPDLC_DL74_MAINT_OWN_SEPARATION_AND_VMC,
	.resp = CPDLC_RESP_N,
	.text = "MAINTAIN OWN SEPARATION AND VMC"
    },
    {
	.msg_nr = CPDLC_DL75_AT_PILOTS_DISCRETION,
	.resp = CPDLC_RESP_N,
	.text = "AT PILOTS DISCRETION"
    },
    {
	.msg_nr = CPDLC_DL76_REACHING_BLOCK_alt_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "REACHING BLOCK [altitude] TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL77_ASSIGNED_BLOCK_alt_TO_alt,
	.num_args = 2,
	.args = { CPDLC_ARG_ALTITUDE, CPDLC_ARG_ALTITUDE },
	.resp = CPDLC_RESP_N,
	.text = "ASSIGNED BLOCK [altitude] TO [altitude]"
    },
    {
	.msg_nr = CPDLC_DL78_AT_time_dist_tofrom_pos,
	.num_args = 4,
	.args = {
	    CPDLC_ARG_TIME, CPDLC_ARG_DISTANCE, CPDLC_ARG_TOFROM,
	    CPDLC_ARG_POSITION
	},
	.resp = CPDLC_RESP_N,
	.text = "AT [time] [distance] [to/from] [position]"
    },
    {
	.msg_nr = CPDLC_DL79_ATIS_code,
	.num_args = 1,
	.args = { CPDLC_ARG_FREETEXT },
	.resp = CPDLC_RESP_N,
	.text = "ATIS [atis code]"
    },
    {
	.msg_nr = CPDLC_DL80_DEVIATING_dir_dist_OF_ROUTE,
	.num_args = 2,
	.args = { CPDLC_ARG_DIRECTION, CPDLC_ARG_DISTANCE },
	.resp = CPDLC_RESP_N,
	.text = "DEVIATING [direction] [distance offset] OF ROUTE"
    }
};
