#!/usr/bin/perl
# -*- cperl -*-
#
# ls.pl - Perl Implementation for an pls command.
#
# AUTHOR: Andreas Pohl (osar@users.sourceforge.net)
#
# $Id$
use strict;

use Rapi2::Simple;
use Rapi2::Defs;

# Parsing commandline
my ($all, $debug, $dir)=(0, 0, "/*");
my @unknown;
foreach my $arg (@ARGV)
{
  if($arg=~/-a|--all/) { $all=1 }
  elsif($arg=~/-d(\d)/) { $debug=$1 }
  elsif($arg=~/-h|--help/) { usage(); exit }
  elsif($dir eq "/*") { ($dir=$arg)=~s|/$|/\*| }
  else { push @unknown, $arg }
}
if(scalar @unknown)
{
  print "unknown arguments: ".join(" ", @unknown)."\n";
  usage();
  exit 1;
}

# Get and print the files.
my @attr=([FILE_ATTRIBUTE_DIRECTORY, 'D'], [FILE_ATTRIBUTE_ARCHIVE, 'A'],
	  [FILE_ATTRIBUTE_READONLY, 'R'], [FILE_ATTRIBUTE_COMPRESSED, 'C'],
	  [FILE_ATTRIBUTE_NORMAL, 'N'], [FILE_ATTRIBUTE_HIDDEN, 'H'],
	  [FILE_ATTRIBUTE_INROM, 'I'], [FILE_ATTRIBUTE_ROMMODULE, 'M'],
	  [FILE_ATTRIBUTE_TEMPORARY, 'T'], [FILE_ATTRIBUTE_SYSTEM, 'S']);

my $rapi=new Rapi2::Simple(log_level => $debug);
my @files=$rapi->FileList($dir);# || die "FileList failed: ".$rapi->geterror;
foreach my $file (@files)
{
  # No hidden files unless -a option specified.
  next if (! $all) && (&FILE_ATTRIBUTE_HIDDEN & $file->{dwFileAttributes});

  # Attributes
  foreach my $a (@attr){ printf "%s", ($a->[0] & $file->{dwFileAttributes})? $a->[1]: "-" }
  # Size
  printf "  %10d", $file->{nFileSizeLow};
  # Time
  my ($sec, $min, $hour, $day, $mon, $year)=localtime($file->{ftLastWriteTime});
  print $file->{dwLastWriteTime};
  printf "  %04d-%02d-%02d %02d:%02d:%02d", 1900+$year, $mon+1, $day, $hour, $min, $sec;
  # OID, Name
  printf "  0x%08x  %s", $file->{dwOID}, $file->{cFileName};
  printf "%s\n", ($file->{dwFileAttributes} & FILE_ATTRIBUTE_DIRECTORY)? "/": "";
}

# Usage
sub usage
{
print<<EOF
usage: $0 [-h|--help] [-a|--all] [-d<level>] [DIRECTORY]
Options:
    -h|--help  - Help
    -a|--all   - Show all files including those marked as hidden
    -d<level>  - Set debug log level
                      0 - No logging (default)
                      1 - Errors only
                      2 - Errors and warnings
                      3 - Everything
    DIRECTORY  - The remote directory where you want to list files

EOF
  ;
}
