.PHONY:		all clean

all:		pacman.jpg ghost.jpg

pacman.jpg:	pacman.xbm
		xbmtopbm $< | pbmtopgm 1 1 | pgmtoppm yellow-black | \
		    convert - $@

ghost.jpg:	ghost.xbm
		xbmtopbm $< | pbmtopgm 1 1 | pgmtoppm magenta-black | \
		    convert - $@

clean:
		rm -f pacnman.jpg ghost.jpg
