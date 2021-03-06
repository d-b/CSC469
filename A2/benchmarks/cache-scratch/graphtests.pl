#!/usr/bin/perl

use strict;
my $dir=".";

my $graphtitle = "passive-false runtimes";

my @namelist = ("libc", "kheap", "cmu", "submit");
my %names;

# This allows you to give each series a name on the graph
# that is different from the file or directory names used
# to collect the data.  We happen to be using the same names.
$names{"libc"} = "libc";
$names{"kheap"} = "kheap";
$names{"cmu"} = "cmu";
$names{"submit"} = "submit";

my $name;
my %baseline;
my %scalability;
my %fragmentation;
my $reqd_mem = 4096; # min memory needed = 1 page, for any no. of threads
my $reftime = 1.607;

my $nthread = 8;

foreach $name (@namelist) {
    open G, "> $dir/Results/$name/data";
    $baseline{$name} = 0;
    $scalability{$name} = 0;
    $fragmentation{$name} = 0;
    for (my $i = 1; $i <= $nthread; $i++) {
	open F, "$dir/Results/$name/cache-scratch-$i";
	my $total = 0;
	my $count = 0;
	my $min = 1e30;
	my $max = -1e30;
	my $mem_min = 1e30;
	my $mem_max = -1e30;
	my $mem_total = 0;

	while (<F>) {
	    chop;
	    # Runtime results
	    if (/([0-9]+\.[0-9]+) seconds/) {
		#	 print "$i\t$1\n";
		my $current = $1;
		$total += $1;
		$count++;
		if ($current < $min) {
		    $min = $current;
		}
		if ($current > $max) {
		    $max = $current;
		}
	    }

	    # Memory usage results
	    if (/Memory used =\s+([0-9]+)\s+/) {
	      my $current = $1;
	      $mem_total += $1;
	      if ($current < $mem_min) {
		$mem_min = $current;
	      }
	      if ($current > $mem_max) {
		$mem_max = $current;
	      }
	    }
	}
	if ($count > 0) {
	    my $avg = $total / $count;
	    #	   print G "$i\t$avg\t$min\t$max\n";
	    print G "$i\t$min\n";
	    $avg = $mem_total / $count;
	    $fragmentation{$name} += $reqd_mem / $avg;
	    if ($i == 1) {
		$baseline{$name} = $min;
	    } elsif ($i < 8) {	# don't count 8 thread case
		my $speedup = $baseline{$name} / $min;
		$scalability{$name} += $speedup / $i;
	    }

	} else {
	    print "oops count is zero, $name, cache-scratch-$i\n";
	}
	close F;
    }
    close G;

    $scalability{$name} /= 6.0;
    $fragmentation{$name} /= $nthread;
    print "name = $name\n";
    printf "\tsequential speed = %.3f\n",$reftime/$baseline{$name};
    printf "\tscalability score = %.3f\n",$scalability{$name};
    printf "\tfragmentation score = %.3f\n\n",$fragmentation{$name};
}

open PLOT, "|gnuplot";
print PLOT "set term postscript\n";
print PLOT "set output \"cache-scratch.ps\"\n";
print PLOT "set title \"$graphtitle\"\n";
print PLOT "set ylabel \"Runtime (seconds)\"\n";
print PLOT "set xlabel \"Number of threads\"\n";
print PLOT "set xrange [0:9]\n";
print PLOT "set yrange [0:*]\n";
print PLOT "plot ";

foreach $name (@namelist) {
    my $titlename = $names{$name};
    if ($name eq $namelist[-1]) {
	print PLOT "\"Results/$name/data\" title \"$titlename\" with linespoints\n";
    } else {
	print PLOT "\"Results/$name/data\" title \"$titlename\" with linespoints,";
    }
}
close PLOT;


