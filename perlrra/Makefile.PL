# Makefile.PL for RRA.pm
#
# To build the wrapper run:
#    perl Makefile.PL [-I<dir> [-I...]]
#    make
#    make install
#
# $Id$
use strict;

my $module="RRA";
my $version="0.01";
my $filename="perlrra_wrap";

$|++;

# Add the include parameters passed to this script.
my $inc="-I/usr/include -I/usr/local/inlcude ".
  join(" ", grep{substr($_, 0, 2) eq "-I"}@ARGV);

# Create the wrapper
print "Creating $filename.c\n";
my $ret=`swig $inc -perl -shadow -module $module -o $filename.c $filename.i`;
die $ret if length $ret;

# Manipulate the resulting mudule
print "Creating $module.pm\n";
my ($file, $exp, @var, $c);
open F, "<$module.pm";
while(<F>)
{
  if ($_!~/^\*swig/ && /^\*(.+) =/ && $c==1)
  {
    # replace: *<define> = <$module>c::<define>;
    #    with: sub <define> { $<$module>c::<define> }
    # This makes define's from the header usable like define's in C/C++.
    # for ex.: #define DEF 5
    #      --> sub DEF { $<module>c::DEF }
    #      --> print DEF; # prints out 5.
    $file.="sub $1 { \$".$module."c::$1 }\n";
  }
  else
  {
    # Put in the version.
    $file.="\$VERSION='$version';\n\n" if /BASE METHODS/;
    $file.=$_;
  }
  # Get everything to be exported.
  $exp.="\n  $1" if $_!~/^\*swig/ && /^\*(.+) =/;
  $c=1 if /VARIABLE STUBS/;
}
close F;

$file=~s/EXPORT = qw\( \)/EXPORT = qw\($exp \)/;

open F, ">$module.pm";
print F $file;
close F;

# Create the Makefile
use ExtUtils::MakeMaker;
WriteMakefile('AUTHOR'       => 'Andreas Pohl <osar@users.sourceforge.net>',
	      'NAME'         => $module,
	      'LIBS'         => ['-lrra'],
	      'OBJECT'       => "$filename.o",
	      'VERSION_FROM' => "$module.pm"
	      #'DIR'          => [qw(Simple Defs)]
);

print "\nEverything seems to be OK. Run `make' now!\n\n";
