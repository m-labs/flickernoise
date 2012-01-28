#!/usr/bin/perl
#
# prvm.pl - FPVM code dump prettifier
#
# Copyright 2012 by Werner Almesberger
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#

$r{"R0002"} = "R2";
while (<>) {
	next if /^==/;
	if (/^(R\S+)\s(.*)/) {
		$r = $1;
		$v = $2;
		$v = sprintf("%g", $v) if $v =~ /^[-0-9]/;
		$r{$r} = $v if $r ne "R0002";
		next;
	}
	for $r (keys %r) {
		s/\b$r\b/$r{$r}/g;
	}
	s/<R2>//;
	s/ +/ /g;
	s/^\d{4}: //;
	print;
}
