#!/usr/bin/perl -w

use strict;
# The input file consists of a sequence of blocks, which can be parsed
# as SVM test cases, RVM test cases, or C functions. Each block starts
# with a header line, then a sequence of options, and finally text in
# a format defined by the block type.
#
# Header lines start with "+TYPE", optionally followed by a name. This
# name is semantically meaningful for SVM and RVM blocks; it
# determines the name of the test case.

# A C block's name is not used, and it takes no options. The body
# (which continues until the first line that looks like a header), is
# just passed straight through into the C source.

# SVM blocks' names are the GLib test case name. The underlying
# function's name is derived by substituting invalid characters with
# '_'. Note that this can result in collisions (eg, /foo_bar/baz
# collides with /foo/bar_baz). If this happens, it's your own damn
# fault; rename the blocks. SVM blocks take three different options:
# @input, @output, and @pre. The @input pragma's argument is a
# C-quoted string that gets passed into the VM as the input string,
# and @output is a C-quoted string that is compared against
# h_write_result_unamb.  @pre lines are prepended verbatim to the
# function body (with the @pre stripped, of course); they can be used
# to initialize environment values.
#
# SVM instructions consist of either two or four fields:
#
#     input_pos opcode [arg env]
#
# input_pos and opcode correspond to the fields in HRVMTrace.  arg and
# env are used to populate an HSVMAction; arg is the function, and env
# is the object whose address should be used as the env.

# RVM blocks are very similar to SVM blocks; the name and options are
# handled exactly the same way. The assembly text is handled slightly
# differently; the format is:
#
#     [label:] opcode [arg ...]
#
# For FORK and GOTO, the arg should be a label that is defined
# elsewhere.
#
# For ACTION, the arguments are handled the same way as with SVM.
#
# MATCH takes two arguments, each of which can be any C integer
# constant (not including character constants), which form the lower
# and upper bounds of the matched character, respectively.
#
# No other RVM instructions take an argument.

# At the beginning of any line, comments preceeded by '#' are allowed;
# they are replaced by C++ comments and inserted in the nearest valid
# location in the output.

my $mode == "TOP";

# common regexes:
my $re_ident = qr/[A-Za-z_][A-Za-z0-9_]*/;
my $re_cstr = qr/"(?:[^\\"]|\\["'abefnrtv0\\]|\\x[0-9a-fA-F]{2}|\\[0-7]{3})*"/;


my %svm = (
    name => sub {
	my ($env, $name) = @_;
	$env->{name} = $name;
    },
    pragma => sub {
	my ($env, $name, $val) = @_;
	if ($name eq "input") {
	    chomp($env->{input} = $val);
	} elsif ($name eq "output") {
	    chomp($env->{output} = $val);
	} elsif ($name eq "pre") {
	    # Do I have the ref precedence right here?
	    push(@$env->{pre}, $val);
	} else {
	    warn "Invalid SVM pragma";
	}
    },
    body => sub {
	my ($env, $line) = @_;
	my ($ipos, $op, $arg, $argenv);
	if ($line =~ /^\s*(\d+)\s+(PUSH|NOP|ACTION|CAPTURE|ACCEPT)(?:\s+($re_ident)\s+($re_ident))?/) {
	    if ($2 eq "PUSH") {
		# TODO: implement all the opcodes
	    }
	}
    }
    );


while (<>) {
    if (/^+(C|RVM|SVM)/) {
	$mode = $1;
    }
    
    if ($mode eq "TOP") {
	if (/^#(.*)/) {
	    print "// $1";
	    next;
	}
    } elsif ($mode eq "SVM") {
    } elsif ($mode eq "RVM") {
    } elsif ($mode eq "C") {
    }
    
}


