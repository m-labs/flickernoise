#!/usr/bin/perl
#
# eval.pl - PFPU code evaluator (experimental)
#
# Copyright 2011 by Werner Almesberger
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#


sub flush
{
	if ($nregs) {
		print 0+keys %reg, "/", (sort { $b cmp $a } keys %reg)[0],
		     "\n";
		return;
	}
	for (sort keys %use) {
		print defined $alias{$_} ? $alias{$_} : $_;
		print " = ".$reg{$_}."\n";
	}
	print $res;
}


if ($ARGV[0] eq "-r") {
	shift @ARGV;
	$nregs = 1;
}


while (<>) {
#	if (/FPVM fragment:/) {
#		&flush if $i;
#		undef %tmp;
#		undef $i;
#	}
	if (/PFPU fragment:/) {
		%use = %tmp;
		&flush if defined $i;
		print "\n" if defined $i;
		%init = %reg unless defined $i;
		undef $res;
		%reg = %init;
		undef @val;
		undef %tmp;
		$i = 0;
	}

	$reg{$1} = sprintf("%g", $2) if /^(R\d+) = ([-0-9.]+)$/;
	if (/^(R\d+) = ([-0-9.]+) (\S+)/) {
		$reg{$1} = sprintf("$3=%g", $2);
		$alias{$1} = $3;
	}

	$tmp{"R$1"} = 1 if /^\d+:.*-> R(\d+)/;
	next unless defined $i;

	next unless
	    /^(\d+):\s+(\S+)\s+(R\d+)?(,(R\d+))?.*?(->\s+(R\d+))?\s*$/;
	#     1        2       3      4 5          6     7
	($c, $op, $a, $b, $d) = ($1, $2, $3, $5, $7);
	undef $e;
	$e = $1 if /E=(\d+)>/;
	die "($i) $_" if $c != $i;

	$reg{$a} = 1 if $nregs && defined $a;
	$reg{$b} = 1 if $nregs && defined $b;

	print STDERR "$i: concurrent read/write on $a (A)\n"
	    if defined $d && $a eq $d;
	print STDERR "$i: concurrent read/write on $b (B)\n"
	    if defined $d && $b eq $d;

	$a = $reg{$a} if defined $reg{$a};
	$b = $reg{$b} if defined $reg{$b};

	if ($op eq "IF<R2>") {
		$expr = "(IF ".$reg{"R002"}." $a $b)";
		$reg{"R002"} = 1 if $nregs;
	} elsif ($op eq "VECTOUT") {
		$res = "A = $a\nB = $b\n";
	} elsif (defined $b) {
		$expr = "($op $a $b)";
	} elsif (defined $a) {
		$expr = "($op $a)";
	} else {
		$expr = "($op)";
	}

	$val[$e] = $expr if defined $e;
	$reg{$d} = $val[$i] if defined $d;
	$i++;
}
%use = %tmp;
&flush;
