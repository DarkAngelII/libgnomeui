/* Stock icons, buttons, and menu items.
   Copyright (C) 1997, 1998 Free Software Foundation
   All rights reserved.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Eckehard Berns
*/
/*
  @NOTATION@
*/

#ifndef __GNOME_STOCK_ICONS_H__
#define __GNOME_STOCK_ICONS_H__

#include <glib/gmacros.h>
#include <gtk/gtkiconfactory.h>

G_BEGIN_DECLS

#define GNOME_ICON_SIZE_TOOLBAR gnome_icon_size_toolbar
GtkIconSize gnome_icon_size_toolbar;

extern void init_gnome_stock_icons (void);

#define GNOME_STOCK_PIXMAP_SCORES "gnome-stock-pixmap-scores"

#define GNOME_STOCK_PIXMAP_SRCHRPL "gnome-stock-pixmap-srchrpl"
#define GNOME_STOCK_PIXMAP_BACK "gnome-stock-pixmap-back"
#define GNOME_STOCK_PIXMAP_FORWARD "gnome-stock-pixmap-forward"

#define GNOME_STOCK_PIXMAP_TIMER "gnome-stock-pixmap-timer"
#define GNOME_STOCK_PIXMAP_TIMER_STOP "gnome-stock-pixmap-timer-stop"
#define GNOME_STOCK_PIXMAP_MAIL "gnome-stock-pixmap-mail"
#define GNOME_STOCK_PIXMAP_MAIL_RCV "gnome-stock-pixmap-mail-rcv"
#define GNOME_STOCK_PIXMAP_MAIL_SND "gnome-stock-pixmap-mail-snd"
#define GNOME_STOCK_PIXMAP_MAIL_RPL "gnome-stock-pixmap-mail-rpl"
#define GNOME_STOCK_PIXMAP_MAIL_FWD "gnome-stock-pixmap-mail-fwd"
#define GNOME_STOCK_PIXMAP_MAIL_NEW "gnome-stock-pixmap-mail-new"

#define GNOME_STOCK_PIXMAP_TRASH_FULL "gnome-stock-pixmap-trash-full"

#define GNOME_STOCK_PIXMAP_MIC "gnome-stock-pixmap-mic"
#define GNOME_STOCK_PIXMAP_LINE_IN "gnome-stock-pixmap-line-in"

#define GNOME_STOCK_PIXMAP_VOLUME "gnome-stock-pixmap-volume"
#define GNOME_STOCK_PIXMAP_MIDI "gnome-stock-pixmap-midi"
#define GNOME_STOCK_PIXMAP_BOOK_RED "gnome-stock-pixmap-book-red"
#define GNOME_STOCK_PIXMAP_BOOK_GREEN "gnome-stock-pixmap-book-green"
#define GNOME_STOCK_PIXMAP_BOOK_BLUE "gnome-stock-pixmap-book-blue"
#define GNOME_STOCK_PIXMAP_BOOK_YELLOW "gnome-stock-pixmap-book-yellow"
#define GNOME_STOCK_PIXMAP_BOOK_OPEN "gnome-stock-pixmap-book-open"
#define GNOME_STOCK_PIXMAP_ABOUT "gnome-stock-pixmap-about"
#define GNOME_STOCK_PIXMAP_QUIT "gnome-stock-pixmap-quit"
#define GNOME_STOCK_PIXMAP_MULTIPLE "gnome-stock-pixmap-multiple"
#define GNOME_STOCK_PIXMAP_NOT "gnome-stock-pixmap-not"

#define GNOME_STOCK_PIXMAP_JUMP_TO "gnome-stock-pixmap-jump-to"
#define GNOME_STOCK_PIXMAP_UP "gnome-stock-pixmap-up"
#define GNOME_STOCK_PIXMAP_DOWN "gnome-stock-pixmap-down"

#define GNOME_STOCK_PIXMAP_ATTACH "gnome-stock-pixmap-attach"

#define GNOME_STOCK_PIXMAP_ALIGN_LEFT "gnome-stock-pixmap-align-left"
#define GNOME_STOCK_PIXMAP_ALIGN_RIGHT "gnome-stock-pixmap-align-right"
#define GNOME_STOCK_PIXMAP_ALIGN_CENTER "gnome-stock-pixmap-align-center"
#define GNOME_STOCK_PIXMAP_ALIGN_JUSTIFY "gnome-stock-pixmap-align-justify"

#define GNOME_STOCK_PIXMAP_TEXT_BOLD "gnome-stock-pixmap-text-bold"
#define GNOME_STOCK_PIXMAP_TEXT_ITALIC "gnome-stock-pixmap-text-italic"
#define GNOME_STOCK_PIXMAP_TEXT_UNDERLINE "gnome-stock-pixmap-text-underline"
#define GNOME_STOCK_PIXMAP_TEXT_STRIKEOUT "gnome-stock-pixmap-text-strikeout"
#define GNOME_STOCK_PIXMAP_TEXT_INDENT "gnome-stock-pixmap-text-indent"
#define GNOME_STOCK_PIXMAP_TEXT_UNINDENT "gnome-stock-pixmap-text-unindent"

#define GNOME_STOCK_PIXMAP_TABLE_BORDERS "gnome-stock-pixmap-table-borders"
#define GNOME_STOCK_PIXMAP_TABLE_FILL "gnome-stock-pixmap-table-fill"

#define GNOME_STOCK_PIXMAP_TEXT_BULLETED_LIST "gnome-stock-pixmap-text-bulleted-list"
#define GNOME_STOCK_PIXMAP_TEXT_NUMBERED_LIST "gnome-stock-pixmap-text-numbered-list"

#define GNOME_STOCK_PIXMAP_EXIT GNOME_STOCK_PIXMAP_QUIT


/*
 * Buttons
 */

#define GNOME_STOCK_BUTTON_OK "gnome-"
#define GNOME_STOCK_BUTTON_CANCEL "gnome-"
#define GNOME_STOCK_BUTTON_YES "gnome-"
#define GNOME_STOCK_BUTTON_NO "gnome-"
#define GNOME_STOCK_BUTTON_CLOSE "gnome-"
#define GNOME_STOCK_BUTTON_APPLY "gnome-"
#define GNOME_STOCK_BUTTON_HELP "gnome-stock-button-help"
#define GNOME_STOCK_BUTTON_NEXT "gnome-stock-button-next"
#define GNOME_STOCK_BUTTON_PREV "gnome-stock-button-prev"
#define GNOME_STOCK_BUTTON_UP "gnome-stock-button-up"
#define GNOME_STOCK_BUTTON_DOWN "gnome-stock-button-down"
#define GNOME_STOCK_BUTTON_FONT "gnome-stock-button-font"

/*
 * Menus
 */

#define GNOME_STOCK_MENU_BLANK "gnome-stock-menu-blank"

#define GNOME_STOCK_MENU_PROP "gnome-stock-menu-prop"
#define GNOME_STOCK_MENU_PREF "gnome-stock-menu-pref"
#define GNOME_STOCK_MENU_ABOUT "gnome-stock-menu-about"
#define GNOME_STOCK_MENU_SCORES "gnome-stock-menu-scores"

#define GNOME_STOCK_MENU_SRCHRPL "gnome-stock-menu-srchrpl"
#define GNOME_STOCK_MENU_BACK "gnome-stock-menu-back"
#define GNOME_STOCK_MENU_FORWARD "gnome-stock-menu-forward"

#define GNOME_STOCK_MENU_MAIL "gnome-stock-menu-mail"
#define GNOME_STOCK_MENU_MAIL_RCV "gnome-stock-menu-mail-rcv"
#define GNOME_STOCK_MENU_MAIL_SND "gnome-stock-menu-mail-snd"
#define GNOME_STOCK_MENU_MAIL_RPL "gnome-stock-menu-mail-rpl"
#define GNOME_STOCK_MENU_MAIL_FWD "gnome-stock-menu-mail-fwd"
#define GNOME_STOCK_MENU_MAIL_NEW "gnome-stock-menu-mail-new"

#define GNOME_STOCK_MENU_TRASH_FULL "gnome-stock-menu-trash-full"

#define GNOME_STOCK_MENU_TIMER "gnome-stock-menu-timer"
#define GNOME_STOCK_MENU_TIMER_STOP "gnome-stock-menu-timer-stop"

#define GNOME_STOCK_MENU_MIC "gnome-stock-menu-mic"
#define GNOME_STOCK_MENU_LINE_IN "gnome-stock-menu-line-in"

#define GNOME_STOCK_MENU_VOLUME "gnome-stock-menu-volume"
#define GNOME_STOCK_MENU_MIDI "gnome-stock-menu-midi"
#define GNOME_STOCK_MENU_BOOK_RED "gnome-stock-menu-book-red"
#define GNOME_STOCK_MENU_BOOK_GREEN "gnome-stock-menu-book-green"
#define GNOME_STOCK_MENU_BOOK_BLUE "gnome-stock-menu-book-blue"
#define GNOME_STOCK_MENU_BOOK_YELLOW "gnome-stock-menu-book-yellow"
#define GNOME_STOCK_MENU_BOOK_OPEN "gnome-stock-menu-book-open"

#define GNOME_STOCK_MENU_JUMP_TO "gnome-stock-menu-jump-to"
#define GNOME_STOCK_MENU_UP "gnome-stock-menu-up"
#define GNOME_STOCK_MENU_DOWN "gnome-stock-menu-down"

#define GNOME_STOCK_MENU_ATTACH "gnome-stock-menu-attach"

#define GNOME_STOCK_MENU_ALIGN_LEFT "gnome-stock-menu-align-left"
#define GNOME_STOCK_MENU_ALIGN_RIGHT "gnome-stock-menu-align-right"
#define GNOME_STOCK_MENU_ALIGN_CENTER "gnome-stock-menu-align-center"
#define GNOME_STOCK_MENU_ALIGN_JUSTIFY "gnome-stock-menu-align-justify"

#define GNOME_STOCK_MENU_TEXT_BOLD "gnome-stock-menu-text-bold"
#define GNOME_STOCK_MENU_TEXT_ITALIC "gnome-stock-menu-text-italic"
#define GNOME_STOCK_MENU_TEXT_UNDERLINE "gnome-stock-menu-text-underline"
#define GNOME_STOCK_MENU_TEXT_STRIKEOUT "gnome-stock-menu-text-strikeout"

#define GNOME_STOCK_MENU_EXIT GNOME_STOCK_MENU_QUIT

G_END_DECLS

#endif /* GNOME_STOCK_ICONS_H */
