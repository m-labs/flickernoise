/*
 * Flickernoise
 * Copyright (C) 2010, 2011 Sebastien Bourdeauducq
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

#include <mtklib.h>
#include <fitz.h>
#include <mupdf.h>

#include "util.h"
#include "filedialog.h"
#include "color.h"
#include "messagebox.h"
#include "pdfreader.h"

static int appid;
static int file_dialog_id;

static fz_glyphcache *glyphcache;

static pdf_xref *xref;
static int page_count;
static int page_n;
static float zoom = 1.0;
static unsigned short *view_fb;

static void display_page()
{
	fz_obj *pageobj;
	fz_error error;
	pdf_page *page;
	fz_device *dev;
	fz_displaylist *list;
	fz_matrix ctm;
	fz_bbox bbox;
	fz_pixmap *pix;
	int x, y;
	unsigned char *pixels;
	int offset;

	pageobj = pdf_getpageobject(xref, page_n);
	error = pdf_loadpage(&page, xref, pageobj);
	if(error) {
		messagebox("PDF reader", "Cannot load page");
		return;
	}
	
	list = fz_newdisplaylist();
	dev = fz_newlistdevice(list);
	error = pdf_runpage(xref, page, dev, fz_identity);
	if(error) {
		messagebox("PDF reader", "Cannot draw page");
		fz_freedevice(dev);
		fz_freedisplaylist(list);
		return;
	}
	fz_freedevice(dev);
	
	ctm = fz_translate(0, -page->mediabox.y1);
	ctm = fz_concat(ctm, fz_scale(zoom, -zoom));
	bbox = fz_roundrect(fz_transformrect(ctm, page->mediabox));
	pix = fz_newpixmapwithrect(fz_devicergb, bbox);
	memset(pix->samples, 0xff, pix->w*pix->h*pix->n);
	
	dev = fz_newdrawdevice(glyphcache, pix);
	fz_executedisplaylist(list, dev, ctm);
	fz_freedevice(dev);
	
	free(view_fb);
	view_fb = malloc(pix->w*pix->h*2);
	if(view_fb != NULL) {
		pixels = pix->samples;
		for(y=0;y<pix->h;y++)
			for(x=0;x<pix->w;x++) {
				offset = 4*(pix->w*y+x);
				view_fb[pix->w*y+x] = MAKERGB565N(pixels[offset], pixels[offset+1], pixels[offset+2]);
			}
		mtk_cmdf(appid, "view.set(-fb %d -w %d -h %d)", view_fb, pix->w, pix->h);
		mtk_cmd(appid, "viewf.set(-xview 0 -yview 0)");
		mtk_cmdf(appid, "e_page.set(-text \"%d\")", page_n);
	}

	fz_droppixmap(pix);
	fz_freedisplaylist(list);
}

static void pdf_close()
{
	if(xref == NULL) return;
	
	mtk_cmd(appid, "e_page.set(-text \"\")");
	mtk_cmd(appid, "l_of.set(-text \"of --\")");
	mtk_cmd(appid, "view.set(-fb 0 -w 600 -h 440)");
	free(view_fb);
	pdf_freexref(xref);
	xref = NULL;
}

static void pdf_open(char *filename, char *password)
{
	fz_error error;
	
	pdf_close();
	
	error = pdf_openxref(&xref, filename, password);
	if(error) {
		messagebox("PDF reader", "Cannot open document");
		xref = NULL;
		return;
	}
	error = pdf_loadpagetree(xref);
	if(error) {
		messagebox("PDF reader", "Cannot load page tree");
		pdf_freexref(xref);
		xref = NULL;
		return;
	}
	
	page_count = pdf_getpagecount(xref);
	page_n = 1;
	mtk_cmd(appid, "e_page.set(-text \"1\")");
	mtk_cmdf(appid, "l_of.set(-text \"of %d\")", page_count);

	display_page();
}

static void pdf_filedialog_ok_callback()
{
	char filepath[384];

	get_filedialog_selection(file_dialog_id, filepath, sizeof(filepath));
	close_filedialog(file_dialog_id);
	
	pdf_open(filepath, "");
}

static void pdf_filedialog_cancel_callback()
{
	close_filedialog(file_dialog_id);
}

static void prev_callback(mtk_event *e, void *arg)
{
	if(xref == NULL) return;
	if(page_n > 1) {
		page_n--;
		display_page();
	}
}

static void page_callback(mtk_event *e, void *arg)
{
	if(xref == NULL) return;
	page_n = mtk_req_i(appid, "e_page.text");
	if(page_n < 1)
		page_n = 1;
	if(page_n > page_count)
		page_n = page_count;
	display_page();
}

static void next_callback(mtk_event *e, void *arg)
{
	if(xref == NULL) return;
	if(page_n < page_count) {
		page_n++;
		display_page();
	}
}

static void open_callback(mtk_event *e, void *arg)
{
	open_filedialog(file_dialog_id, "/");
}

static int w_open;

static void close_callback(mtk_event *e, void *arg)
{
	close_filedialog(file_dialog_id);
	mtk_cmd(appid, "w.close()");
	pdf_close();
	w_open = 0;
}

void init_pdfreader()
{
	appid = mtk_init_app("PDF reader");
	
	mtk_cmd_seq(appid, "g = new Grid()",
		
		"g_ctl = new Grid()",

		"b_open = new Button(-text \"Open\")",
		"b_prev = new Button(-text \"Prev\")",
		"e_page = new Entry()",
		"l_of = new Label(-text \"of --\")",
		"b_next = new Button(-text \"Next\")",
		"b_close = new Button(-text \"Close\")",
		"g_ctl.place(b_open, -column 1 -row 1)",
		"g_ctl.place(b_prev, -column 2 -row 1)",
		"g_ctl.place(e_page, -column 3 -row 1)",
		"g_ctl.place(l_of, -column 4 -row 1)",
		"g_ctl.place(b_next, -column 5 -row 1)",
		"g_ctl.place(b_close, -column 6 -row 1)",
	
		"view = new Pixmap(-w 600 -h 440)",
		"viewf = new Frame(-content view -scrollx yes -scrolly yes)",

		"g.place(g_ctl, -column 1 -row 1)",
		"g.place(viewf, -column 1 -row 2)",

		"w = new Window(-content g -title \"PDF reader\" -workh 350)",
		0);

	mtk_bind(appid, "b_open", "commit", open_callback, NULL);
	mtk_bind(appid, "b_prev", "commit", prev_callback, NULL);
	mtk_bind(appid, "e_page", "commit", page_callback, NULL);
	mtk_bind(appid, "b_next", "commit", next_callback, NULL);
	mtk_bind(appid, "b_close", "commit", close_callback, NULL);

	mtk_bind(appid, "w", "close", close_callback, NULL);

	file_dialog_id = create_filedialog("PDF selection", 0, pdf_filedialog_ok_callback, NULL, pdf_filedialog_cancel_callback, NULL);
	
	glyphcache = fz_newglyphcache();
}

void open_pdfreader_window()
{
	if(w_open) return;
	w_open = 1;
	mtk_cmd(appid, "w.open()");
}
