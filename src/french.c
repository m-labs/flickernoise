/*
 * Flickernoise
 * Copyright (C) 2011 Sebastien Bourdeauducq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mtki18n.h>
#include <stdlib.h>

struct mtk_i18n_entry translation_french[] = {
	// General
	{ "OK",							"OK" },
	{ "Cancel",						"Annuler" },
	{ "Close",						"Fermer" },
	{ "Filename:",						"Nom de fichier :" },
	{ "Browse",						"Parcourir" },
	{ "Clear",						"Effacer" },
	{ "Ready.",						"Pr\xea""t" },

	// File dialog box
	{ "Quick find: ",					"Recherche rapide : " },
	{ "Selection:",						"S\xe9""lection :" },

	// Control panel
	{ "Control panel",					"Panneau de contr\xf4""le" },
	{ "Control panel [modified]",				"Panneau de contr\xf4""le [modifi\xe9""]" },
	{ "Interfaces",						"Interfaces" },
	{ "Keyboard",						"Clavier" },
	{ "IR remote",						"T\xe9""l\xe9""commande IR" },
	{ "Audio",						"Audio" },
	{ "MIDI",						"MIDI" },
	{ "OSC",						"OSC" },
	{ "DMX512",						"DMX512" },
	{ "Video in",						"Entr\xe9""e vid\xe9""o" },
	{ "Online",						"En ligne" },
	{ "RSS wall",						"Mur RSS" },
	{ "Web update",						"Mise \xe0"" jour web" },
	{ "Patches",						"Patchs" },
	{ "Patch editor",					"Editeur de patchs" },
	{ "Variable monitor",					"Moniteur de variable" },
	{ "Performance",					"Performance" },
	{ "New",						"Nouveau" },
	{ "Load",						"Charger" },
	{ "Save",						"Enregistrer" },
	{ "First patch",					"Premier patch" },
	{ "Start",						"D\xe9""marrer" },
	{ "Tools",						"Outils" },
	{ "File manager",					"Gestionnaire de fichiers" },
	{ "System",						"Syst\xe8""me" },
	{ "Settings",						"Param\xe8""tres" },
	{ "About",						"A propos" },
	{ "Shutdown",						"Arr\xea""ter" },

	// Keyboard settings
	{ "Keyboard settings",					"Param\xe8""tres du clavier" },
	{ "Existing bindings",					"Associations existantes" },
	{ "Add/edit",						"Ajouter/\xe9""diter" },
	{ "Key (a-z):",						"Touche (a-z) :" },
	{ "Add/update",						"Ajouter/mettre \xe0"" jour" },
	{ "Auto build",						"Liste automatique" },
	
	// IR settings
	{ "IR remote control settings",				"Param\xe8""tres de la t\xe9""l\xe9""commande IR" },
	{ "Key code:",						"Code touche :" },
	
	// Audio settings
	{ "Audio settings",					"Param\xe8""tres audio" },
	{ "Line volume",					"Volume ligne" },
	{ "Mic volume",						"Volume micro" },
	{ "Mute",						"Muet" },
	
	// MIDI settings
	{ "MIDI settings",					"Param\xe8""tres MIDI" },
	{ "Channel (0-15):",					"Canal (0-15) :" },
	{ "Note:",						"Note :" },
	{ "Controller mapping",					"Connexion des contr\xf4""leurs" },
	{ "Latest active controller:",				"Dernier contr\xf4""leur actif :" },
	
	// OSC settings
	{ "OSC settings",					"Param\xe8""tres OSC" },
	{ "Number:",						"Num\xe9""ro :" },
	
	// DMX settings
	{ "DMX settings",					"Param\xe8""tres DMX" },
	{ "Chain",						"Cha\xee""ner" },
	{ "DMX spy",						"Espion DMX" },
	{ "DMX desk",						"Table DMX" },
	
	// Video input settings
	{ "Video input settings",				"Param\xe8""tres" },
	{ "Brightness:",					"Luminosit\xe9"" :" },
	{ "Contrast:",						"Contraste :" },
	{ "Hue:",						"Teinte :" },
	{ "Detected signal:",					"Signal d\xe9tect\xe9 :" },
	{ "None",						"Aucun" },
	
	// RSS wall
	{ "RSS/ATOM wall",					"Mur RSS/ATOM" },
	{ "RSS/ATOM URL:",					"URL RSS/ATOM" },
	{ "Idle message:",					"Message d'inactivit\xe9"" :" },
	{ "Refresh period:",					"P\xe9""riode de rafra\xee""chissement :" },
	{ "Idle period:",					"P\xe9""riode d'inactivit\xe9"" :" },
	{ "seconds",						"secondes" },
	{ "refreshes",						"rafra\xee""chissements" },
	
	// Update
	{ "Update",						"Mise \xe0"" jour" },
	// TODO: explanation text
	{ "Installed",						"Install\xe9" },
	{ "Available",						"Disponible" },
	{ "Update from web",					"Mise \xe0"" jour depuis le web" },
	{ "Check versions",					"V\xe9rifier les versions" },
	{ "Update from files",					"Mise \xe0"" jour depuis des fichiers" },
	
	// Patch editor
	{ "Save As",						"Enregistrer Sous" },
	{ "Run (F2)",						"Lancer (F2)" },
	
	// Performance
	{ "Load performance",					"Charger une performance" },
	
	{ NULL, NULL }
};
