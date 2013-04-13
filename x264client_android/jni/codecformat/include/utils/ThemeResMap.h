/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Theme Manager Map class.
//

#ifndef __LIBS_THEMERESMAP_H
#define __LIBS_THEMERESMAP_H

#include <utils/Asset.h>
#include <utils/AssetDir.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/String16.h>
#include <utils/ZipFileRO.h>
#include <utils/threads.h>

namespace android {
	struct ThemeFileList {
		char file[100];
	};

	struct ThemePathMap {
		char path[100];
		int pathArrayIndex;
	};
	
	/**
	 * Asset file list for framework resources.
	 */
	ThemeFileList mFrameworkList[] = {
		//title bar
		"activity_title_bar.9.png",

		//check box
		"btn_check_label_background.9.png",
		"btn_check_off.png",
		"btn_check_off_disable.png",
		"btn_check_off_disable_focused.png",
		"btn_check_off_pressed.png",
		"btn_check_off_selected.png",
		"btn_check_on.png",
		"btn_check_on_disable.png",
		"btn_check_on_disable_focused.png",
		"btn_check_on_pressed.png",
		"btn_check_on_selected.png",

		//circle button
		"btn_circle_disable.png",
		"btn_circle_disable_focused.png",
		"btn_circle_normal.png",
		"btn_circle_pressed.png",
		"btn_circle_selected.png",
		"ic_btn_round_more_disabled.png",
		"ic_btn_round_more_normal.png",

		//button normal size
		"btn_default_normal.9.png",
		"btn_default_normal_disable.9.png",
		"btn_default_normal_disable_focused.9.png",
		"btn_default_pressed.9.png",
		"btn_default_selected.9.png",

		//button small size
		"btn_default_small_normal.9.png",
		"btn_default_small_normal_disable.9.png",
		"btn_default_small_normal_disable_focused.9.png",
		"btn_default_small_pressed.9.png",
		"btn_default_small_selected.9.png",

		//button transparent style
		"btn_default_transparent_normal.9.png",

		//button inset style
		"btn_erase_default.9.png",
		"btn_erase_pressed.9.png",
		"btn_erase_selected.9.png",

		//radio button
		"btn_radio_label_background.9.png",
		"btn_radio_off.png",
		"btn_radio_off_pressed.png",
		"btn_radio_off_selected.png",
		"btn_radio_on.png",
		"btn_radio_on_pressed.png",
		"btn_radio_on_selected.png",

		//toggle button
		"btn_toggle_off.9.png",
		"btn_toggle_on.9.png",

		//dialog background
		"popup_bottom_bright.9.png",
		"popup_bottom_dark.9.png",
		"popup_bottom_medium.9.png",
		"popup_center_bright.9.png",
		"popup_center_dark.9.png",
		"popup_full_bright.9.png",
		"popup_full_dark.9.png",
		"popup_top_bright.9.png",
		"popup_top_dark.9.png",
		"divider_horizontal_dark.9.png",

		//horizontal indeterminate progressbar
		"progressbar_indeterminate1.png",
		"progressbar_indeterminate2.png",
		"progressbar_indeterminate3.png",

		//horizontal progressbar drawable
		"progress_horizontal.xml",

		//circle progressbar
		"spinner_black_16.png",
		"spinner_black_20.png",
		"spinner_black_48.png",
		"spinner_black_76.png",
		"spinner_white_16.png",
		"spinner_white_48.png",
		"spinner_white_76.png",

		//edit text
		"textfield_default.9.png",
		"textfield_disabled.9.png",
		"textfield_disabled_selected.9.png",
		"textfield_pressed.9.png",
		"textfield_selected.9.png",

		//toast
		"toast_frame.9.png",

		//button bar
		"bottom_bar.png",

		//spinner
		"btn_dropdown_disabled.9.png",
		"btn_dropdown_disabled_focused.9.png",
		"btn_dropdown_normal.9.png",
		"btn_dropdown_pressed.9.png",
		"btn_dropdown_selected.9.png",

		//menu
		"divider_vertical_dark.9.png",
		"menu_background.9.png",
		"menu_background_fill_parent_width.9.png",
		"highlight_disabled.9.png",
		"menu_separator.9.png",
		"menu_submenu_background.9.png",
					
		//overscroll glowedge
		"overscroll_edge.png",
		"overscroll_glow.png",

		//scrollbar
		"scrollbar_handle_accelerated_anim2.9.png",
		"scrollbar_handle_horizontal.9.png",
		"scrollbar_handle_vertical.9.png",

		//lockscreen & slidingtab & phone
		"ic_jog_dial_sound_off.png",
		"ic_jog_dial_sound_on.png",
		"ic_jog_dial_unlock.png",
		"ic_jog_dial_vibrate_on.png",
		"jog_tab_bar_left_end_confirm_gray.9.png",
		"jog_tab_bar_left_end_confirm_green.9.png",
		"jog_tab_bar_left_end_confirm_red.9.png",
		"jog_tab_bar_left_end_confirm_yellow.9.png",
		"jog_tab_bar_left_end_normal.9.png",
		"jog_tab_bar_left_end_pressed.9.png",
		"jog_tab_bar_right_end_confirm_gray.9.png",
		"jog_tab_bar_right_end_confirm_green.9.png",
		"jog_tab_bar_right_end_confirm_red.9.png",
		"jog_tab_bar_right_end_confirm_yellow.9.png",
		"jog_tab_bar_right_end_normal.9.png",
		"jog_tab_bar_right_end_pressed.9.png",
		"jog_tab_left_confirm_gray.png",
		"jog_tab_left_confirm_green.png",
		"jog_tab_left_confirm_red.png",
		"jog_tab_left_confirm_yellow.png",
		"jog_tab_left_normal.png",
		"jog_tab_left_pressed.png",
		"jog_tab_right_confirm_gray.png",
		"jog_tab_right_confirm_green.png",
		"jog_tab_right_confirm_red.png",
		"jog_tab_right_confirm_yellow.png",
		"jog_tab_right_normal.png",
		"jog_tab_right_pressed.png",
		"jog_tab_target_gray.png",
		"jog_tab_target_green.png",
		"jog_tab_target_red.png",
		"jog_tab_target_yellow.png",
		
		//For Theme Wallpaper
		"themewallpaper.png",

		//For Theme Preview
		"previewtheme.png",

		//For Lock Screen Wallpaper
		"lockwallpaper.png",
	};
	
	
	/**
	 * Asset file list for Launcher2.
	 */
	ThemeFileList mLauncherList[] = {
		// For launcher folder
		"box_launcher_bottom.9.png",
		"box_launcher_top_normal.9.png",
		"box_launcher_top_pressed.9.png",
		"box_launcher_top_selected.9.png",
		
		// For hotseat buttons
		"trashcan.png",
		"trashcan_hover.png",
		"all_apps_button_focused.png",
		"all_apps_button_normal.png",
		"all_apps_button_pressed.png",
		"hotseat_bg_center.9.png",
		"hotseat_bg_left.9.png",
		"hotseat_bg_right.9.png",
		"hotseat_browser_focused.png",
		"hotseat_browser_normal.png",
		"hotseat_browser_pressed.png",
		"hotseat_phone_normal.png",
		"hotseat_phone_focused.png",
		"hotseat_phone_pressed.png",
		"ic_home_arrows_1_focus.png",
		"ic_home_arrows_1_focus_right.png",
		"ic_home_arrows_1_normal.png",
		"ic_home_arrows_1_normal_right.png",
		"ic_home_arrows_1_press.png",
		"ic_home_arrows_1_press_right.png",
		"ic_home_arrows_2_focus.png",
		"ic_home_arrows_2_focus_right.png",
		"ic_home_arrows_2_normal.png",
		"ic_home_arrows_2_normal_right.png",
		"ic_home_arrows_2_press.png",
		"ic_home_arrows_2_press_right.png",
		"ic_home_arrows_3_focus.png",
		"ic_home_arrows_3_focus_right.png",
		"ic_home_arrows_3_normal.png",
		"ic_home_arrows_3_normal_right.png",
		"ic_home_arrows_3_press.png",
		"ic_home_arrows_3_press_right.png",
		"ic_home_arrows_4_focus.png",
		"ic_home_arrows_4_focus_right.png",
		"ic_home_arrows_4_normal.png",
		"ic_home_arrows_4_normal_right.png",
		"ic_home_arrows_4_press.png",
		"ic_home_arrows_4_press_right.png",
		"home_button_focused.png",
		"home_button_normal.png",
		"home_button_pressed.png",
		"theme_applist_bkg.png",
	};

	/**
	 * Asset file list for LauncherPlus.
	 */
	ThemeFileList mLauncherPlusList[] = {
		// For  launcher plus folder
		"box_launcher_bottom.9.png",
		"box_launcher_top_normal.9.png",
		"box_launcher_top_pressed.9.png",
		"box_launcher_top_selected.9.png",
		
		// For hot seat buttons
		"trashcan.png",
		"trashcan_hover.png",
		"all_apps_button_focused.png",
		"all_apps_button_normal.png",
		"all_apps_button_pressed.png",
		"bottom_background.png",
		"indicator.png",
		"indicator_big.png",
		"message_normal.png",
		"message_pressed.png",
		"phone_normal.png",
		"phone_pressed.png",
		"home_button_focused.png",
		"home_button_normal.png",
		"home_button_pressed.png",
		"apps_button_normal.png",
		"apps_button_pressed.png",
		"widget_button_normal.png",
		"widget_button_pressed.png",
		"theme_applist_bkg.png",
	};

	/**
	 * Asset file list for bookmark widget.
	 */
	ThemeFileList mMtkBookmarkList[] = {
		"bookmark_title_icon.png",
		"empty_background.png",
		"frame.9.png",
		"grid_background.png",
		"grid_frame_with_black.9.png",
	};
	
	/**
	 * Asset file list for contact widget.
	 */
	ThemeFileList mMtkContactWidgetList[] = {
		"background_call.png",
		"background_card.9.png",
		"background_contact.9.png",
		"background_grid.9.png",
		"background_name.9.png",
		"background_no_favorites.png",
		"background_sim.png",
		"call_by_sim1.png",
		"call_by_sim1_disable.png",
		"call_by_sim1_pressed.png",
		"call_by_sim2.png",
		"call_by_sim2_disable.png",
		"call_by_sim2_pressed.png",
		"contact_photo_default_big.png",
		"contact_photo_default_big_blue.png",
		"contact_photo_default_big_red.png",
		"contact_photo_default_big_yellow.png",
		"contact_photo_default_blue.png",
		"contact_photo_default_red.png",
		"contact_photo_default_yellow.png",
		"icon_back_normal.png",
		"icon_back_pressed.png",
		"icon_call_normal.png",
		"icon_call_pressed.png",
		"icon_email_normal.png",
		"icon_email_pressed.png",
		"icon_mms_normal.png",
		"icon_mms_pressed.png",
	};
	
	/**
	 * Asset file list for message widget.
	 */
	ThemeFileList mMtkMMSWidgetList[] = {
		"attach_frame.png",
		"default_attach_thumbnail.png",
		"default_avatar_blue.png",
		"default_avatar_red.png",
		"default_avatar_yellow.png",
		"setting_add_card.png",
		"setting_add_disable.png",
		"setting_back.png",
		"setting_back_highlight.png",
		"setting_contact_card_bg.png",
		"setting_default_avatar_blue.png",
		"setting_default_avatar_red.png",
		"setting_default_avatar_yellow.png",
		"setting_delete.png",
		"setting_delete_flag.png",
		"setting_delete_flag_press.png",
		"setting_delete_highlight.png",
		"setting_indicator_bg.png",
		"wgt_message_bg.9.png",
		"wgt_message_photo_frame.png",
		"wgt_message_settings_btn.png",
		"wgt_message_settings_highlight_btn.png",
		"wgt_message_setting_bg.9.png",
		"wgt_message_setting_photo_frame.png",
	};
	
	/**
	 * Asset file list for music widget.
	 */
	ThemeFileList mMtkMusicWidgetList[] = {
		"albumart_unknown.png",
		"albumart_veil.png",
		"background.png",
		"background_full.png",
		"next_default.png",
		"next_pressed.png",
		"pause_default.png",
		"pause_pressed.png",
		"play_default.png",
		"play_pressed.png",
		"prev_default.png",
		"prev_pressed.png",
	};
	
	/**
	 * Asset file list for photo widget.
	 */
	ThemeFileList mMtkPhotoWidgetList[] = {
		"back_button1.png",
		"back_button2.png",
		"background2.9.png",
		"background_null.9.png",
		"background_picture.png",
		"background_picture2.9.png",
		"radiobutton_checked.png",
		"radiobutton_normal.png",
		"settings_focus.png",
		"settings_normal.png",
		"shadow.png",
	};
	
	/**
	 * Asset file list for system clock widget.
	 */
	ThemeFileList mMtkSystemClockWidgetList[] = {
		"wgt_clock_style3_dial.png",
		"wgt_clock_style3_hour.png",
		"wgt_clock_style3_minute.png",
	};
	
	/**
	 * Asset file list for weather widget.
	 */
	ThemeFileList mMtkWeatherWidgetList[] = {
		"bg1.9.png",
		"bg11.9.png",
		"bg2.9.png",
		"bg3.9.png",
		"ic_btn_search.png",
		"ic_dialog_menu_generic.png",
		"ic_menu_add.png",
		"wgt_weather_background.9.png",
		"wgt_weather_button.9.png",
		"wgt_weather_button_disable.9.png",
		"wgt_weather_button_select.9.png",
		"wgt_weather_ic_arrow_bottom.png",
		"wgt_weather_ic_arrow_bottom_disable.png",
		"wgt_weather_ic_arrow_bottom_select.png",
		"wgt_weather_ic_arrow_top.png",
		"wgt_weather_ic_arrow_top_disable.png",
		"wgt_weather_ic_arrow_top_select.png",
		"wgt_weather_ic_city.png",
		"wgt_weather_ic_more.png",
		"wgt_weather_ic_setting.png",
		"wgt_weather_ic_update.png",
	};

	/**
	 * Asset file list for world clock widget.
	 */
	ThemeFileList mMtkWorldClockWidgetList[] = {
		"ic_menu_new_window.png",
		"wgt_clock_style1_day_dial.png",
		"wgt_clock_style1_day_hat.png",
		"wgt_clock_style1_day_hour.png",
		"wgt_clock_style1_day_minute.png",
		"wgt_clock_style1_night_dial.png",
		"wgt_clock_style1_night_hat.png",
		"wgt_clock_style1_night_hour.png",
		"wgt_clock_style1_night_minute.png",
		"wgt_clock_style_bg.9.png",		
	};

	/**
	 * Asset file list for sns widget. 
	 */
	ThemeFileList mSNSWidgetList[] = {
		"btn_sns_widget_list_hl.9.png",
		"favorite_line.9.png",
		"keyboard_textfield_selected.9.png",
		"no_account_btn_bg_hi.9.png",
		 "no_account_btn_bg.9.png",
		 "sns_popup_message_no.9.png",
		 "sns_widget_background_user_name.9.png",
		 "sns_wigdet_favorite_btn.9.png",
		 "title_bg.png",
		 "wgt_sns_bg.9.png",
		 "wgt_statusbar_bg.9.png",
		 "add_n.png",
		 "add_p.png",
		 "add_s.png",
		 "bg_line.9.png",
		 "btn_sns_widget_hl_1.png",
		 "btn_sns_widget_hl_2.png",
		 "btn_sns_widget_hl_3.png",
		 "btn_sns_widget_hl_4.png",
		 "btn_sns_widget_hl_5.png",
		 "btn_sns_widget_hl_6.png",
		 "btn_sns_widget_hl_7.png",
		 "btn_sns_widget_nor.png",
		 "btn_sns_widget_pressed.png",
		 "btn_widget_headhome_select.png",
		 "color_bar.png",
		 "ic_sns.png",
		 "listview_divider.png",
		 "sns_evnt_default_image.png",
		 "sns_navigation_user_pressed.png",
		 "sns_navigation_user_select.png",
		 "sns_navigation_user.png",
		 "title_bg.png",
		 "wgt_sns_normal_avatar_1.png",
		 "wgt_sns_normal_avatar_2.png",
		 "wgt_sns_normal_avatar_3.png",
		 "wgt_sns_normal_avatar_frame.png",
	};

	/**
	 * Asset file list for sns 24 widget.
	 */
	ThemeFileList mSNSWidget24List[] = {
		"keyboard_textfield_selected.9.png",
		"no_account_bg.png",
		"no_account_btn_bg_di.9.png",
		"no_account_btn_bg_hi.9.png",
		"no_account_btn_bg.9.png",
		"wgt_statusbar_bg.9.png",
		"back_btn_df.png",
		"back_btn_sel.png",
		"delete_btn_df.png",
		"delete_btn_gray.png",
		"delete_btn_sel.png",
		"favorite_disable.png",
		"favorite_enable.png",
		"favorite_gallery_mask.png",
		"ic_sns.png",
		"message_default_avatar_1.png",
		"message_default_avatar_2.png",
		"message_default_avatar_3.png",
		"scroll_indicator_bg.png",
		"setting_add_disable.png",
		"setting_add_flag.png",
		"setting_delete_flag.png",
		"wgt_sns_photo_1.png",
		"wgt_sns_photo_2.png",
		"wgt_sns_photo_3.png",
		"wgt_sns_setting_bg.png",
		"wgt_sns_setting_photo_frame.png",
		"widget_message_account_p.png",
		"widget_message_account.png",
		"widget_message_bg.png",
		"widget_message_camera_hi.png",
		"widget_message_camera.png",
		"widget_message_headportrait_bg.png",
		"widget_message_refresh_anim_1.png",
		"widget_message_refresh_anim_2.png",
		"widget_message_refresh_anim_3.png",
		"widget_message_refresh_anim_4.png",
		"widget_message_refresh_anim_5.png",
		"widget_message_refresh_anim_6.png",
		"widget_message_refresh_anim_7.png",
		"widget_message_setting_hi.png",
		"widget_message_setting.png",
		"widget_message_status_bg.png",
	};

	/**
	 * Asset file list for StatusBar and notification list.
	 */
	ThemeFileList mSystemUIList[] = {
		"statusbar_background.9.png",
		"title_bar_portrait.9.png",
		"status_bar_close_on.9.png",
		"zzz_switch_panel_bkg.png",
	};

	/**
	 * Phone SlidingTab file list.
	 */
	ThemeFileList mPhoneList[] = {
		 "ic_jog_dial_answer.png",
		 "ic_jog_dial_decline.png",
	};

	/**
	 * Asset file list for mediatek resources.
	 */
	ThemeFileList mMtkBaseList[] = {
		//for date and time picker
		"date_picker_frame_bkg.9.png",
		"date_picker_bkg_big_up.9.png",
		"date_picker_bkg_big_bottom.9.png",
		"selected_item.9.png",
	};

	ThemePathMap mPathMap[] = {
		{"/system/framework/framework-res.apk", 0},
		{"/system/app/Launcher2.apk", 1},
		{"/system/app/SystemUI.apk", 2},
		{"/system/app/Phone.apk", 3},
		{"/system/app/SnsWidget.apk", 4},
		{"/system/app/SnsWidget24.apk", 5},
		{"/system/app/LauncherPlus.apk", 6},
		{"/system/app/MtkSystemClockWidget.apk", 7},
		{"/system/app/MtkWorldClockWidget.apk", 8},
		{"/system/app/MtkWeatherWidget.apk", 9},
		{"/system/app/MtkBookmarkWidget.apk", 10},
		{"/system/app/MtkContactWidget.apk", 11},
		{"/system/app/MtkMMSWidget.apk", 12},
		{"/system/app/MtkMusicWidget.apk", 13},
		{"/system/app/MtkPhotoWidget.apk", 14},
		{"/system/framework/mtkBase-res.apk", 15},
	};
}

#endif /* __LIBS_THEMERESMAP_H */
