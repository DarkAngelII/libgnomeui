/* Generated by makeenums.pl */

static const GtkEnumValue _gnome_animator_status_values[] = {
  { GNOME_ANIMATOR_STATUS_STOPPED, "GNOME_ANIMATOR_STATUS_STOPPED", "stopped" },
  { GNOME_ANIMATOR_STATUS_RUNNING, "GNOME_ANIMATOR_STATUS_RUNNING", "running" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_animator_loop_type_values[] = {
  { GNOME_ANIMATOR_LOOP_NONE, "GNOME_ANIMATOR_LOOP_NONE", "none" },
  { GNOME_ANIMATOR_LOOP_RESTART, "GNOME_ANIMATOR_LOOP_RESTART", "restart" },
  { GNOME_ANIMATOR_LOOP_PING_PONG, "GNOME_ANIMATOR_LOOP_PING_PONG", "ping-pong" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_app_widget_position_type_values[] = {
  { GNOME_APP_POS_TOP, "GNOME_APP_POS_TOP", "top" },
  { GNOME_APP_POS_BOTTOM, "GNOME_APP_POS_BOTTOM", "bottom" },
  { GNOME_APP_POS_LEFT, "GNOME_APP_POS_LEFT", "left" },
  { GNOME_APP_POS_RIGHT, "GNOME_APP_POS_RIGHT", "right" },
  { GNOME_APP_POS_FLOATING, "GNOME_APP_POS_FLOATING", "floating" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_ui_info_type_values[] = {
  { GNOME_APP_UI_ENDOFINFO, "GNOME_APP_UI_ENDOFINFO", "endofinfo" },
  { GNOME_APP_UI_ITEM, "GNOME_APP_UI_ITEM", "item" },
  { GNOME_APP_UI_TOGGLEITEM, "GNOME_APP_UI_TOGGLEITEM", "toggleitem" },
  { GNOME_APP_UI_RADIOITEMS, "GNOME_APP_UI_RADIOITEMS", "radioitems" },
  { GNOME_APP_UI_SUBTREE, "GNOME_APP_UI_SUBTREE", "subtree" },
  { GNOME_APP_UI_SEPARATOR, "GNOME_APP_UI_SEPARATOR", "separator" },
  { GNOME_APP_UI_HELP, "GNOME_APP_UI_HELP", "help" },
  { GNOME_APP_UI_JUSTIFY_RIGHT, "GNOME_APP_UI_JUSTIFY_RIGHT", "justify-right" },
  { GNOME_APP_UI_BUILDER_DATA, "GNOME_APP_UI_BUILDER_DATA", "builder-data" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_ui_pixmap_type_values[] = {
  { GNOME_APP_PIXMAP_NONE, "GNOME_APP_PIXMAP_NONE", "none" },
  { GNOME_APP_PIXMAP_STOCK, "GNOME_APP_PIXMAP_STOCK", "stock" },
  { GNOME_APP_PIXMAP_DATA, "GNOME_APP_PIXMAP_DATA", "data" },
  { GNOME_APP_PIXMAP_FILENAME, "GNOME_APP_PIXMAP_FILENAME", "filename" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_calculator_mode_values[] = {
  { GNOME_CALCULATOR_DEG, "GNOME_CALCULATOR_DEG", "deg" },
  { GNOME_CALCULATOR_RAD, "GNOME_CALCULATOR_RAD", "rad" },
  { GNOME_CALCULATOR_GRAD, "GNOME_CALCULATOR_GRAD", "grad" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_interact_style_values[] = {
  { GNOME_INTERACT_NONE, "GNOME_INTERACT_NONE", "none" },
  { GNOME_INTERACT_ERRORS, "GNOME_INTERACT_ERRORS", "errors" },
  { GNOME_INTERACT_ANY, "GNOME_INTERACT_ANY", "any" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_dialog_type_values[] = {
  { GNOME_DIALOG_ERROR, "GNOME_DIALOG_ERROR", "error" },
  { GNOME_DIALOG_NORMAL, "GNOME_DIALOG_NORMAL", "normal" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_save_style_values[] = {
  { GNOME_SAVE_GLOBAL, "GNOME_SAVE_GLOBAL", "global" },
  { GNOME_SAVE_LOCAL, "GNOME_SAVE_LOCAL", "local" },
  { GNOME_SAVE_BOTH, "GNOME_SAVE_BOTH", "both" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_restart_style_values[] = {
  { GNOME_RESTART_IF_RUNNING, "GNOME_RESTART_IF_RUNNING", "if-running" },
  { GNOME_RESTART_ANYWAY, "GNOME_RESTART_ANYWAY", "anyway" },
  { GNOME_RESTART_IMMEDIATELY, "GNOME_RESTART_IMMEDIATELY", "immediately" },
  { GNOME_RESTART_NEVER, "GNOME_RESTART_NEVER", "never" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_client_state_values[] = {
  { GNOME_CLIENT_IDLE, "GNOME_CLIENT_IDLE", "idle" },
  { GNOME_CLIENT_SAVING, "GNOME_CLIENT_SAVING", "saving" },
  { GNOME_CLIENT_WAITING, "GNOME_CLIENT_WAITING", "waiting" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_font_picker_mode_values[] = {
  { GNOME_FONT_PICKER_MODE_PIXMAP, "GNOME_FONT_PICKER_MODE_PIXMAP", "pixmap" },
  { GNOME_FONT_PICKER_MODE_FONT_INFO, "GNOME_FONT_PICKER_MODE_FONT_INFO", "font-info" },
  { GNOME_FONT_PICKER_MODE_USER_WIDGET, "GNOME_FONT_PICKER_MODE_USER_WIDGET", "user-widget" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_icon_list_mode_values[] = {
  { GNOME_ICON_LIST_ICONS, "GNOME_ICON_LIST_ICONS", "icons" },
  { GNOME_ICON_LIST_TEXT_BELOW, "GNOME_ICON_LIST_TEXT_BELOW", "text-below" },
  { GNOME_ICON_LIST_TEXT_RIGHT, "GNOME_ICON_LIST_TEXT_RIGHT", "text-right" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_mdi_mode_values[] = {
  { GNOME_MDI_NOTEBOOK, "GNOME_MDI_NOTEBOOK", "notebook" },
  { GNOME_MDI_TOPLEVEL, "GNOME_MDI_TOPLEVEL", "toplevel" },
  { GNOME_MDI_MODAL, "GNOME_MDI_MODAL", "modal" },
  { GNOME_MDI_DEFAULT_MODE, "GNOME_MDI_DEFAULT_MODE", "default-mode" },
  { GNOME_MDI_REDRAW, "GNOME_MDI_REDRAW", "redraw" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_stock_pixmap_type_values[] = {
  { GNOME_STOCK_PIXMAP_TYPE_NONE, "GNOME_STOCK_PIXMAP_TYPE_NONE", "none" },
  { GNOME_STOCK_PIXMAP_TYPE_DATA, "GNOME_STOCK_PIXMAP_TYPE_DATA", "data" },
  { GNOME_STOCK_PIXMAP_TYPE_FILE, "GNOME_STOCK_PIXMAP_TYPE_FILE", "file" },
  { GNOME_STOCK_PIXMAP_TYPE_PATH, "GNOME_STOCK_PIXMAP_TYPE_PATH", "path" },
  { GNOME_STOCK_PIXMAP_TYPE_WIDGET, "GNOME_STOCK_PIXMAP_TYPE_WIDGET", "widget" },
  { GNOME_STOCK_PIXMAP_TYPE_IMLIB, "GNOME_STOCK_PIXMAP_TYPE_IMLIB", "imlib" },
  { GNOME_STOCK_PIXMAP_TYPE_IMLIB_SCALED, "GNOME_STOCK_PIXMAP_TYPE_IMLIB_SCALED", "imlib-scaled" },
  { GNOME_STOCK_PIXMAP_TYPE_GPIXMAP, "GNOME_STOCK_PIXMAP_TYPE_GPIXMAP", "gpixmap" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_preferences_type_values[] = {
  { GNOME_PREFERENCES_NEVER, "GNOME_PREFERENCES_NEVER", "never" },
  { GNOME_PREFERENCES_USER, "GNOME_PREFERENCES_USER", "user" },
  { GNOME_PREFERENCES_ALWAYS, "GNOME_PREFERENCES_ALWAYS", "always" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_win_layer_values[] = {
  { WIN_LAYER_DESKTOP, "WIN_LAYER_DESKTOP", "desktop" },
  { WIN_LAYER_BELOW, "WIN_LAYER_BELOW", "below" },
  { WIN_LAYER_NORMAL, "WIN_LAYER_NORMAL", "normal" },
  { WIN_LAYER_ONTOP, "WIN_LAYER_ONTOP", "ontop" },
  { WIN_LAYER_DOCK, "WIN_LAYER_DOCK", "dock" },
  { WIN_LAYER_ABOVE_DOCK, "WIN_LAYER_ABOVE_DOCK", "above-dock" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_win_state_values[] = {
  { WIN_STATE_STICKY, "WIN_STATE_STICKY", "sticky" },
  { WIN_STATE_MINIMIZED, "WIN_STATE_MINIMIZED", "minimized" },
  { WIN_STATE_MAXIMIZED_VERT, "WIN_STATE_MAXIMIZED_VERT", "maximized-vert" },
  { WIN_STATE_MAXIMIZED_HORIZ, "WIN_STATE_MAXIMIZED_HORIZ", "maximized-horiz" },
  { WIN_STATE_HIDDEN, "WIN_STATE_HIDDEN", "hidden" },
  { WIN_STATE_SHADED, "WIN_STATE_SHADED", "shaded" },
  { WIN_STATE_HID_WORKSPACE, "WIN_STATE_HID_WORKSPACE", "hid-workspace" },
  { WIN_STATE_HID_TRANSIENT, "WIN_STATE_HID_TRANSIENT", "hid-transient" },
  { WIN_STATE_FIXED_POSITION, "WIN_STATE_FIXED_POSITION", "fixed-position" },
  { WIN_STATE_ARRANGE_IGNORE, "WIN_STATE_ARRANGE_IGNORE", "arrange-ignore" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_win_hints_values[] = {
  { WIN_HINTS_SKIP_FOCUS, "WIN_HINTS_SKIP_FOCUS", "skip-focus" },
  { WIN_HINTS_SKIP_WINLIST, "WIN_HINTS_SKIP_WINLIST", "skip-winlist" },
  { WIN_HINTS_SKIP_TASKBAR, "WIN_HINTS_SKIP_TASKBAR", "skip-taskbar" },
  { WIN_HINTS_GROUP_TRANSIENT, "WIN_HINTS_GROUP_TRANSIENT", "group-transient" },
  { WIN_HINTS_FOCUS_ON_CLICK, "WIN_HINTS_FOCUS_ON_CLICK", "focus-on-click" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gnome_win_app_state_values[] = {
  { WIN_APP_STATE_NONE, "WIN_APP_STATE_NONE", "none" },
  { WIN_APP_STATE_ACTIVE1, "WIN_APP_STATE_ACTIVE1", "active1" },
  { WIN_APP_STATE_ACTIVE2, "WIN_APP_STATE_ACTIVE2", "active2" },
  { WIN_APP_STATE_ERROR1, "WIN_APP_STATE_ERROR1", "error1" },
  { WIN_APP_STATE_ERROR2, "WIN_APP_STATE_ERROR2", "error2" },
  { WIN_APP_STATE_FATAL_ERROR1, "WIN_APP_STATE_FATAL_ERROR1", "fatal-error1" },
  { WIN_APP_STATE_FATAL_ERROR2, "WIN_APP_STATE_FATAL_ERROR2", "fatal-error2" },
  { WIN_APP_STATE_IDLE1, "WIN_APP_STATE_IDLE1", "idle1" },
  { WIN_APP_STATE_IDLE2, "WIN_APP_STATE_IDLE2", "idle2" },
  { WIN_APP_STATE_WAITING1, "WIN_APP_STATE_WAITING1", "waiting1" },
  { WIN_APP_STATE_WAITING2, "WIN_APP_STATE_WAITING2", "waiting2" },
  { WIN_APP_STATE_WORKING1, "WIN_APP_STATE_WORKING1", "working1" },
  { WIN_APP_STATE_WORKING2, "WIN_APP_STATE_WORKING2", "working2" },
  { WIN_APP_STATE_NEED_USER_INPUT1, "WIN_APP_STATE_NEED_USER_INPUT1", "need-user-input1" },
  { WIN_APP_STATE_NEED_USER_INPUT2, "WIN_APP_STATE_NEED_USER_INPUT2", "need-user-input2" },
  { WIN_APP_STATE_STRUGGLING1, "WIN_APP_STATE_STRUGGLING1", "struggling1" },
  { WIN_APP_STATE_STRUGGLING2, "WIN_APP_STATE_STRUGGLING2", "struggling2" },
  { WIN_APP_STATE_DISK_TRAFFIC1, "WIN_APP_STATE_DISK_TRAFFIC1", "disk-traffic1" },
  { WIN_APP_STATE_DISK_TRAFFIC2, "WIN_APP_STATE_DISK_TRAFFIC2", "disk-traffic2" },
  { WIN_APP_STATE_NETWORK_TRAFFIC1, "WIN_APP_STATE_NETWORK_TRAFFIC1", "network-traffic1" },
  { WIN_APP_STATE_NETWORK_TRAFFIC2, "WIN_APP_STATE_NETWORK_TRAFFIC2", "network-traffic2" },
  { WIN_APP_STATE_OVERLOADED1, "WIN_APP_STATE_OVERLOADED1", "overloaded1" },
  { WIN_APP_STATE_OVERLOADED2, "WIN_APP_STATE_OVERLOADED2", "overloaded2" },
  { WIN_APP_STATE_PERCENT000_1, "WIN_APP_STATE_PERCENT000_1", "percent000-1" },
  { WIN_APP_STATE_PERCENT000_2, "WIN_APP_STATE_PERCENT000_2", "percent000-2" },
  { WIN_APP_STATE_PERCENT010_1, "WIN_APP_STATE_PERCENT010_1", "percent010-1" },
  { WIN_APP_STATE_PERCENT010_2, "WIN_APP_STATE_PERCENT010_2", "percent010-2" },
  { WIN_APP_STATE_PERCENT020_1, "WIN_APP_STATE_PERCENT020_1", "percent020-1" },
  { WIN_APP_STATE_PERCENT020_2, "WIN_APP_STATE_PERCENT020_2", "percent020-2" },
  { WIN_APP_STATE_PERCENT030_1, "WIN_APP_STATE_PERCENT030_1", "percent030-1" },
  { WIN_APP_STATE_PERCENT030_2, "WIN_APP_STATE_PERCENT030_2", "percent030-2" },
  { WIN_APP_STATE_PERCENT040_1, "WIN_APP_STATE_PERCENT040_1", "percent040-1" },
  { WIN_APP_STATE_PERCENT040_2, "WIN_APP_STATE_PERCENT040_2", "percent040-2" },
  { WIN_APP_STATE_PERCENT050_1, "WIN_APP_STATE_PERCENT050_1", "percent050-1" },
  { WIN_APP_STATE_PERCENT050_2, "WIN_APP_STATE_PERCENT050_2", "percent050-2" },
  { WIN_APP_STATE_PERCENT060_1, "WIN_APP_STATE_PERCENT060_1", "percent060-1" },
  { WIN_APP_STATE_PERCENT060_2, "WIN_APP_STATE_PERCENT060_2", "percent060-2" },
  { WIN_APP_STATE_PERCENT070_1, "WIN_APP_STATE_PERCENT070_1", "percent070-1" },
  { WIN_APP_STATE_PERCENT070_2, "WIN_APP_STATE_PERCENT070_2", "percent070-2" },
  { WIN_APP_STATE_PERCENT080_1, "WIN_APP_STATE_PERCENT080_1", "percent080-1" },
  { WIN_APP_STATE_PERCENT080_2, "WIN_APP_STATE_PERCENT080_2", "percent080-2" },
  { WIN_APP_STATE_PERCENT090_1, "WIN_APP_STATE_PERCENT090_1", "percent090-1" },
  { WIN_APP_STATE_PERCENT090_2, "WIN_APP_STATE_PERCENT090_2", "percent090-2" },
  { WIN_APP_STATE_PERCENT100_1, "WIN_APP_STATE_PERCENT100_1", "percent100-1" },
  { WIN_APP_STATE_PERCENT100_2, "WIN_APP_STATE_PERCENT100_2", "percent100-2" },
  { 0, NULL, NULL }
};
static const GtkEnumValue _gtk_clock_type_values[] = {
  { GTK_CLOCK_INCREASING, "GTK_CLOCK_INCREASING", "increasing" },
  { GTK_CLOCK_DECREASING, "GTK_CLOCK_DECREASING", "decreasing" },
  { GTK_CLOCK_REALTIME, "GTK_CLOCK_REALTIME", "realtime" },
  { 0, NULL, NULL }
};
