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
	{ "Clear",						"Eff. ch." },
	{ "Enable",						"Activer" },
	{ "Ready.",						"Pr\xea""t." },

	// Main
	{ "Rescue mode",					"Mode de secours" },
	{ "You have booted in rescue mode.\n"
	  "Your system will function as usual, using back-up software.\n"
	  "From there, you can update the main software or perform\nother actions to fix the problem.\n",
	  "Vous avez activ\xe9"" le mode de secours.\n"
	  "Votre syst\xe8""me fonctionne normalement, en utilisant une copie\nde sauvegarde du logiciel.\n"
	  "Vous pouvez effectuer une mise \xe0"" jour pour r\xe9""installer le logiciel principal\net tenter de corriger le probl\xe8""me.\n" },

	// File dialog box
	{ "Quick find: ",					"Recherche rapide : " },
	{ "Selection:",						"S\xe9""lection :" },

	// Control panel
	{ "Control panel",					"Panneau de contr\xf4""le" },
	{ "Control panel *",					"Panneau de contr\xf4""le *" },
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
	{ "Variable monitor",					"Moniteur de variables" },
	{ "Performance",					"Performance" },
	{ "New",						"Nouveau" },
	{ "Load",						"Charger" },
	{ "Save",						"Enregistrer" },
	{ "Save performance",					"Enregistrer performance" },
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
	{ "Select keyboard patch",				"S\xe9""lection du patch clavier" },
	
	// IR settings
	{ "IR remote control settings",				"Param\xe8""tres de la t\xe9""l\xe9""commande IR" },
	{ "Key code:",						"Code touche :" },
	{ "Select IR patch",					"S\xe9""lection du patch IR" },
	
	// Audio settings
	{ "Audio settings",					"Param\xe8""tres audio" },
	{ "Line volume",					"Volume ligne" },
	{ "Mic volume",						"Volume micro" },
	{ "Mute",						"Muet" },
	
	// MIDI settings
	{ "MIDI settings",					"Param\xe8""tres MIDI" },
	{ "Global parameters",					"Param\xe8""tres globaux" },
	{ "Channel (0-15):",					"Canal (0-15) :" },
	{ "Note:",						"Note :" },
	{ "Controller mapping",					"Connexion des contr\xf4""leurs" },
	{ "Latest active controller:",				"Dernier contr\xf4""leur actif :" },
	{ "Select MIDI patch",					"S\xe9""lection du patch MIDI" },
	
	// OSC settings
	{ "OSC settings",					"Param\xe8""tres OSC" },
	{ "Number:",						"Num\xe9""ro :" },
	{ "OSC patch select",					"S\xe9""lection du patch OSC" },
	
	// DMX settings
	{ "DMX settings",					"Param\xe8""tres DMX" },
	{ "Chain",						"Cha\xee""ner" },
	{ "DMX spy",						"Espion DMX" },
	{ "DMX desk",						"Table DMX" },
	{ "Latest active channel:",				"Dernier canal actif :" },
	
	// Video input settings
	{ "Video input settings",				"Param\xe8""tres d'entr\xe9""e vid\xe9""o" },
	{ "Format",						"Format" },
	{ "CVBS: Green",					"CVBS: Vert" },
	{ "CVBS: Blue",						"CVBS: Bleu" },
	{ "CVBS: Red",						"CVBS: Rouge" },
	{ "S-Video (Y: Green, C: Blue)",			"S-Video (Y: Vert, C: Bleu)" },
	{ "Component (YPbPr)",					"Composantes (YPbPr)" },
	{ "Parameters",						"Param\xe8""tres" },
	{ "Brightness:",					"Luminosit\xe9"" :" },
	{ "Contrast:",						"Contraste :" },
	{ "Hue:",						"Teinte :" },
	{ "Detected signal:",					"Signal d\xe9tect\xe9 :" },
	{ "Preview",						"Pr\xe9""visualisation" },
	{ "None",						"Aucun" },
	{ "Unknown",						"Inconnu" },
	
	// RSS wall
	{ "Wall:",						"Mur :" },
	{ "RSS/ATOM wall",					"Mur RSS/ATOM" },
	{ "RSS/ATOM URL:",					"URL RSS/ATOM :" },
	{ "Idle message:",					"Message d'inactivit\xe9"" :" },
	{ "Refresh period:",					"P\xe9""riode de\nrafra\xee""chissement :" },
	{ "Idle period:",					"P\xe9""riode d'inactivit\xe9"" :" },
	{ "seconds",						"secondes" },
	{ "refreshes",						"rafra\xee""ch." },
	
	// Update
	{ "Update",						"Mise \xe0"" jour" },
	{ "Click the 'Update from web' button to begin.\nSince you are in rescue mode, the new software will always be reinstalled,\neven if you already have the latest version.",
	  "Cliquez sur le bouton 'Mise \xe0"" jour depuis le web' pour commencer.\nComme vous \xea""tes en mode de secours, le logiciel\nsera syst\xe9""matiquement r\xe9""install\xe9""." },
	{ "Click the 'Update from web' button to begin.\nIf your synthesizer does not restart after the update, don't panic!\nHold right (R) pushbutton during power-up to enable rescue mode.",
	  "Cliquez sur le bouton 'Mise \xe0"" jour depuis le web' pour commencer.\nEn cas de probl\xe8""me, appuyez sur le bouton de droite (R) pendant\nla mise en marche afin de passer en mode de secours." },
	{ "Installed",						"Install\xe9" },
	{ "Available",						"Disponible" },
	{ "Update from web",					"Mise \xe0"" jour depuis le web" },
	{ "Check versions",					"V\xe9""rifier les versions" },
	{ "Update from files",					"Mise \xe0"" jour depuis des fichiers" },
	{ "Bitstream image (.FPG):",				"Image bitstream (.FPG) :" },
	{ "BIOS image (.BIN):",					"Image BIOS (.BIN) :" },
	{ "Application image (.FBI):",				"Image application (.FBI) :" },
	{ "Flash",						"Flasher" },
	{ "Patch pool:",					"Patchs partag\xe9""s :" },
	{ "Open flash image",					"Choisir image \xe0"" flasher" },
	{ "Working...",						"Traitement en cours..." },
	{ "Erasing bitstream...",				"Effacement du bitstream..." },
	{ "Programming bitstream...",				"Programmation du bitstream..." },
	{ "Failed to program bitstream.",			"Erreur de programmation du bitstream." },
	{ "Erasing BIOS...",					"Effacement du BIOS..." },
	{ "Programming BIOS...",				"Programmation du BIOS..." },
	{ "Failed to program BIOS.",				"Erreur de programmation du BIOS." },
	{ "Erasing application...",				"Effacement de l'application..." },
	{ "Programming application...",				"Programmation de l'application..." },
	{ "Failed to program application.",			"Erreur de programmation de l'application" },
	{ "Failed to download version information.",		"Erreur de t\xe9""l\xe9""chargement des informations de version." },
	{ "Downloading bitstream...",				"T\xe9""l\xe9""chargement du bitstream..." },
	{ "Failed to download bitstream.",			"Erreur de t\xe9""l\xe9""chargement du bitstream." },
	{ "Downloading BIOS...",				"T\xe9""l\xe9""chargement du BIOS..." },
	{ "Failed to download BIOS.",				"Erreur de t\xe9""l\xe9""chargement du BIOS." },
	{ "Downloading application...",				"T\xe9""l\xe9""chargement de l'application..." },
	{ "Failed to download application.",			"Erreur de t\xe9""l\xe9""chargement de l'application." },
	{ "Downloading patches...",				"T\xe9""l\xe9""chargement des patches..." },
	{ "Completed successfully.",				"Termin\xe9"" avec succ\xe8""s." },
	
	// Patch editor
	{ "Open",						"Ouvrir" },
	{ "Save As",						"Enregistrer sous" },
	{ "Run (F2)",						"Lancer (F2)" },
	{ "untitled",						"sans titre" },
	{ "Open patch",						"Ouvrir un patch" },
	{ "Save patch",						"Enregistrer le patch" },
	
	// Performance
	{ "Load performance",					"Charger une performance" },
	{ "Mode:",						"Mode :" },
	{ "Simple:",						"Simple :" },
	{ "Display titles",					"Afficher les titres" },
	{ "Auto switch",					"Changement automatique" },
	{ "Configured",						"Configur\xe9" },
	{ "Go!",						"D\xe9""marrer !" },
	{ "Compiling patches...",				"Compilation des patchs..." },
	{ "Select first patch",					"S\xe9""lection du premier patch" },
	{ "Error",						"Erreur" },
	{ "No first patch defined!",				"Pas de premier patch d\xe9""fini !" },
	{ "No patches found!",					"Pas de patch trouv\xe9"" !" },
	
	// File manager
	{ "Copy",						"Copier" },
	{ "Move",						"D\xe9""p." },
	{ "Name:",						"Nom :" },
	{ "Rename",						"Renom." },
	{ "Delete",						"Suppr." },
	{ "Mkdir",						"Cr. rep." },
	{ "Sure?",						"S\xfb""r?" },
	
	// System settings
	{ "System settings",					"Param\xe8""tres du syst\xe8""me" },
	{ "Desktop",						"Bureau" },
	{ "Resolution:",					"R\xe9""solution :" },
	{ "Wallpaper:",						"Papier peint :" },
	{ "Select wallpaper",					"S\xe9""lection du papier peint" },
	{ "Localization",					"Localisation" },
	{ "Language:",						"Langue :" },
	{ "Keyboard layout:",					"Clavier :" },
	{ "English",						"Anglais" },
	{ "French",						"Fran\xe7""ais" },
	{ "German",						"Allemand" },
	{ "US",							"US" },
	{ "Network",						"R\xe9""seau" },
	{ "DHCP client:",					"Client DHCP :" },
	{ "IP address:",					"Adresse IP :" },
	{ "Netmask:",						"Masque :" },
	{ "Gateway:",						"Passerelle :" },
	{ "DNS:",						"DNS :" },
	{ "Remote access",					"Acc\xe8""s distant" },
	{ "Login:",						"Nom :" },
	{ "Pass:",						"MdP :" },
	{ "Autostart",						"D\xe9""marrage automatique" },
	{ "Configured:",					"Configur\xe9 :" },
	{ "Select autostart performance",			"S\xe9""lection du d\xe9""marrage automatique" },
	
	// About
	{ "Flickernoise is free software, released under GNU GPL version 3.\n"
	"Copyright (C) 2010, 2011 Flickernoise developers.\n"
	"Milkymist is a trademark of S\xe9""bastien Bourdeauducq.\n"
	"Web: www.milkymist.org",
	 "Flickernoise est un logiciel libre, sous licence GNU GPL version 3.\n"
	"Copyright (C) 2010, 2011 d\xe9""veloppeurs Flickernoise.\n"
	"Milkymist est une marque de S\xe9""bastien Bourdeauducq.\n"
	"Web: www.milkymist.org" },
	
	// Shutdown
	{ "Do you want to power off or reboot the system?",	"Souhaitez-vous arr\xea""ter ou red\xe9""marrer le syst\xe8""me ?" },
	{ "Power off",						"Arr\xea""ter" },
	{ "Reboot",						"Red\xe9""marrer" },
	
	{ "Messagebox",						"Bo\xee""te de message" },
	
	{ NULL, NULL }
};
