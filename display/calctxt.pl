#!/usr/bin/perl

$addr = $ARGV[0];

$decimal = hex($addr);

$addrrel = $decimal - 0x400;

print "$addr = $decimal ($addrrel)\n";

# 400, 480, 500 = each 'superrow' is 128 bytes apart.

$superrow = $addrrel >> 7;

print "superrow = $superrow\n";

$superoffset = $addrrel & 0x7F;

print "superoffset = $superoffset\n";

$subrow = int($superoffset / 40);
$charoffset = $superoffset % 40;

print "subrow = $subrow\n";
print "charoffset = $charoffset\n";

$y_loc = $subrow * 8 + $superrow;
$x_loc = $charoffset;

print "x, y = $x_loc, $y_loc\n";
