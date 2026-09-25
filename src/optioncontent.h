#ifndef ATANKS_OPTIONCONTENT_H_INCLUDED
#define ATANKS_OPTIONCONTENT_H_INCLUDED 1

/*
 * atanks - obliterate each other with oversize weapons
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "globaltypes.h"
#include "optiontypes.h"

/** @file optioncontent.h
 *
 * This file defines static string arrays with the text content of the
 * player, play and option menu and all sub menus.
 *
 * It is not necessary to include this file anywhere else but in menu.cpp.
 *
 **/


// Maximum number of entries including Title and 0x0 termination per menu
/// Entries per menu including title and terminator.
uint32_t const MAX_ENTRIES_PER_MENU = 18;


// Maximum text entries per text class including 0x0 termination
/// Text entries per class including terminator.
uint32_t const MAX_ENTRIES_PER_CLASS = 11;


/** @brief string array for the menu content
 *
 * The ordering, although it looks a bit overwhelming here, is quite simple.
 * The first index is the menu class, the second is the language.
 *
 * With this both translation and adding new content is fairly easy. Just
 * copy a block (after adding new enum entries at the proper places in
 * optiontypes.h) and edit to the new content.
 *
 * All text arrays end with a zero 0x0 entry. It is therefore not needed to
 * hard code any menu list sizes.
 *
 * As a rule of thumb, the title is the first line, every other texts are
 * listed with two entries per line. Unless a possible third entry is the
 * finalizing 0x0 entry, it does not need its own line.
 **/
char const* const MENU_TITLE_TEXT[ MC_MENUCLASS_COUNT ][ EL_LANGUAGE_COUNT ][ MAX_ENTRIES_PER_MENU ] = {
	{/* -------------------- *
         * --- AREYOUSURE   --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Are you sure?",
	    "Yes",
	    "No",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    /* ===== Needs to be translated ===== */
	    "Are you sure?",
	    "Yes",
	    "No",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    /* ===== Needs to be translated ===== */
	    "Are you sure?",
	    "Yes",
	    "No",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Sind Sie sicher?",
	    "Ja",
	    "Nein",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    /* ===== Needs to be translated ===== */
	    "Are you sure?",
	    "Yes",
	    "No",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    /* ===== Needs to be translated ===== */
	    "Are you sure?",
	    "Yes",
	    "No",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Are you sure?",
	    "Yes",
	    "No",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Are you sure?",
	    "Yes",
	    "No",
	    nullptr } },
	{           /* -------------------- *
         * --- FINANCE      --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Money",
	    "Starting Money",
	    "Interest Rate",
	    "Round Win Bonus",
	    "Damage Bounty",
	    "Self-Damage Penalty",
	    "Team-Damage Penalty",
	    "Tank Destruction Bonus",
	    "Tank Self-Destruction Penalty",
	    "Item Sell Multiplier",
	    "Teams Share",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    "Dinheiro",
	    "Dinheiro inicial",
	    "Taxa de Juros",
	    "Bônus por Vitória",
	    "Bônus por Estrago",
	    "Penalidade por Auto-Estrago",
	    "Team-Damage Penalty",
	    "Bônus por Tanque Destruído",
	    "Penalidade por Auto-Destruição",
	    "Multiplicador de Item Vendido",
	    "Parte das equipes",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    "Finances",
	    "Somme de départ",
	    "Taux d'intérêt",
	    "Gains par victoire",
	    "Bonus dommages",
	    "Pénalité auto-dommages",
	    "Team-Damage Penalty",
	    "Bonus destruction tank",
	    "Pénalité autodestruction tank",
	    "Coeff. vente item",
	    "Part d'equipes",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Geld",
	    "Startgeld",
	    "Zinssatz",
	    "Rundenbonus",
	    "Schadensbonus",
	    "Strafe für Selbstschaden",
	    "Strafe für Teamschaden",
	    "Zerstörungsbonus",
	    "Selbstzerstörungsstrafe",
	    "Verkaufsmultiplikator",
	    "Mannschaftanteil",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    "Peniaze",
	    "Peniaze na začiatku",
	    "Úroková miera",
	    "Bonus pri skončení kola",
	    "Odmena za poškodenie",
	    "Pokuta za vlastné poškodenie",
	    "Team-Damage Penalty",
	    "Bonus za zničenie tanku",
	    "Pokuta za vlastné zničenie tanku",
	    "Násobiteľ pri predaji položiek",
	    "Teamy zdieľajú peniaze",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    "Экономика",
	    "Начальные деньги",
	    "Банковский процент",
	    "Бонус за победу",
	    "Бонус за попадание",
	    "Штраф за попадание в себя",
	    "Team-Damage Penalty",
	    "Бонус за уничтожение",
	    "Штраф за самоуничтожение",
	    "Коэфф. продажи снаряжения",
	    "Командные боеприпасы",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Money",
	    "Starting Money",
	    "Interest Rate",
	    "Round Win Bonus",
	    "Damage Bounty",
	    "Self-Damage Penalty",
	    "Team-Damage Penalty",
	    "Tank Destruction Bonus",
	    "Tank Self-Destruction Penalty",
	    "Item Sell Multiplier",
	    "Teams Share",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Money",
	    "Starting Money",
	    "Interest Rate",
	    "Round Win Bonus",
	    "Damage Bounty",
	    "Self-Damage Penalty",
	    "Team-Damage Penalty",
	    "Tank Destruction Bonus",
	    "Tank Self-Destruction Penalty",
	    "Item Sell Multiplier",
	    "Teams Share",
	    "Back",
	    nullptr } },
	{ /* -------------------- *
         * --- GRAPHICS     --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Graphics",
	    "Full Screen",
	    "Dithering",
	    "Detailed Land",
	    "Detailed Sky",
	    "Fading Text",
	    "Shadowed Text",
	    "Swaying Text",
	    "Colour Theme",
	    "Screen Width",
	    "Screen Height",
	    "Mouse Pointer",
	    "Game Speed",
	    "Custom Background",
	    "Show AI Feedback",
	    "Dynamic CMenu Background",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    "Gráficos",
	    "Tela Cheia",
	    "Pontilhamento",
	    "Detalhes do Terreno",
	    "Detalhes do Céu",
	    "texto sombreado",
	    "texto de desvanecimento",
	    "Swaying Text",
	    "tema da cor",
	    "Largura da Tela",
	    "Altura da Tela",
	    "Ponteiro do Rato",
	    "Velocidade do jogo",
	    "Fundo personalizado",
	    "Show AI Feedback",
	    "Dynamic CMenu Background",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    "Graphismes",
	    "Full Screen",
	    "Tramage",
	    "Détails du terrain",
	    "Ciel détaillé",
	    "texte ombragé",
	    "texte de effacement",
	    "Swaying Text",
	    "Thème de couleurs",
	    "Largeur d'écran",
	    "Hauteur d'écran",
	    "Curseur de souris",
	    "Vitesse du jeu",
	    "Fond fait sur commande",
	    "Show AI Feedback",
	    "Dynamic CMenu Background",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Grafik",
	    "Vollbild",
	    "Dithering",
	    "Landdetails",
	    "Himmeldetails",
	    "Ausblendender Text",
	    "Schattierter Text",
	    "Schwingender Text",
	    "Farbschema",
	    "Bildschirmbreite",
	    "Bildschirmhöhe",
	    "Mauszeiger",
	    "Spielgeschwindigket",
	    "Eigener Hintergrund",
	    "Zeige AI Feedback",
	    "Dynamischer Menühintergrund",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    "Grafika",
	    "Na celú obrazovku",
	    "Rozptyl",
	    "Detaily krajiny",
	    "Detaily oblohy",
	    "Slabnúci text",
	    "Text s tieňom",
	    "Swaying Text",
	    "Farebná téma",
	    "Šírka obrazovky",
	    "Výška obrazovky",
	    "Ukazovateľ myši",
	    "Rýchlosť hry",
	    "Vlastné pozadie",
	    "Show AI Feedback",
	    "Dynamic CMenu Background",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    "Графика",
	    "Full Screen",
	    "Сглаживание",
	    "Детализированный ландшафт",
	    "Детализированное небо",
	    "Исчезающий текст",
	    "Оттененный текст",
	    "Swaying Text",
	    "Цветовая тема",
	    "Ширина окна игры",
	    "Высота окна игры",
	    "Курсор в игре",
	    "Скорость игры",
	    "Собственный фон",
	    "Show AI Feedback",
	    "Dynamic CMenu Background",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Graphics",
	    "Full Screen",
	    "Dithering",
	    "Detailed Land",
	    "Detailed Sky",
	    "Fading Text",
	    "Shadowed Text",
	    "Swaying Text",
	    "Colour Theme",
	    "Screen Width",
	    "Screen Height",
	    "Mouse Pointer",
	    "Game Speed",
	    "Custom Background",
	    "Show AI Feedback",
	    "Dynamic CMenu Background",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Graphics",
	    "Full Screen",
	    "Dithering",
	    "Detailed Land",
	    "Detailed Sky",
	    "Fading Text",
	    "Shadowed Text",
	    "Swaying Text",
	    "Colour Theme",
	    "Screen Width",
	    "Screen Height",
	    "Mouse Pointer",
	    "Game Speed",
	    "Custom Background",
	    "Show AI Feedback",
	    "Dynamic CMenu Background",
	    "Back",
	    nullptr } },
	{ /* -------------------- *
         * --- MAIN         --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Main CMenu",
	    "Reset All",
	    "Physics",
	    "Weather",
	    "Graphics",
	    "Money",
	    "Network",
	    "Sound",
	    "Weapon Tech Level",
	    "Item Tech Level",
	    "Landscape",
	    "Turn Order",
	    "Skip AI-only play",
	    "Show FPS",
	    "Language",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    "CMenu Principal",
	    "Reset All",
	    "Física",
	    "Condições Meteorológicas",
	    "Gráficos",
	    "Finanças",
	    "Rede",
	    "Som",
	    "Arma Nível Tecnológico",
	    "Artigo Nível Tecnológico",
	    "Cenário",
	    "Ordem de Jogadas",
	    "Continuar o Jogo Só com Robôs",
	    "Show FPS",
	    "Língua",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    "CMenu principal",
	    "Reset All",
	    "Physique",
	    "Météo",
	    "Graphismes",
	    "Finances",
	    "Réseau",
	    "Sound",
	    "Niveau technique armes",
	    "Niveau technique équipement",
	    "Paysage",
	    "Ordre de passage",
	    "Continuer le Jeu Robots seuls",
	    "Show FPS",
	    "Langue",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Hauptmenü",
	    "Alles zurücksetzen",
	    "Physik",
	    "Wetter",
	    "Grafik",
	    "Geld",
	    "Netzwerk",
	    "Sounds",
	    "Technologiestufe Waffen",
	    "Technologiestufe Gegenstände",
	    "Landschaft",
	    "Reihenfolge",
	    "Überspringe Nur-KI",
	    "FPS anzeigen",
	    "Sprache",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    "Hlavné menu",
	    "Reset All",
	    "Fyzika",
	    "Počasie",
	    "Grafika",
	    "Peniaze",
	    "Sieť",
	    "Zvuk",
	    "Tech úroveň zbraní",
	    "Tech úroveň vecí",
	    "Krajina",
	    "Poradie",
	    "Preskočiť hru samotného PC",
	    "Show FPS",
	    "Jazyk",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    "Главное меню",
	    "Reset All",
	    "Физика",
	    "Погода",
	    "Графика",
	    "Экономика",
	    "Настройки сети",
	    "Звук",
	    "Уровень оружия",
	    "Уровень снаряжения",
	    "Тип ландшафта",
	    "Порядок хода",
	    "Пропускать игру компьютеров",
	    "Show FPS",
	    "Язык (Language)",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Main CMenu",
	    "Reset All",
	    "Physics",
	    "Weather",
	    "Graphics",
	    "Money",
	    "Network",
	    "Sound",
	    "Weapon Tech Level",
	    "Item Tech Level",
	    "Landscape",
	    "Turn Order",
	    "Skip AI-only play",
	    "Show FPS",
	    "Language",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Main CMenu",
	    "Reset All",
	    "Physics",
	    "Weather",
	    "Graphics",
	    "Money",
	    "Network",
	    "Sound",
	    "Weapon Tech Level",
	    "Item Tech Level",
	    "Landscape",
	    "Turn Order",
	    "Skip AI-only play",
	    "Show FPS",
	    "Language",
	    "Back",
	    nullptr } },
	{ /* -------------------- *
         * --- NETWORK      --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Network",
	    "Check Updates",
	    "Networking",
	    "Listen Port",
	    "Server Address",
	    "Server Port",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    "Network",
	    "Procurar actualizações",
	    "Activar Rede",
	    "Número de Porta",
	    "Server Address",
	    "Server Port",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    /* ===== Needs to be translated ===== */
	    "Network",
	    "Check Updates",
	    "Networking",
	    "Listen Port",
	    "Server Address",
	    "Server Port",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Netzwerk",
	    "Auf Aktualisierungen prüfen",
	    "Netzwerk",
	    "offener Port",
	    "Serveraddresse",
	    "Server Port",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    "Sieť",
	    "Kontrola aktualizácii",
	    "Sieťová hra",
	    "Port pre načúvanie",
	    "Server Address",
	    "Server Port",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    "Настройки сети",
	    "Проверять обновления",
	    "Networking",
	    "Listen Port",
	    "Server Address",
	    "Server Port",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Network",
	    "Check Updates",
	    "Networking",
	    "Listen Port",
	    "Server Address",
	    "Server Port",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Network",
	    "Check Updates",
	    "Networking",
	    "Listen Port",
	    "Server Address",
	    "Server Port",
	    "Back",
	    nullptr } },
	{ /* -------------------- *
         * --- PHYSICS      --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Physics",
	    "Gravity",
	    "Viscosity",
	    "Land Slide",
	    "Land Slide Delay",
	    "Wall Type",
	    "Boxed Mode",
	    "Boxed Ceiling Wrapping",
	    "Violent Death",
	    "Timed Shots",
	    "Volley Delay",
	    "Explosion Debris",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    "Física",
	    "Gravidade",
	    "Viscosidade",
	    "Deslizamento de Terra",
	    "Corrediça da terra atrasa",
	    "Tipo de Parede",
	    "Modalidade encaixotada",
	    "Boxed Ceiling Wrapping",
	    "Morte violenta",
	    "Tiro programado",
	    "Volley Delay",
	    "Explosion Debris",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    "Physique",
	    "Gravité",
	    "Viscosité",
	    "Glissements de terrain",
	    "Délai glissements de terrain",
	    "Murs",
	    "Enfermé dans boîte",
	    "Boxed Ceiling Wrapping",
	    "Mort violente",
	    "Projectile synchronisé",
	    "Volley Delay",
	    "Explosion Debris",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Physik",
	    "Gravitation",
	    "Reibung",
	    "Erdrutsch",
	    "Erdrutsch Verzögerung",
	    "Wand Art",
	    "Höhlenmodus",
	    "Höhlendeckenwarp",
	    "Gewalttätiger Tod",
	    "Zeitlimit",
	    "Mehrfachschussverzögerung",
	    "Explosionsschrott",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    "Fyzika",
	    "Gravitácia",
	    "Viskozita",
	    "Zosun zeme",
	    "Zdržanie zosunu zeme",
	    "Typ steny",
	    "Režim krabíc",
	    "Boxed Ceiling Wrapping",
	    "Krutá smrť",
	    "Časované strely",
	    "Volley Delay",
	    "Explosion Debris",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    "Физика",
	    "Гравитация",
	    "Сила трения",
	    "Падение земли",
	    "Задержка падения земли",
	    "Тип стен",
	    "Потолок",
	    "Boxed Ceiling Wrapping",
	    "Мощные взрывы танков",
	    "Задержка выстрела",
	    "Volley Delay",
	    "Explosion Debris",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Physics",
	    "Gravity",
	    "Viscosity",
	    "Land Slide",
	    "Land Slide Delay",
	    "Wall Type",
	    "Boxed Mode",
	    "Boxed Ceiling Wrapping",
	    "Violent Death",
	    "Timed Shots",
	    "Volley Delay",
	    "Explosion Debris",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Physics",
	    "Gravity",
	    "Viscosity",
	    "Land Slide",
	    "Land Slide Delay",
	    "Wall Type",
	    "Boxed Mode",
	    "Boxed Ceiling Wrapping",
	    "Violent Death",
	    "Timed Shots",
	    "Volley Delay",
	    "Explosion Debris",
	    "Back",
	    nullptr } },
	{           /* -------------------- *
         * --- PLAY         --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Select Players",
	    "Rounds",
	    "New Game Name",
	    "or Load Game",
	    "Load Game",
	    "Campaign",
	    "Okay",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    /* ===== Needs to be translated ===== */
	    "Select Players",
	    "Rounds",
	    "New Game Name",
	    "or Load Game",
	    "Load Game",
	    "Campaign",
	    "Okay",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    /* ===== Needs to be translated ===== */
	    "Select Players",
	    "Rounds",
	    "New Game Name",
	    "or Load Game",
	    "Load Game",
	    "Campaign",
	    "Okay",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Spieler auswählen",
	    "Rundenanzahl",
	    "Neues Spiel",
	    "oder Spiel laden",
	    "Spiel laden",
	    "Kampagne",
	    "Starten",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    "Výber hráčov",
	    "Kolá",
	    "Názov novej hry",
	    "alebo načítať hru",
	    "Načítať hru",
	    "Kampaň",
	    "OK",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    "Выберите игроков",
	    "Кол-во раундов",
	    "Имя для игры",
	    "или имя прошлой игры",
	    "Загрузить игру",
	    "Кампания",
	    "OK",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Select Players",
	    "Rounds",
	    "New Game Name",
	    "or Load Game",
	    "Load Game",
	    "Campaign",
	    "Okay",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Select Players",
	    "Rounds",
	    "New Game Name",
	    "or Load Game",
	    "Load Game",
	    "Campaign",
	    "Okay",
	    "Back",
	    nullptr } },
	{   /* -------------------- *
         * --- CPlayer       --- *
         * -------------------- *
         * Note: The title says "New Player", but this class is used for the
         * player editing, too. There the title is substituted by the player
         * name.
         * Further the "New Player" screen itself does not display the
         * "Delete This Player" option.
         */
	  { /* === EL_ENGLISH === */
	    "New Player",
	    "Name",
	    "Colour",
	    "Type",
	    "Team",
	    "Generate Pref",
	    "Played",
	    "Won",
	    "Tank Type",
	    "Delete This Player",
	    "Okay",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    /* ===== Needs to be translated ===== */
	    "New Player",
	    "Name",
	    "Colour",
	    "Type",
	    "Team",
	    "Generate Pref",
	    "Played",
	    "Won",
	    "Tank Type",
	    "Delete This Player",
	    "Okay",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    /* ===== Needs to be translated ===== */
	    "New Player",
	    "Name",
	    "Colour",
	    "Type",
	    "Team",
	    "Generate Pref",
	    "Played",
	    "Won",
	    "Tank Type",
	    "Delete This Player",
	    "Okay",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Neuer Spieler",
	    "Name",
	    "Farbe",
	    "Typ",
	    "Team",
	    "Erzeuge Konfig",
	    "Gespielt",
	    "Gewonnen",
	    "Panzertyp",
	    "Diesen Spieler Löschen",
	    "Anlegen",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    /* ===== Needs to be translated ===== */
	    "New Player",
	    "Name",
	    "Colour",
	    "Type",
	    "Team",
	    "Generate Pref",
	    "Played",
	    "Won",
	    "Tank Type",
	    "Delete This Player",
	    "OK",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    /* ===== Needs to be translated ===== */
	    "New Player",
	    "Name",
	    "Colour",
	    "Type",
	    "Team",
	    "Generate Pref",
	    "Played",
	    "Won",
	    "Tank Type",
	    "Delete This Player",
	    "OK",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "New Player",
	    "Name",
	    "Colour",
	    "Type",
	    "Team",
	    "Generate Pref",
	    "Played",
	    "Won",
	    "Tank Type",
	    "Delete This Player",
	    "Okay",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "New Player",
	    "Name",
	    "Colour",
	    "Type",
	    "Team",
	    "Generate Pref",
	    "Played",
	    "Won",
	    "Tank Type",
	    "Delete This Player",
	    "Okay",
	    "Back",
	    nullptr } },
	{   /* -------------------- *
         * --- PLAYERS      --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Players",
	    "Create New",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    /* ===== Needs to be translated ===== */
	    "Players",
	    "Create New",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    /* ===== Needs to be translated ===== */
	    "Players",
	    "Create New",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Spieler",
	    "Neuer Spieler",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    /* ===== Needs to be translated ===== */
	    "Players",
	    "Create New",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    /* ===== Needs to be translated ===== */
	    "Players",
	    "Create New",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Players",
	    "Create New",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Players",
	    "Create New",
	    "Back",
	    nullptr } },
	{   /* -------------------- *
         * --- RESET        --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Reset Options?",
	    "Reset",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    /* ===== Needs to be translated ===== */
	    "Reset Options?",
	    "Reset",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    /* ===== Needs to be translated ===== */
	    "Optionen zurücksetzen?",
	    "Zurücksetzen",
	    "Abbruch",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Reset Options?",
	    "Reset",
	    "Back",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    /* ===== Needs to be translated ===== */
	    "Reset Options?",
	    "Reset",
	    "Back",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    /* ===== Needs to be translated ===== */
	    "Reset Options?",
	    "Reset",
	    "Back",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Reset Options?",
	    "Reset",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Reset Options?",
	    "Reset",
	    "Back",
	    nullptr } },
	{   /* -------------------- *
         * --- SOUND        --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Sound",
	    "All Sound",
	    "Sound Driver",
	    "Music",
	    "Volume Factor",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    "Som",
	    "Efeitos de Som",
	    "Sistema de Som",
	    "Música",
	    "Volume Factor",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    "Sound",
	    "Effets Sonores",
	    "Système de Son",
	    "Musique",
	    "Volume Factor",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Sounds",
	    "Alle Sounds",
	    "Sound Treiber",
	    "Musik",
	    "Lautstärkefaktor",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    "Zvuk",
	    "Všetky zvuky",
	    "Ovládač zvuku",
	    "Hudba",
	    "Volume Factor",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    /* ===== Needs to be translated ===== */
	    "Sound",
	    "All Sound",
	    "Sound Driver",
	    "Music",
	    "Volume Factor",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Sound",
	    "All Sound",
	    "Sound Driver",
	    "Music",
	    "Volume Factor",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Sound",
	    "All Sound",
	    "Sound Driver",
	    "Music",
	    "Volume Factor",
	    "Back",
	    nullptr } },
	{   /* -------------------- *
         * --- WEATHER      --- *
         * -------------------- */
	  { /* === EL_ENGLISH === */
	    "Weather",
	    "Meteor Showers",
	    "Lightning",
	    "Falling Dirt",
	    "Laser Satellite",
	    "Fog",
	    "Max Wind Strength",
	    "Wind Variation",
	    "Back",
	    nullptr },
         { /* ===	EL_PORTUGUESE === */
	    "Condições Meteorológicas",
	    "Chuvas de Meteoro",
	    "Relâmpagos",
	    "Sujeira de queda",
	    "Satélite do Laser",
	    "Neblina",
	    "Velocidade Max do Vento",
	    "Variação do Vento",
	    "Back",
	    nullptr },
         { /* ===	EL_FRENCH === */
	    "Météo",
	    "Orages de météorites",
	    "Éclairs",
	    "Saleté en chute",
	    "Satellites Laser",
	    "Brouillard",
	    "Force maxi du vent",
	    "Variation du vent",
	    "Back",
	    nullptr },
         { /* ===	EL_GERMAN === */
	    "Wetter",
	    "Meteoritenregen",
	    "Gewitter",
	    "Schmutzregen",
	    "Lasersatellit",
	    "Nebel",
	    "Max Windstärke",
	    "Windveränderung",
	    "Zurück",
	    nullptr },
         { /* ===	EL_SLOVAK === */
	    "Počasie",
	    "Dážď meteorov",
	    "Blesky",
	    "Padajúca zem",
	    "Laserový satelit",
	    "Hmla",
	    "Maximálna sila vetra",
	    "Zmena vetra",
	    "Späť",
	    nullptr },
         { /* ===	EL_RUSSIAN === */
	    "Погода",
	    "Метеоритный дождь",
	    "Молнии",
	    "Падающая грязь",
	    "Удары со спутника",
	    "Туман",
	    "Макс. сила ветра",
	    "Изменения силы ветра",
	    "Назад",
	    nullptr },
         { /* ===	EL_SPANISH === */
	    /* ===== Needs to be translated ===== */
	    "Weather",
	    "Meteor Showers",
	    "Lightning",
	    "Falling Dirt",
	    "Laser Satellite",
	    "Fog",
	    "Max Wind Strength",
	    "Wind Variation",
	    "Back",
	    nullptr },
         { /* ===	EL_ITALIAN === */
	    /* ===== Needs to be translated ===== */
	    "Weather",
	    "Meteor Showers",
	    "Lightning",
	    "Falling Dirt",
	    "Laser Satellite",
	    "Fog",
	    "Max Wind Strength",
	    "Wind Variation",
	    "Back",
	    nullptr } }
};


/** @brief string array for the option text class content
 *
 * The ordering, although it looks a bit overwhelming here, is quite simple.
 * The first index is the text class, the second is the language.
 *
 * With this both translation and adding new content is fairly easy. Just
 * copy a block (after adding new enum entries at the proper places in
 * optiontypes.h) and edit to the new content.
 *
 * All text arrays end with a zero 0x0 entry. It is therefore not needed to
 * hard code any option value sizes.
 **/
char const* const OPTION_CLASS_TEXT[ TC_TEXTCLASS_COUNT ][ EL_LANGUAGE_COUNT ][ MAX_ENTRIES_PER_CLASS ] = {
	{ /* -------------------- *
         * --- TC_COLOUR   --- *
         * -------------------- */
	  /* === EL_ENGLISH === */
	  { "Regular", "Crispy", nullptr },
         /* ===	EL_PORTUGUESE === */
	  /* ===== Needs to be translated ===== */
	  { "Regular", "Crispy", nullptr },
         /* ===	EL_FRENCH === */
	  { "Régulier", "Croustillant", nullptr },
         /* ===	EL_GERMAN === */
	  { "Normal", "Kontrastreich", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Normálna", "Svieža", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Обычная", "Четкая", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Regular", "Crispy", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Regular", "Crispy", nullptr }

	},
	{ /* --------------------- *
         * --- TC_LANDSLIDE --- *
         * --------------------- */
	  /* === EL_ENGLISH === */
	  { "None", "Tank Only", "Instant", "Gravity", "Cartoon", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Nenhum", "Tanque Somente", "Instantâneo", "Gravidade", "Cartoon", nullptr },
         /* ===	EL_FRENCH === */
	  { "Aucun", "Réservoir Seulement", "Instantané", "Gravité", "Dessin animé", nullptr },
         /* ===	EL_GERMAN === */
	  { "Keine", "Nur Panzer", "Sofort", "Schwerkraft", "Cartoon", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Žiaden", "Iba tank", "Okamžitý", "Gravitácia", "Kresl.film", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Выкл.", "Только танки", "Сразу же", "По умолчанию", "Как в мультиках", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "None", "Tank Only", "Instant", "Gravity", "Cartoon", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "None", "Tank Only", "Instant", "Gravity", "Cartoon", nullptr }

	},
	{ /* -------------------- *
         * --- TC_LANDTYPE --- *
         * -------------------- */
	  /* === EL_ENGLISH === */
	  { "Random", "Canyons", "Mountains", "Valleys", "Hills", "Foothills", "Plains", "None", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Aleatório", "Canyons", "Montanhas", "Vales", "Colinas", "Morros", "Planos", "Nenhum", nullptr },
         /* ===	EL_FRENCH === */
	  { "Aléatoire", "Canyons", "Montagnes", "Vallées", "Collines", "Contreforts", "Plaines", "Aucun", nullptr },
         /* ===	EL_GERMAN === */
	  { "Zufällig", "Canyons", "Berge", "Täler", "Hügel", "Flache Hügel", "Ebene", "Nichts", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Náhodná", "Kaňony", "Hory", "Údolia", "Kopce", "Úpätia", "Nížiny", "Žiadna", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Случайный", "Каньоны", "Горы", "Возвышенность", "Холмы", "Предгорья", "Равнины", "Выкл.", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Random", "Canyons", "Mountains", "Valleys", "Hills", "Foothills", "Plains", "None", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Random", "Canyons", "Mountains", "Valleys", "Hills", "Foothills", "Plains", "None", nullptr }

	},
	{ /* -------------------- *
         * --- TC_LANGUAGE --- *
         * -------------------- */

	  /* === EL_ENGLISH === */
	  { "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", nullptr },
         /* ===	EL_FRENCH === */
	  { "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", nullptr },
         /* ===	EL_GERMAN === */
	  { "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Anglicky", "Portugalsky", "Francúzsky", "Nemecky", "Slovensky", "Rusky", "Spanish", "Italian", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "English", "Português", "Français", "Deutsch", "Slovak", "Русский", "Spanish", "Italian", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", nullptr }

	},
	{ /* --------------------- *
         * --- TC_LIGHTNING --- *
         * --------------------- */
	  /* === EL_ENGLISH === */
	  { "Off", "Weak", "Energetic", "Violent", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Desligado", "Fraco", "Energético", "Violento", nullptr },
         /* ===	EL_FRENCH === */
	  { "Aucun", "Faible", "Energique", "Violent", nullptr },
         /* ===	EL_GERMAN === */
	  { "Aus", "Schwach", "Energetisch", "Brutal", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Vypnuté", "Slabé", "Energetické", "Kruté", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Нет", "Слабые", "Сильные", "Мощные", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "Weak", "Energetic", "Violent", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "Weak", "Energetic", "Violent", nullptr }

	},
	{ /* -------------------- *
         * --- TC_METEOR   --- *
         * -------------------- */
	  /* === EL_ENGLISH === */
	  { "Off", "Light", "Heavy", "Lethal", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Desligado", "Fraco", "Forte", "Letal", nullptr },
         /* ===	EL_FRENCH === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "Light", "Heavy", "Lethal", nullptr },
         /* ===	EL_GERMAN === */
	  { "Aus", "Leicht", "Schwer", "Tödlich", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Vypnuté", "Ľahké", "Ťažké", "Smrteľné", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Нет", "Слабый", "Сильный", "Смертельный", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "Light", "Heavy", "Lethal", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "Light", "Heavy", "Lethal", nullptr }

	},
	{ /* -------------------- *
         * --- TC_MOUSE    --- *
         * -------------------- */
	  /* === EL_ENGLISH === */
	  { "Custom", "Default", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Personalizado", "Padrão", nullptr },
         /* ===	EL_FRENCH === */
	  { "Pesonnel", "Défaut", nullptr },
         /* ===	EL_GERMAN === */
	  { "Angepasst", "Standard", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Vlastné", "Východzie", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Собственный", "По умолчанию", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Custom", "Default", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Custom", "Default", nullptr }

	},
	{ /* -------------------- *
         * --- TC_OFFON    --- *
         * -------------------- */
	  /* === EL_ENGLISH === */
	  { "Off", "On", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Desligado", "Ligado", nullptr },
         /* ===	EL_FRENCH === */
	  { "Non", "Oui", nullptr },
         /* ===	EL_GERMAN === */
	  { "Aus", "An", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Vypnuté", "Zapnuté", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Выкл.", "Вкл.", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "On", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "On", nullptr }

	},
	{ /* ----------------------- *
         * --- TC_OFFONRANDOM --- *
         * ----------------------- */
	  /* === EL_ENGLISH === */
	  { "Off", "On", "Random", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Desligado", "Ligado", "Aleatório", nullptr },
         /* ===	EL_FRENCH === */
	  { "Non", "Oui", "Hasard", nullptr },
         /* ===	EL_GERMAN === */
	  { "Aus", "An", "Zufällig", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Vypnuté", "Zapnuté", "Náhodný", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Выкл.", "Вкл.", "Случайно", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "On", "Random", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "On", "Random", nullptr }

	},
	{
         /* ----------------------- *
         * --- TC_PLAYERPREF  --- *
         * ----------------------- */
		/* === EL_ENGLISH === */
		{ "Per Game", "Only Once", nullptr },
         /* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Per Game", "Only Once", nullptr },
         /* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Per Game", "Only Once", nullptr },
         /* ===	EL_GERMAN === */
		{ "Pro Spiel", "Nur einmal", nullptr },
         /* ===	EL_SLOVAK === */
		{ "Na hru", "Iba raz", nullptr },
         /* ===	EL_RUSSIAN === */
		{ "Каждую игру заново", "Только один раз", nullptr },
         /* ===	EL_SPANISH === */
		{ "Por Juego", "Solo una vez", nullptr },
         /* ===	EL_ITALIAN === */
		{ "Per Gioco", "Only Once", nullptr },

	 },
	{
         /* ----------------------- *
         * --- TC_PLAYERTEAM  --- *
         * ----------------------- */
		/* === EL_ENGLISH === */
		{ "Rogue", "Neutral", "Bastion", nullptr },
         /* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Rogue", "Neutral", "Bastion", nullptr },
         /* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Rogue", "Neutral", "Bastion", nullptr },
         /* ===	EL_GERMAN === */
		{ "Rogue", "Neutral", "Bastion", nullptr },
         /* ===	EL_SLOVAK === */
		{ "Rogue", "Neutrálny", "Bastion", nullptr },
         /* ===	EL_RUSSIAN === */
		{ "Роуг", "Нейтральный", "Бастион", nullptr },
         /* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Rogue", "Neutral", "Bastion", nullptr },
         /* ===	EL_ITALIAN === */
		{ "Rogue", "Neutrale", "Bastion", nullptr },

	 },
	{
         /* ----------------------- *
         * --- TC_PLAYERTYPE  --- *
         * ----------------------- */
		/* === EL_ENGLISH === */
		{ "Human", "Useless", "Guesser", "Range", "Targetter", "Deadly", nullptr },
         /* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Human", "Useless", "Guesser", "Range", "Targetter", "Deadly", nullptr },
         /* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Human", "Useless", "Guesser", "Range", "Targetter", "Deadly", nullptr },
         /* ===	EL_GERMAN === */
		{ "Mensch", "Nutzlos", "Ratlos", "Schütze", "Scharfschütze", "Tödlich", nullptr },
         /* ===	EL_SLOVAK === */
		{ "Človek", "Nepoužiteľný", "Ten, čo háda", "Ten, čo hľadá správnu silu", "Ten, čo mieri", "Ten, čo prináša smrť", nullptr },
         /* ===	EL_RUSSIAN === */
		{ "Человек", "Ноль", "Слабый ИИ", "Средний ИИ", "Сильный ИИ", "Терминатор", nullptr },
         /* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Humano", "Inservible", "Guesser", "Rango", "Targetter", "Mortal", nullptr },
         /* ===	EL_ITALIAN === */
		{ "Umano", "Sottodotato", "Mediocre", "Medio", "Elevato", "Mortale", nullptr },

	 },
	{ /* --------------------- *
         * --- TC_SATELLITE --- *
         * --------------------- */
	  /* === EL_ENGLISH === */
	  { "Off", "Weak", "Strong", "Super", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Desligado", "Fraco", "Forte", "Super", nullptr },
         /* ===	EL_FRENCH === */
	  { "Aucun", "Faible", "Fort", "Super", nullptr },
         /* ===	EL_GERMAN === */
	  { "Aus", "Schwach", "Stark", "Super", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Vypnutý", "Slabý", "Silný", "Super", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Нет", "Слабые", "Сильные", "Супер!!", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "Weak", "Strong", "Super", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "Weak", "Strong", "Super", nullptr }

	},
	{ /* -------------------- *
         * --- TC_SKIPTYPE --- *
         * -------------------- */
	  /* === EL_ENGLISH === */
	  { "Off", "Humans Dead", nullptr },
         /* ===	EL_PORTUGUESE === */
	  /* ===== Wrong translation ? ===== */
	  { "Desligado", "Ligado", nullptr },
         /* ===	EL_FRENCH === */
	  /* ===== Wrong translation ? ===== */
	  { "Non", "Oui", nullptr },
         /* ===	EL_GERMAN === */
	  { "Aus", "Menschen Tot", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Vypnuté", "Smrť ľudí", nullptr },
         /* ===	EL_RUSSIAN === */
	  /* ===== Wrong translation ? ===== */
	  { "Выкл.", "Вкл.", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "Humans Dead", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Off", "Humans Dead", nullptr }

	},
	{ /* ----------------------- *
         * --- TC_SOUNDDRIVER --- *
         * ----------------------- */
	  /* === EL_ENGLISH === */
	  { "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", nullptr },
         /* ===	EL_PORTUGUESE === */
	  /* ===== Needs to be translated ===== */
	  { "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", nullptr },
         /* ===	EL_FRENCH === */
	  /* ===== Needs to be translated ===== */
	  { "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", nullptr },
         /* ===	EL_GERMAN === */
	  { "Automatisch", "OSS", "ESD", "ARTS", "ALSA", "JACK", nullptr },
         /* ===	EL_SLOVAK === */
	  /* ===== Needs to be translated ===== */
	  { "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", nullptr },
         /* ===	EL_RUSSIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", nullptr }

	},
	{
         /* -------------------- *
         * --- TC_TANKTYPE --- *
         * -------------------- */
		/* === EL_ENGLISH === */
		{ "Normal", "Classic", "Big Grey", "T34", "Heavy", "Future", "UFO", "Spider", "Big Foot", "Mini", nullptr },
         /* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Normal", "Classic", "Big Grey", "T34", "Heavy", "Future", "UFO", "Spider", "Big Foot", "Mini", nullptr },
         /* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Normal", "Classic", "Big Grey", "T34", "Heavy", "Future", "UFO", "Spider", "Big Foot", "Mini", nullptr },
         /* ===	EL_GERMAN === */
		{ "Normal", "Klassisch", "Der Große Graue", "T34", "Schwergewicht", "Futuristisch", "UFO", "Spinne", "Big Foot", "Mini", nullptr },
         /* ===	EL_SLOVAK === */
		{ "Bežný", "Klasický", "Veľký šedý", "T34", "Ťažký", "Futuristický", "UFO", "Spider", "Big Foot", "Mini", nullptr },
         /* ===	EL_RUSSIAN === */
		{ "Обычный", "В старом стиле", "Большой Серый Танк", "Т-34", "Heavy", "Future", "UFO", "Spider", "Big Foot", "Mini", nullptr },
         /* ===	EL_SPANISH === */
		{ "Normal", "Clasico", "Big Grey", "T34", "Pesado", "Futuro", "UFO", "Araña", "Big Foot", "Mini", nullptr },
         /* ===	EL_ITALIAN === */
		{ "Normale", "Classico", "Big Grey", "T34", "Pesante", "Futuro", "UFO", "Spider", "Big Foot", "Mini", nullptr },

	 },
	{ /* -------------------- *
         * --- TC_TURNTYPE --- *
         * -------------------- */
	  /* === EL_ENGLISH === */
	  { "High+", "Low+", "Random", "Simul", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Melhores+", "Piores+", "Aleatório", "Simular", nullptr },
         /* ===	EL_FRENCH === */
	  { "Haut", "Bas", "Aléatoire", "Similaire", nullptr },
         /* ===	EL_GERMAN === */
	  { "Hoch+", "Niedrig+", "Zufällig", "Simul", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Vysoký+", "Nízky+", "Náhodný", "Simul", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Сильные +", "Слабые +", "Случайно", "Все сразу", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "High+", "Low+", "Random", "Simul", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "High+", "Low+", "Random", "Simul", nullptr }

	},
	{ /* -------------------- *
         * --- TC_WALLTYPE --- *
         * -------------------- */
	  /* === EL_ENGLISH === */
	  { "Rubber", "Steel", "Spring", "Wrap", "Random", nullptr },
         /* ===	EL_PORTUGUESE === */
	  { "Elástico", "Aço", "Mola", "Envoltório", "Aleatório", nullptr },
         /* ===	EL_FRENCH === */
	  { "Elastique", "Acier", "Mou", "Enveloppe", "Aléatoire", nullptr },
         /* ===	EL_GERMAN === */
	  { "Gummi", "Stahl", "Federnd", "Verbunden", "Zufällig", nullptr },
         /* ===	EL_SLOVAK === */
	  { "Guma", "Oceľ", "Pružina", "Prikrývka", "Náhodný", nullptr },
         /* ===	EL_RUSSIAN === */
	  { "Резиновые", "Непробиваемые", "Пружинящие", "Бесконечность", "Случайные", nullptr },
         /* ===	EL_SPANISH === */
	  /* ===== Needs to be translated ===== */
	  { "Rubber", "Steel", "Spring", "Wrap", "Random", nullptr },
         /* ===	EL_ITALIAN === */
	  /* ===== Needs to be translated ===== */
	  { "Rubber", "Steel", "Spring", "Wrap", "Random", nullptr }

	}
}; // End of MenuClassText


#endif // ATANKS_OPTIONCONTENT_H_INCLUDED
