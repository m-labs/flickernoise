fig2pdf = ./figfilt $(1) <$< | \
    fig2dev -L pdf -m 0.2 >$@ || { rm -rf $@; exit 1; }

PICEXT = pdf

PICS = bind stim \
       button-up button-down \
       switch-off switch-on \
       fader-0 fader-40 fader-60 fader-100 \
       pot-0 pot-90 pot-180 pot-270 \
       enc-0 enc-90 enc-270 enc-450 enc-405 \
       dial-0 dial-90 dial-180 dial-360 \
       dial-405 dial-315

.PHONY:			all xpdf clean

all:			midi.pdf

midi.pdf:		midi.tex $(PICS:%=%.$(PICEXT))
			pdflatex midi

xpdf:			midi.pdf
			xpdf $<

bind.pdf:		bind.fig
			fig2dev -L pdf -m 0.9 $< $@ || { rm -rf $@; exit 1; }

stim.pdf:		stim.fig
			fig2dev -L pdf -m 0.9 $< $@ || { rm -rf $@; exit 1; }

button-up.$(PICEXT):	button.fig
			$(call fig2pdf, button)

button-down.$(PICEXT):	button.fig
			$(call fig2pdf, button finger)

switch-off.$(PICEXT):	switch.fig
			$(call fig2pdf, switch off)

switch-on.$(PICEXT):	switch.fig
			$(call fig2pdf, switch on)

fader-0.$(PICEXT):	fader.fig
			$(call fig2pdf, min)

fader-40.$(PICEXT):	fader.fig
			$(call fig2pdf, 40)

fader-60.$(PICEXT):	fader.fig
			$(call fig2pdf, 60)

fader-100.$(PICEXT):	fader.fig
			$(call fig2pdf, max)

pot-0.$(PICEXT):	dial.fig
			$(call fig2pdf, 225deg stop)

pot-90.$(PICEXT):	dial.fig
			$(call fig2pdf, 315deg stop cw)

pot-180.$(PICEXT):	dial.fig
			$(call fig2pdf, 45deg stop cw)

pot-270.$(PICEXT):	dial.fig
			$(call fig2pdf, 135deg stop cw)

enc-0.$(PICEXT):	dial.fig
			$(call fig2pdf, 0deg)

enc-90.$(PICEXT):	dial.fig
			$(call fig2pdf, 90deg cw)

enc-270.$(PICEXT):	dial.fig
			$(call fig2pdf, 270deg cw-fast)

enc-450.$(PICEXT):	dial.fig
			$(call fig2pdf, 90deg cw-fast)

enc-405.$(PICEXT):	dial.fig
			$(call fig2pdf, 45deg ccw-slow)

dial-0.$(PICEXT):	dial.fig
			$(call fig2pdf, 0deg)

dial-90.$(PICEXT):	dial.fig
			$(call fig2pdf, 90deg cw)

dial-180.$(PICEXT):	dial.fig
			$(call fig2pdf, 180deg cw)

dial-360.$(PICEXT):	dial.fig
			$(call fig2pdf, 0deg cw-fast)

dial-405.$(PICEXT):	dial.fig
			$(call fig2pdf, 45deg cw-slow)

dial-315.$(PICEXT):	dial.fig
			$(call fig2pdf, 315deg ccw)

clean:
			rm -f $(PICS:%=%.$(PICEXT))
			rm -f midi.aux midi.log
