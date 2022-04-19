#!/usr/bin/perl

#
#  commands.pl - Build-process tool for TECO-64 text editor.
#
#  @copyright 2020-2022 Franklin P. Johnston / Nowwith Treble Software
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIA-
#  BILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.
#
################################################################################

use strict;
use warnings;
use version; our $VERSION = '1.0.0';

use File::Basename;
use File::Spec;
use File::Slurp;
use Getopt::Long;
use XML::LibXML;
use English qw( -no_match_vars );
use autodie qw( open close );
use POSIX qw( strftime );
use Readonly;
use Carp;

Readonly my $INSERT_WARNING => '/\* \(INSERT: WARNING NOTICE\) \*/';
Readonly my $INSERT_CMDS    => '/\* \(INSERT: GENERAL COMMANDS\) \*/';
Readonly my $INSERT_E_CMDS  => '/\* \(INSERT: E COMMANDS\) \*/';
Readonly my $INSERT_F_CMDS  => '/\* \(INSERT: F COMMANDS\) \*/';

Readonly my $WARNING =>
  '///  *** Automatically generated from template file. DO NOT MODIFY. ***';

# Command-line arguments

my %args = (
    input    => undef,    # Input file (usually commands.xml)
    template => undef,    # Template file
    output   => undef,    # Output file
);

Readonly my $NAME => q{@} . 'name';

my $cmds;
my $e_cmds;
my $f_cmds;
my %scan_funcs;
my %exec_funcs;

#
#  Parse our command-line options
#

GetOptions(
    'input=s'    => \$args{input},
    'template=s' => \$args{template},
    'output=s'   => \$args{output}
);

check_file( $args{input},    'r', '-e' );
check_file( $args{template}, 'r', '-t' );
check_file( $args{output},   'w', '-o' );

read_xml();    # Parse XML file

my $template = read_file( $args{template} );

if ( $args{output} =~ /commands.h/msx )
{
    make_commands_h();
}
elsif ( $args{output} =~ /exec.h/msx )
{
    make_exec_h();
}
else
{
    croak "Unknown output file $args{output}";
}

exit;

#
#  Validate that we have a file name for specified option, that the file
#  exists, that it is readable, and that it is a plain file.
#

sub check_file
{
    my ( $file, $mode, $option ) = @_;

    croak "No file specified for $option option\n" if !$file;

    return if $mode eq 'w';

    croak "File $file does not exist"      if !-e $file;
    croak "File $file is not readable"     if !-r $file;
    croak "File $file is not a plain file" if !-f $file;

    return;
}

#
#  Get child element of current node, and validate it.
#

sub get_child
{
    my ( $tag, $child, $lineno ) = @_;

    my @child = $tag->findnodes("./$child");

    croak "Only one <$child> tag allowed for error at line $lineno"
      if ( $#child > 0 );

    return q{} if $#child < 0;

    return q{ } if length $child[0]->textContent() == 0;

    return $child[0]->textContent;
}

#
#  Get 'detail' child elements of current node, and validate it.
#

sub get_details
{
    my ( $tag, $child, $lineno ) = @_;

    my @child = $tag->findnodes("./$child");

    croak "Missing <$child> tag for error at line $lineno"
      if ( $#child < 0 );

    my @details = ();

    for my $child (@child)
    {
        push @details, $child->textContent();
    }

    return @details;
}

#
#  Create commands.h (or equivalent)
#

sub make_commands_h
{
    my $fh;
    my $file = $args{output};

    $template =~ s/$INSERT_WARNING/$WARNING/ms;
    $template =~ s/$INSERT_CMDS/$cmds/ms;
    $template =~ s/$INSERT_E_CMDS/$e_cmds/ms;
    $template =~ s/$INSERT_F_CMDS/$f_cmds/ms;

    print {*STDERR} "Creating $file\n" or croak;

    ## no critic (RequireBriefOpen)

    open $fh, '>', $file or croak "Can't open $file\n";

    print {$fh} $template or croak "Can't print to $file\n";

    close $fh or croak "Can't close $file\n";

    ## use critic

    return;
}

#
#  Create macro line for each command defined in commands.h
#

sub make_entry
{
    my ( $name, $scan, $exec, $mn_args ) = @_;

    if ( $name eq q{'} || $name eq q{\\} )
    {
        $name = "'\\$name'";
    }
    elsif ( length $name == 1 || $name =~ /^\\x..$/msx )
    {
        $name = "'$name'";
    }

    $name .= q{,};
    $name = sprintf '%-11s', $name;

    if ( $scan ne 'NULL' )
    {
        $scan_funcs{$scan} = 1;
    }

    if ( defined $exec )
    {
        $exec_funcs{$exec} = 1;
    }
    else
    {
        $exec = 'NULL';
    }

    $scan .= q{,};
    $scan = sprintf '%-15s', $scan;
    $exec .= q{,};
    $exec = sprintf '%-15s', $exec;

    my $entry = sprintf '%s', "    ENTRY($name  $scan  $exec  $mn_args),\n";

    if ( $name =~ s/^('[[:upper:]]',)/\L$1/msx )
    {
        $entry .= sprintf '%s', "    ENTRY($name  $scan  $exec  $mn_args),\n";
    }

    return $entry;
}

#
#  Create exec.h (or equivalent)
#

sub make_exec_h
{
    my $fh;
    my $file = $args{output};
    my $scan_list;
    my $exec_list;

    foreach my $key ( sort keys %scan_funcs )
    {
        $scan_list .= "extern bool $key(struct cmd *cmd);\n\n";
    }

    chomp $scan_list;

    foreach my $key ( sort keys %exec_funcs )
    {
        $exec_list .= "extern void $key(struct cmd *cmd);\n\n";
    }

    chomp $exec_list;

    my $INSERT_WARNING = '/\* \(INSERT: WARNING NOTICE\) \*/';
    my $insert_scan    = '/\* \(INSERT: SCAN FUNCTIONS\) \*/';
    my $insert_exec    = '/\* \(INSERT: EXEC FUNCTIONS\) \*/';

    $template =~ s/$INSERT_WARNING/$WARNING/ms;
    $template =~ s/$insert_scan/$scan_list/ms;
    $template =~ s/$insert_exec/$exec_list/ms;

    print {*STDERR} "Creating $file\n" or croak;

    ## no critic (RequireBriefOpen)

    open $fh, '>', $file or croak "Can't open $file\n";

    printf {$fh} $template, or croak "Can't print to $file\n";

    close $fh or croak "Can't close $file\n";

    ## use critic

    return;
}

#
#  parse_commands() - Find all TECO commands in XML data.
#

sub parse_commands
{
    my ($xml) = @_;

    my $dom = XML::LibXML->load_xml( string => $xml, line_numbers => 1 );

    my $teco = $dom->findnodes("/teco/$NAME");

    die "Can't find program name\n" if !$teco;

    my $e_cmd;

    foreach my $section ( $dom->findnodes('/teco/section') )
    {
        my $line  = $section->line_number();
        my $title = $section->getAttribute('title');

        croak "Section element missing title at line $line" if !defined $title;

        foreach my $command ( $section->findnodes('./command') )
        {
            my $name    = $command->getAttribute('name');
            my $scan    = $command->getAttribute('scan');
            my $exec    = $command->getAttribute('exec');
            my $mn_args = 'NO_ARGS';

            if ( defined $exec && $exec =~ / (.+) ! /msx )
            {
                $mn_args = 'MN_ARGS';
                $exec =~ s/ (.+) ! /$1/msx;
            }

            if ( defined $scan )
            {
                $scan = "scan_$scan";
            }
            else
            {
                $scan = 'NULL';
            }

            if ( defined $exec )
            {
                $exec = "exec_$exec";
            }

            # Note the use of a flag variable to mark when we've started parsing
            # E commands. This allows us to distinguish the 1-character FF (form
            # feed) command from the 2-character FF command.

            if ( $name =~ /^E(.)$/msx )
            {
                $e_cmds .= make_entry( $1, $scan, $exec, $mn_args );
                $e_cmd = 1;
            }
            elsif ( $name =~ /^F(.)$/msx && $e_cmd )
            {
                $f_cmds .= make_entry( $1, $scan, $exec, $mn_args );
            }
            else
            {
                if ( $name =~ /^\[(.)\]/msx )
                {
                    $name = $1;
                }

                $cmds .= make_entry( $name, $scan, $exec, $mn_args );
            }
        }
    }

    chomp $cmds;
    chomp $e_cmds;
    chomp $f_cmds;

    return;
}

#
#  read_xml() - Read XML file and extract data for each name.
#

sub read_xml
{
    print {*STDERR} "Reading configuration file $args{input}...\n" or croak;

    # Read entire input file into string.

    my $xml = do { local ( @ARGV, $RS ) = $args{input}; <> };

    parse_commands($xml);

    return;
}
