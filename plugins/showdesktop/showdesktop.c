/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (C) 2007-2010 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <common/panel-private.h>
#include <common/panel-utils.h>

#include "showdesktop.h"



static void     show_desktop_plugin_screen_changed          (GtkWidget              *widget,
                                                             GdkScreen              *previous_screen);
static void     show_desktop_plugin_free_data               (XfcePanelPlugin        *panel_plugin);
static gboolean show_desktop_plugin_size_changed            (XfcePanelPlugin        *panel_plugin,
                                                             gint                    size);
static void     show_desktop_plugin_toggled                 (GtkToggleButton        *button,
                                                             ShowDesktopPlugin      *plugin);
static void     show_desktop_plugin_showing_desktop_changed (WnckScreen             *wnck_screen,
                                                             ShowDesktopPlugin      *plugin);



struct _ShowDesktopPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _ShowDesktopPlugin
{
  XfcePanelPlugin __parent__;

  /* the toggle button */
  GtkWidget  *button;

  /* the wnck screen */
  WnckScreen *wnck_screen;
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (ShowDesktopPlugin, show_desktop_plugin)



static void
show_desktop_plugin_class_init (ShowDesktopPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->free_data = show_desktop_plugin_free_data;
  plugin_class->size_changed = show_desktop_plugin_size_changed;
}



static void
show_desktop_plugin_init (ShowDesktopPlugin *plugin)
{
  GtkWidget *button, *image;

  plugin->wnck_screen = NULL;

  /* monitor screen changes */
  g_signal_connect (G_OBJECT (plugin), "screen-changed",
      G_CALLBACK (show_desktop_plugin_screen_changed), NULL);

  /* create the toggle button */
  button = plugin->button = xfce_panel_create_toggle_button ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (plugin), button);
  gtk_widget_set_name (button, "showdesktop-button");
  g_signal_connect (G_OBJECT (button), "toggled",
      G_CALLBACK (show_desktop_plugin_toggled), plugin);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), button);
  gtk_widget_show (button);

  image = xfce_panel_image_new_from_source ("user-desktop");
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);
}



static void
show_desktop_plugin_screen_changed (GtkWidget *widget,
                                    GdkScreen *previous_screen)
{
  ShowDesktopPlugin *plugin = XFCE_SHOW_DESKTOP_PLUGIN (widget);
  WnckScreen        *wnck_screen;
  GdkScreen         *screen;

  panel_return_if_fail (XFCE_IS_SHOW_DESKTOP_PLUGIN (widget));

  /* get the new wnck screen */
  screen = gtk_widget_get_screen (widget);
  wnck_screen = wnck_screen_get (gdk_screen_get_number (screen));
  panel_return_if_fail (WNCK_IS_SCREEN (wnck_screen));

  /* leave when the wnck screen did not change */
  if (plugin->wnck_screen == wnck_screen)
    return;

  /* disconnect signals from an existing wnck screen */
  if (plugin->wnck_screen != NULL)
    g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->wnck_screen),
        show_desktop_plugin_showing_desktop_changed, plugin);

  /* set the new wnck screen */
  plugin->wnck_screen = wnck_screen;
  g_signal_connect (G_OBJECT (wnck_screen), "showing-desktop-changed",
      G_CALLBACK (show_desktop_plugin_showing_desktop_changed), plugin);

  /* toggle the button to the current state or update the tooltip */
  if (G_UNLIKELY (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (plugin->button)) !=
        wnck_screen_get_showing_desktop (wnck_screen)))
    show_desktop_plugin_showing_desktop_changed (wnck_screen, plugin);
  else
    show_desktop_plugin_toggled (GTK_TOGGLE_BUTTON (plugin->button), plugin);
}



static void
show_desktop_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  ShowDesktopPlugin *plugin = XFCE_SHOW_DESKTOP_PLUGIN (panel_plugin);

  /* disconnect screen changed signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (plugin),
     show_desktop_plugin_screen_changed, NULL);

  /* disconnect handle */
  if (plugin->wnck_screen != NULL)
    g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->wnck_screen),
        show_desktop_plugin_showing_desktop_changed, plugin);
}



static gboolean
show_desktop_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                                  gint             size)
{
  panel_return_val_if_fail (XFCE_IS_SHOW_DESKTOP_PLUGIN (panel_plugin), FALSE);

  /* keep the button squared */
  gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), size, size);

  return TRUE;
}



static void
show_desktop_plugin_toggled (GtkToggleButton   *button,
                             ShowDesktopPlugin *plugin)
{
  gboolean     active;
  const gchar *text;

  panel_return_if_fail (XFCE_IS_SHOW_DESKTOP_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  panel_return_if_fail (WNCK_IS_SCREEN (plugin->wnck_screen));

  /* toggle the desktop */
  active = gtk_toggle_button_get_active (button);
  if (active != wnck_screen_get_showing_desktop (plugin->wnck_screen))
    wnck_screen_toggle_showing_desktop (plugin->wnck_screen, active);

  if (active)
    text = _("Restore the minimized windows");
  else
    text = _("Minimize all open windows and show the desktop");

  gtk_widget_set_tooltip_text (GTK_WIDGET (button), text);
  panel_utils_set_atk_info (GTK_WIDGET (button), _("Show Desktop"), text);
}



static void
show_desktop_plugin_showing_desktop_changed (WnckScreen        *wnck_screen,
                                             ShowDesktopPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_SHOW_DESKTOP_PLUGIN (plugin));
  panel_return_if_fail (WNCK_IS_SCREEN (wnck_screen));
  panel_return_if_fail (plugin->wnck_screen == wnck_screen);

  /* update button to user action */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button),
      wnck_screen_get_showing_desktop (wnck_screen));
}
