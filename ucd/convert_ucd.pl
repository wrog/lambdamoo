#!/usr/bin/perl
#
# Perl script to convert the Unicode Character Database source files
# into data for libucd.
#

#
# Internally this file uses a hash with the UCS value as key, and
# as data another hash from property name to value.
#

%ucs_props = ();

sub parse_unicode_string($) {
    my ($str) = @_;
    my @str = split(/\s+/, $str, 0);
    my @xv = ();
    my $x;

    foreach $x ( @str ) {
	push(@xv, hex $x);
    }

    return pack("U*", @xv);
}

#
# File to read a UCD file with a list of properties (no names)
#
sub read_separated_file($$$) {
    my ($filename, $proplist, $default) = @_;
    my $fh;
    my $line, @fields, $c0, $c1, $c;
    my $was_first = 0;

    print STDERR "Reading $filename\n";
    open($fh, '<', $filename) or return 0;
    while ( defined($line = <$fh>) ) {
	chomp $line;
	$line =~ s/\s*(|\#.*)$//;
	@fields = split(/\s*\;\s*/, $line);

	if ( $fields[0] =~ /^([0-9a-f]+)(|..([0-9a-f]+))$/i ) {
	    if ( $was_first ) {
		$c1 = hex $1;
		$was_first = 0;
	    } else {
		$c0 = hex $1;
		$c1 = ($2 eq '') ? $c0 : hex $3;
	    }

	    for ( $c = $c0 ; $c <= $c1 ; $c++ ) {
		my $p, $f;

		$ucs_props{$c} = {} unless ( exists($ucs_props{$c}) );

		for ( $f = 1 ; $f < scalar(@fields) ; $f++ ) {
		    my $prop = ${$proplist}[$f-1];
		    if ( defined($prop) ) {
			my $type = substr($prop, 0, 1);
			my $prop = substr($prop, 1);
			my $def = ${$default}[$f-1];
			$def = sprintf("%04X", $c) if ( $def eq '=' );
			my $val = $fields[$f];
			$val = $def if ( $val eq '' );

			if ( $type eq 'b' ) {
			    # Boolean (Y/N)
			    $val = ($val eq 'N') ? 0 : 1;
			} elsif ( $type eq 'p' ) {
			    # Code point
			    $val = hex $val;
			} elsif ( $type eq 's' ) {
			    # String of code points
			    $val = parse_unicode_string($val);
			} elsif ( $type eq '!' ) {
			    # Special case
			    if ( $prop eq 'Name' ) {
				# In UnicodeData.txt, ranges aren't encoded the same way
				# as elsewhere, instead the first and last code point are
				# entered, with <..., first> or <..., last> as the name
				if ( $val =~ /^\<.*, First\>$/ ) {
				    $was_first = 1;
				}
				$val = undef if ( $val =~ /^\<.*\>$/ );
			    } elsif ( $prop eq 'Decomposition' ) {
				$prop = 'Decomposition_Mapping';
				if ( $val =~ /^(\<([a-z0-9]+)\>\s*|)([0-9a-f\s]+)$/i ) {
				    my $dct = $2 || 'canonical';
				    $val = parse_unicode_string($3);

				    ${$ucs_props{$c}}{'Decomposition_Type'} = $dct;
				}
			    } else {
				die "$0: Unknown special: $prop\n";
			    }
			}

			${$ucs_props{$c}}{$prop} = $val;
		    }
		}
	    }
	}
    }
    close($fh);

    return 1;
}

#
# File to read a UCD file with boolean properties
#
sub read_boolean_file($) {
    my ($filename) = @_;
    my $fh;
    my $line, @fields, $c0, $c1, $c;

    print STDERR "Reading $filename\n";
    open($fh, '<', $filename) or return 0;
    while ( defined($line = <$fh>) ) {
	chomp $line;
	$line =~ s/\s*(|\#.*)$//;
	@fields = split(/\s*\;\s*/, $line);

	if ( $fields[0] =~ /^([0-9A-F]+)(|..([0-9A-F]+))$/ &&
	     $fields[1] ne '' ) {
	    $c0 = hex $1;
	    $c1 = ($2 eq '') ? $c0 : hex $3;

	    for ( $c = $c0 ; $c <= $c1 ; $c++ ) {
		my $p, $f;

		$ucs_props{$c} = {} unless ( exists($ucs_props{$c}) );

		${$ucs_props{$c}}{$fields[1]} = 1;
	    }
	}
    }
    close($fh);

    return 1;
}

sub make_jamo_string($) {
    my ($s) = @_;
    my $i, $c;
    my $o = '';

    $o .= "{";
    for ( $i = 0 ; $i < 4 ; $i++ ) {
	$c = substr($s,$i,1);
	$o .= length($c) ? "\'$c\'" : '0';
	$o .= ($i == 3) ? '}' : ',';
    }

    return $o;
}

# This produces tables used to generate the systematic Hangul syllables
sub make_jamo_tables() {
    my $LBase = 0x1100;
    my $VBase = 0x1161;
    my $TBase = 0x11A7;
    my $LCount = 19;
    my $VCount = 21;
    my $TCount = 28;
    my $i;
    my $fh;

    # None of the syllables exceed 4 bytes, so let's not waste
    # pointer space that might have to be relocated...

    print STDERR "Writing gen/jamo.c\n";
    open($fh, '>', 'gen/jamo.c') or die "$0 cannot create gen/jamo.c";
    print $fh "#include \"libucd_int.h\"\n\n";

    print $fh "const char _libucd_hangul_jamo_l[$LCount][4] = {\n";
    for ( $i = 0 ; $i < $LCount ; $i++ ) {
	printf $fh "\t%s,\n", make_jamo_string(${$ucs_props{$LBase+$i}}{'Jamo_Short_Name'});
    }
    print $fh "};\n";
    print $fh "const char _libucd_hangul_jamo_v[$VCount][4] = {\n";
    for ( $i = 0 ; $i < $VCount ; $i++ ) {
	printf $fh "\t%s,\n", make_jamo_string(${$ucs_props{$VBase+$i}}{'Jamo_Short_Name'});
    }
    print $fh "};\n";
    print $fh "const char _libucd_hangul_jamo_t[$TCount][4] = {\n";
    for ( $i = 0 ; $i < $TCount ; $i++ ) {
	printf $fh "\t%s,\n", make_jamo_string(${$ucs_props{$TBase+$i}}{'Jamo_Short_Name'});
    }
    print $fh "};\n";

    close($fh);
}

# This produces a names list sorted by UCS, and produces a reverse map.
%name_to_ucs   = ();

sub make_names_list() {
    my $k;
    my $pos = 0;
    my $fh;
    my $col;

    print STDERR "Writing gen/nameslist.tab\n";
    open($fh, '>', 'gen/nameslist.tab') or die;

    foreach $k ( sort {$a <=> $b} (keys(%ucs_props)) ) {
	print STDERR "Not a number: \"$k\"\n" if ( $k ne ($k+0) );

	my $n = ${$ucs_props{$k}}{'Name'};
	if ( defined($n) ) {
	    if ( defined($name_to_ucs{$n}) ) {
		printf STDERR "WARNING: Name \"%s\" duplicated from U+%04X to U+%04X\n",
		$n, $k, $name_to_ucs{$n};
	    } else {
		$name_to_ucs{$n} = $k;
		printf $fh "%05x %s\n", $k, $n;
	    }
	}
    }
    close($fh);
}

#
# Produce a list of names for automatic hash table generation.
# This includes the Hangul syllables, but not systematically
# named CJK.
#
sub write_hangul_names($$)
{
    my ($fh, $fht) = @_;
    my $SBase = 0xAC00;
    my $LBase = 0x1100;
    my $VBase = 0x1161;
    my $TBase = 0x11A7;
    my $LCount = 19;
    my $VCount = 21;
    my $TCount = 28;
    my $SCount = $LCount*$VCount*$TCount;
    my $l, $v, $t, $c;

    $c = $SBase;
    for ( $l = 0 ; $l < $LCount ; $l++ ) {
	for ( $v = 0 ; $v < $VCount ; $v++ ) {
	    for ( $t = 0 ; $t < $TCount ; $t++) {
		my $name = sprintf("HANGUL SYLLABLE %s%s%s",
				   ${$ucs_props{$LBase+$l}}{'Jamo_Short_Name'},
				   ${$ucs_props{$VBase+$v}}{'Jamo_Short_Name'},
				   ${$ucs_props{$TBase+$t}}{'Jamo_Short_Name'});
		printf $fh "%s\n", $name;
		printf $fht "%05x %s\n", $c, $name;

		$c++;
	    }
	}
    }
}

sub make_name_keyfile()
{
    my $fh, $fht;
    my $k;

    print STDERR "Writing gen/nametoucs.keys and gen/nametoucs.tab\n";
    open($fh, '>', 'gen/nametoucs.keys') or die;
    open($fht, '>', 'gen/nametoucs.tab') or die;

    foreach $k ( keys(%name_to_ucs) ) {
	printf $fh "%s\n", $k;
	printf $fht "%05x %s\n", $name_to_ucs{$k}, $k;
    }

    write_hangul_names($fh, $fht);

    close($fh);
    close($fht);
}

#
# Make a keyfile for all non-systematically named codepoints
#
sub make_named_ucs_keyfile()
{
    my $fh;
    my $k;

    print STDERR "Writing gen/ucstoname.keys\n";
    open($fh, '>', 'gen/ucstoname.keys')
	or die "$0: cannot write gen/ucstoname.keys\n";

    foreach $k ( values(%name_to_ucs) ) {
	printf $fh "%08x\n", $k;
    }

    close($fh);
}

#
# Produce a list of character properties, sans names; this is
# a test in order to figure out how much we could save from a
# range-oriented table for everything except names.
#
sub dump_prop_list()
{
    my $fh, $c;

    print STDERR "Writing gen/propdump.txt\n";
    open($fh, '>', 'gen/propdump.txt')
	or die "$0: cannot write gen/propdump.txt\n";
    binmode $fh, ':utf8';

    for ( $c = 0 ; $c <= 0x10ffff ; $c++ ) {
	my %h = %{$ucs_props{$c}};

	# Handle these separately
	delete $h{'Name'};
	delete $h{'Unicode_1_Name'};
	delete $h{'ISO_Comment'};
	delete $h{'Decomposition_Mapping'};
	# delete $h{'Uppercase_Mapping'};
	# delete $h{'Lowercase_Mapping'};
	# delete $h{'Titlecase_Mapping'};
	# delete $h{'Special_Case_Condition'};
	delete $h{'Jamo_Short_Name'};

	# Store these as offsets.
	my $k;
	foreach $k ( 'Simple_Uppercase_Mapping',
		     'Simple_Lowercase_Mapping',
		     'Simple_Titlecase_Mapping' ) {
	    if ( defined($h{$k}) ) {
		$h{$k} -= $c;	# Convert to offset
	    } else {
		$h{$k} = 0;	# Default is zero offset
	    }
	}

	my @l = sort(keys(%h));
	my $p;
	printf $fh "%05X ", $c;
	foreach $p ( @l ) {
	    print $fh $p,':',$h{$p},';';
	}
	print $fh "\n";
    }
}

#
# Produce the properties array
#
%prop_array_position = ();

sub emit_int24($) {
    my($v) = @_;
    return sprintf("{0x%02x, 0x%02x, 0x%02x}",
		    $v & 0xff,
		    ($v >> 8) & 0xff,
		    ($v >> 16) & 0xff);
}

sub make_properties_array()
{
    my $fh, $fhi, $c, $prev, $mine, $cnt, $cp;

    # List of boolean properties that translate 1:1 into flags
    my @boolean_props = ('Composition_Exclusion', 'Alphabetic', 'Default_Ignorable_Code_Point',
			 'Lowercase', 'Grapheme_Base', 'Grapheme_Extend', 'ID_Start', 'ID_Continue',
			 'Math', 'Uppercase', 'XID_Start', 'XID_Continue', 'Hex_Digit',
			 'Bidi_Control', 'Dash', 'Deprecated', 'Diacritic', 'Extender',
			 'Grapheme_Link', 'Ideographic', 'IDS_Binary_Operator',
			 'IDS_Trinary_Operator', 'Join_Control', 'Logical_Order_Exception',
			 'Noncharacter_Code_Point', 'Pattern_Syntax', 'Pattern_White_Space',
			 'Quotation_Mark', 'Radical', 'Soft_Dotted', 'STerm',
			 'Terminal_Punctuation', 'Unified_Ideograph', 'Variation_Selector',
			 'White_Space', 'Bidi_Mirrored');

    print STDERR "Writing gen/proparray.c and gen/proparrayindex\n";
    open($fh, '>', 'gen/proparray.c') or die;
    open($fhi, '>', 'gen/proparrayindex') or die;
    binmode $fh, ':utf8';

    undef $prev;
    $cnt = 0;

    for ( $c = 0 ; $c <= 0x10ffff ; $c++ ) {
	$cp = $ucs_props{$c};
	# Careful with the formatting: we rely on the fact that
	# the first 14 characters contain the UCS value and the rest
	# the properties.

	# Code point UCS value
	$mine = sprintf("\t{\n\t\t0x%05x,\n", $c);

	# General category
	my $gc = $$cp{'General_Category'} || 'Cn';
	$mine .= "\t\tUC_GC_$gc,\n";

	# Script
	my $sc = $$cp{'Script'} || 'Common';
	$mine .= "\t\tUC_SC_$sc,\n";

	# Numeric value
	my $nv = $$cp{'Numeric_Value'};
	if ( $nv > 255 ) {
	    my $exp = int(log($nv)/log(10))-1;
	    my $num = int($nv/(10**$exp));
	    $mine .= "\t\t$num, 128+$exp,\n";
	} else {
	    my $num = $nv + 0;
	    my $den = 1;

	    if ( $nv != 0 ) {
		while ( ($nv-($num/$den))/$nv > 1e-7 ) {
		    $den++;
		    $num = int($nv*$den+0.5);
		}
	    }
	    $mine .= "\t\t$num, $den,\n";
	}

	# Boolean properties and block index
	my $bp;
	foreach $bp ( @boolean_props ) {
	    if ( $$cp{$bp} ) {
		$mine .= "\t\tUC_FL_\U$bp\E |\n";
	    }
	}
	my $block = $$cp{'Block'} || 'No_Block';
	$block =~ tr/ .-/___/;
	$mine .= "\t\t((uint64_t)UC_BLK_$block << 48),\n";

	# Simple case mappings
	my $sum = ($$cp{'Simple_Uppercase_Mapping'} || $c) - $c;
	$mine .= "\t\t".emit_int24($sum).",\n";
	my $slm = ($$cp{'Simple_Lowercase_Mapping'} || $c) - $c;
	$mine .= "\t\t".emit_int24($slm).",\n";
	my $stm = ($$cp{'Simple_Titlecase_Mapping'} || $c) - $c;
	$mine .= "\t\t".emit_int24($stm).",\n";

	# Age (assume 31.7 as maximum; Unicode has traditionally not had
	# many minor versions per major version.)
	my $age = $$cp{'Age'} || '0.0';
	my (@sage) = split(/\./, $age);
	$mine .= sprintf("\t\t(%d << 3) + %d, /* $age */\n", $sage[0], $sage[1]);

	# Canonical Combining Class
	my $ccc = $$cp{'Canonical_Combining_Class'} || 'NR';
	if ( $ccc =~ /^[0-9]+$/ ) {
	    $mine .= "\t\t$ccc,\n"; # Numeric CCC
	} else {
	    $mine .= "\t\tUC_CCC_$ccc,\n";
	}

	# Sentence Break
	my $sb = $$cp{'Sentence_Break'} || 'Other';
	$mine .= "\t\tUC_SB_$sb,\n";

	# Grapheme Cluster Break
	my $gcb = $$cp{'Grapheme_Cluster_Break'} || 'Other';
	$mine .= "\t\tUC_GCB_$gcb,\n";

	# Word Break
	my $wb = $$cp{'Word_Break'} || 'Other';
	$mine .= "\t\tUC_WB_$wb,\n";

	# Arabic Joining Type
	my $ajt = $$cp{'Joining_Type'} ||
	    ($gc eq 'Mn' || $gc eq 'Me' || $gc eq 'Cf') ? 'T' : 'U';
	$mine .= "\t\tUC_JT_$ajt,\n";

	# Arabic Joining Group
	my $ajg = $$cp{'Joining_Group'} || 'No_Joining_Group';
	$ajg =~ tr/ /_/;
	$ajg =~ s/([A-Z])([A-Z]+)/$1\L$2\E/g;
	$mine .= "\t\tUC_JG_$ajg,\n";

	# East Asian Width
	my $ea = $$cp{'East_Asian_Width'} || 'N';
	$mine .= "\t\tUC_EA_$ea,\n";

	# Hangul Syllable Type
	my $hst = $$cp{'Hangul_Syllable_Type'} || 'NA';
	$mine .= "\t\tUC_HST_$hst,\n";

	# Line Break
	my $lb = $$cp{'Line_Break'} || 'XX';
	$mine .= "\t\tUC_LB_$lb,\n";

	# Numeric Type
	my $nt = $$cp{'Numeric_Type'} || 'None';
	$mine .= "\t\tUC_NT_$nt,\n";

	# Bidi Class
	my $bc = $$cp{'Bidi_Class'} || 'L';
	$mine .= "\t\tUC_BC_$bc,\n";

	# Additional properties...
	$mine .= "\t},\n";

	if ( substr($prev,14) ne substr($mine,14) ) {
	    print $fh $mine;
	    $prev = $mine;
	    printf $fhi "0x%05x $cnt\n", $c, $cnt;
	    $cnt++;
	}
	$prop_array_position{$c} = $cnt;
    }
    print $fh "\t/* Total: $cnt ranges */\n";

    close($fh);
    close($fhi);
}

#
# Import files
#
read_separated_file('ucd/UnicodeData.txt',
		    ['!Name', 'eGeneral_Category', 'nCanonical_Combining_Class',
		     'eBidi_Class', '!Decomposition', undef, undef,
		     'eNumeric_Value', 'bBidi_Mirrored',
		     'mUnicode_1_Name', 'mISO_Comment', 'pSimple_Uppercase_Mapping',
		     'pSimple_Lowercase_Mapping', 'pSimple_Titlecase_Mapping'],
		    ['<reserved>', 'Cn', 0, undef, undef, undef, undef, undef,
		     'N', undef, undef, '=', '=', '=']);

read_separated_file('ucd/extracted/DerivedNumericType.txt', ['eNumeric_Type'], []);
read_separated_file('ucd/extracted/DerivedNumericValues.txt', ['eNumeric_Value'], []);
read_separated_file('ucd/extracted/DerivedBidiClass.txt', ['eBidi_Class'], ['L']);
read_separated_file('ucd/ArabicShaping.txt', [undef, 'eJoining_Type', 'eJoining_Group'], []);
read_separated_file('ucd/BidiMirroring.txt', ['pBidi_Mirroring_Glyph'], []);
read_separated_file('ucd/Blocks.txt', ['cBlock'], []);
read_separated_file('ucd/CompositionExclusions.txt', 'bComposition_Exclusion', []);
# read_separated_file('ucd/CaseFolding.txt', ['eCase_Folding_Type', 'sCase_Folding'], []);
read_separated_file('ucd/DerivedAge.txt', ['cAge'], []);
read_separated_file('ucd/EastAsianWidth.txt', ['eEast_Asian_Width'], []);
read_separated_file('ucd/HangulSyllableType.txt', ['eHangul_Syllable_Type'], []);
read_separated_file('ucd/LineBreak.txt', ['eLine_Break'], []);
read_separated_file('ucd/Scripts.txt', ['cScript'], ['Common']);
read_separated_file('ucd/SpecialCasing.txt', ['sUppercase_Mapping', 'sLowercase_Mapping',
					  'sTitlecase_Mapping', 'mSpecial_Case_Condition'], []);
read_separated_file('ucd/Jamo.txt', ['mJamo_Short_Name'], []);
read_separated_file('ucd/auxilliary/GraphemeBreakProperty.txt', ['eGrapheme_Cluster_Break'], []);
read_separated_file('ucd/auxilliary/SentenceBreakProperty.txt', ['eSentence_Break'], []);
read_separated_file('ucd/auxilliary/WordBreakProperty.txt', ['eWord_Break'], []);
read_boolean_file('ucd/DerivedCoreProperties.txt');
read_boolean_file('ucd/PropList.txt');

#
# Produce output
#
make_jamo_tables();
make_names_list();
make_name_keyfile();
make_named_ucs_keyfile();
make_properties_array();
# dump_prop_list();
