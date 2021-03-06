/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rom-properties-plugin.c: ThunarX Plugin Definition.                     *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "rom-properties-provider.hpp"
#include "rom-properties-page.hpp"

// GLib on non-Windows platforms defines G_MODULE_EXPORT to a no-op.
// This doesn't work when we use symbol visibility settings.
#if !defined(_WIN32) && (defined(__GNUC__) && __GNUC__ >= 4)
#ifdef G_MODULE_EXPORT
#undef G_MODULE_EXPORT
#endif
#define G_MODULE_EXPORT __attribute__ ((visibility ("default")))
#endif /* !_WIN32 && __GNUC__ >= 4 */

G_MODULE_EXPORT void thunar_extension_initialize	(ThunarxProviderPlugin *plugin);
G_MODULE_EXPORT void thunar_extension_shutdown		(void);
G_MODULE_EXPORT void thunar_extension_list_types	(const GType	**types,
							 gint		 *n_types);

static GType type_list[1];

G_MODULE_EXPORT void
thunar_extension_initialize(ThunarxProviderPlugin *plugin)
{
	const gchar *mismatch;

	/* verify that the thunarx versions are compatible */
	mismatch = thunarx_check_version(THUNARX_MAJOR_VERSION, THUNARX_MINOR_VERSION, THUNARX_MICRO_VERSION);
	if (G_UNLIKELY(mismatch != NULL)) {
		g_warning ("Version mismatch: %s", mismatch);
		return;
	}

#ifdef G_ENABLE_DEBUG
	g_message("Initializing rom-properties-xfce extension");
#endif

	/* Register the types provided by this plugin */
	rom_properties_provider_register_type(plugin);
	rom_properties_page_register_type(plugin);

	/* Setup the plugin provider type list */
	type_list[0] = TYPE_ROM_PROPERTIES_PROVIDER;
}

G_MODULE_EXPORT void
thunar_extension_shutdown(void)
{
#ifdef G_ENABLE_DEBUG
	g_message("Shutting down rom-properties-xfce extension");
#endif
}

G_MODULE_EXPORT void
thunar_extension_list_types(const GType	**types,
			    gint	 *n_types)
{
	*types = type_list;
	*n_types = G_N_ELEMENTS(type_list);
}
