#!/usr/bin/perl
#
# Process enum.list and output ucd.h as well as
# C code indicies for each enum.
#

sub close_frag($) {
    my($longname) = @_;

    print UCD_H "};\n\n";
    print UCD_H "int unicode_property_names_\L${longname}\E(enum unicode_\L${longname}\E, const char **, const char **);\n\n";

    print FRAG "};\n";
    print FRAG <<EOF;

int
unicode_property_names_\L${longname}\E(enum unicode_\L${longname}\E v,
				       const char **longname,
				       const char **shortname)
{
    const char *ln, *sn;
    int rv;

    if ( (unsigned)v >= sizeof enum_names/sizeof(struct libucd_enum_names) ) {
	ln = NULL;
	sn = NULL;
	rv = 1;
    } else {
	ln = enum_names[v].long_name;
	sn = enum_names[v].short_name;
	rv = 0;
    }
    if ( longname )  *longname  = ln;
    if ( shortname ) *shortname = sn;
    return rv;
}
EOF
    close(FRAG);
}

open(LIST,   '<', "enum.list") or die;
open(UCD_IN, '<', "ucd.h.in")  or die;
open(UCD_H,  '>', "ucd.h")     or die;
mkdir("enums", 0777);

while ( defined($line = <UCD_IN>) ) {
    last if ( $line =~ /ENUMS\;/ );
    print UCD_H $line;
}

undef $shortname;

while( defined($line = <LIST>) ) {
    chomp $line;
    next if ( $line =~ /^\s*(\#\#.*|)$/ );

    if ( $line =~ /^\#\s*(\S+)\s+\((\S+)\)\s+(long|short)\s*$/ ) {
	$prev = $longname;

	$longname  = $1;
	$shortname = $2;
	$whichname = ($3 eq 'long') ? 1 : 0;

	if ( defined($prev) ) {
	    close_frag($prev);
	}
	print UCD_H "enum unicode_\L${longname}\E {\n";
	open(FRAG, '>', "enums/${longname}.c") or die;
	print FRAG "#include \"libucd_int.h\"\n";
	print FRAG "static const struct libucd_enum_names enum_names[] = {\n";

	$seqpos = 0;
    } elsif ( $line =~ /\;/ ) {
	$line =~ s/\s*\#.*$//;	# Remove comments
	@list = split(/\s*;\s*/, $line);

	if ( shift(@list) ne $shortname ) {
	    die "$0: Unexpected line: $line\n";
	}

	undef $epos;
	if ( $list[0] =~ /^[0-9]+$/ ) {
	    $epos = shift(@list);
	}

	# Write ucd.h

	# $na is the preferred name, $nx the alternate
	($na = $list[$whichname]) =~ tr/-/_/;
	($nx = $list[1-$whichname]) =~ tr/-/_/;

	# If the Unicode Consortium defines a numeric value,
	# use that, otherwise use the sequential order in enum.list.
	# For that reason, enum.list should be maintained manually
	# and entries may only be added to the end of lists.

	$epos = defined($epos) ? $epos : $seqpos;
	$seqpos++;

	printf UCD_H "  %s = %d,\n",
	"UC_\U${shortname}\E_${na}", $epos;
	if ($nx ne 'n/a' && $nx ne $na && $nx) {
	    printf UCD_H "  %s = %d,\n",
	    "UC_\U${shortname}\E_${nx}", $epos;
	}

	# Write generator fragment

	($ln = $list[1]) =~ tr/_/ /;
	$sn = $list[0];
	$sn = ($sn eq 'n/a') ? '0' : "\"$sn\"";
	if ( defined($epos) ) {
	    printf FRAG "\t[%3d] = { \"%s\", %s },\n", $epos, $ln, $sn;
	} else {
	    printf FRAG "\t{ \"%s\", %s },\n", $ln, $sn;
	}
    } else {
	die "$0: Cannot parse: $line\n";
    }
}

close_frag($longname);

while ( defined($line = <UCD_IN>) ) {
    print UCD_H $line;
}

close(UCD_IN);
close(UCD_H);
