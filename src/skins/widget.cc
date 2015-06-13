/*
 * widget.cc
 * Copyright 2015 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include "widget.h"

void Widget::set_gtk (GtkWidget * widget)
{
    g_signal_connect (widget, "destroy", (GCallback) Widget::destroy_cb, this);
    g_signal_connect (widget, "expose-event", (GCallback) Widget::draw_cb, this);

    m_widget = widget;
}

void Widget::destroy_cb (GtkWidget * widget, Widget * me)
{
    delete me;
}

gboolean Widget::draw_cb (GtkWidget * widget, GdkEventExpose * event, Widget * me)
{
    cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (widget));

    if (! gtk_widget_get_has_window (widget))
    {
        GtkAllocation alloc;
        gtk_widget_get_allocation (widget, & alloc);
        cairo_translate (cr, alloc.x, alloc.y);
        cairo_rectangle (cr, 0, 0, alloc.width, alloc.height);
        cairo_clip (cr);
    }

    me->draw (cr);

    cairo_destroy (cr);
    return false;
}
