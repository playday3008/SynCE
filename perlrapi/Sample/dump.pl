#!/usr/bin/perl
# -*- cperl -*-
# $Id$
$|++;

use strict;
use Rapi2::Simple;
use XML::Simple;
use MIME::Base64;

my $rapi=new Rapi2::Simple;

my $out={};
foreach my $db ($rapi->DBList())
{
  print $db->{DbInfo}->{szDbaseName}." ";
  $rapi->OpenDB($db->{OidDb}) || die "open failed: ".$rapi->geterror;
  while(my $hr=$rapi->fetch())
  {
    # encode the values with base64 to store binary data to xml
    foreach my $id (keys %$hr) { $hr->{$id}->[0]=encode_base64($hr->{$id}->[0], "") }
    push @{$out->{$db->{OidDb}}}, $hr;
    print ".";
  }
  $rapi->CloseDB() || die "close failed: ".$rapi->geterror;
  print " ok\n";
}

open F, ">dbdump.xml";
print F XMLout($out);
close F;
