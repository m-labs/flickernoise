#include <stdio.h>
#include <stdlib.h>

#include <dopelib.h>
#include <vscreen.h>

static long cp_app_id;

int main(int argc, char *argv[])
{
	dope_init();

	cp_app_id = dope_init_app("Control panel");

	dope_cmd_seq(cp_app_id,
		"g = new Grid()",

		"l_iosetup = new Label(-text \"Interfaces\")",
		"g_iosetup1 = new Grid()",
		"g_iosetup2 = new Grid()",
		"b_keyb = new Button(-text \"Keyboard\")",
		"b_ir = new Button(-text \"IR remote\")",
		"b_audio = new Button(-text \"Audio\")",
		"b_midi = new Button(-text \"MIDI\")",
		"b_osc = new Button(-text \"OSC\")",
		"b_dmx = new Button(-text \"DMX512\")",
		"b_videoin = new Button(-text \"Video in\")",
		"g_iosetup1.place(b_keyb, -column 1 -row 1)",
		"g_iosetup1.place(b_ir, -column 2 -row 1)",
		"g_iosetup1.place(b_audio, -column 3 -row 1)",
		"g_iosetup2.place(b_midi, -column 1 -row 2)",
		"g_iosetup2.place(b_osc, -column 2 -row 2)",
		"g_iosetup2.place(b_dmx, -column 3 -row 2)",
		"g_iosetup2.place(b_videoin, -column 4 -row 2)",
		"g.place(l_iosetup, -column 1 -row 1)",
		"g.place(g_iosetup1, -column 1 -row 2)",
		"g.place(g_iosetup2, -column 1 -row 3)",

		"l_presets = new Label(-text \"Presets\")",
		"g_presets = new Grid()",
		"b_editor = new Button(-text \"Preset editor\")",
		"b_monitor = new Button(-text \"Variable monitor\")",
		"g_presets.place(b_editor, -column 1 -row 1)",
		"g_presets.place(b_monitor, -column 2 -row 1)",
		"g.place(l_presets, -column 1 -row 4)",
		"g.place(g_presets, -column 1 -row 5)",
		
		"l_performance = new Label(-text \"Performance\")",
		"g_performance = new Grid()",
		"b_start = new Button(-text \"Start!\")",
		"b_load = new Button(-text \"Load\")",
		"b_save = new Button(-text \"Save\")",
		"g_performance.place(b_load, -column 1 -row 1)",
		"g_performance.place(b_save, -column 2 -row 1)",
		"g.place(l_performance, -column 1 -row 6)",
		"g.place(b_start, -column 1 -row 7)",
		"g.place(g_performance, -column 1 -row 8)",

		"l_system = new Label(-text \"System\")",
		"g_system = new Grid()",
		"b_autostart = new Button(-text \"Autostart\")",
		"b_filemanager = new Button(-text \"File manager\")",
		"b_shutdown = new Button(-text \"Shutdown\")",
		"g_system.place(b_autostart, -column 1 -row 1)",
		"g_system.place(b_filemanager, -column 2 -row 1)",
		"g_system.place(b_shutdown, -column 3 -row 1)",
		"g.place(l_system, -column 1 -row 9)",
		"g.place(g_system, -column 1 -row 10)",

		"w = new Window(-content g -title \"Control panel\")",

		"w.open()",
		0);
	
	dope_eventloop(0);

	dope_deinit();

	return 0;
}
