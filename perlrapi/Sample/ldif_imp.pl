#!/usr/bin/perl
# -*- cperl -*-
#
# Import all Contacts from a Pocket PC to a Mozilla-LDIF adressbook.
#
# AUTHOR: Andreas Pohl
#
# $Id$
use Rapi2::Simple;
use Rapi2::Defs;
use Net::LDAP::LDIF;
use Net::LDAP::Entry;

$|++;

my $file;
if(defined $ARGV[0])
{
  if($ARGV[0]=~/-h|--help/)
  {
    print "Usage: $0 [filename]\n";
    exit;
  }
  else
  {
    $file=$ARGV[0];
  }
}
$file||="adressbook.ldif";

my $rapi=new Rapi2::Simple;
my $ldif=new Net::LDAP::LDIF($file, "w", onerror => 'warn');

$rapi->OpenDB("Contacts Database") ||
  die $rapi->geterror;

my $href;
my $c=0;
while($href=$rapi->fetch)
{
  my $entry=new Net::LDAP::Entry;
  my ($desc, $givenName, $sn, $mail);
  print $c++." ";
  foreach my $key (keys %{$href})
  {
    my $attr;
    my $value=${$href->{$key}}[0];
    $value=~s/ö/oe/;
    $value=~s/Ö/OE/;
    $value=~s/ä/ae/;
    $value=~s/Ä/AE/;
    $value=~s/ü/ue/;
    $value=~s/Ü/UE/;
    $value=~s/ß/ss/;
    print ".";
    if(CE_FIRST_NAME == $key) { $attr="givenName"; $givenName=$value }
    elsif(CE_LAST_NAME == $key) { $attr="sn"; $sn=$value }
    elsif(CE_TITLE == $key) { $desc.="CE_TITLE: $value\n" }
    elsif(CE_TEL_OFFICE == $key) { $attr="telephoneNumber" }
    elsif(CE_TEL_PRIVAT == $key) { $attr="homePhone" }
    elsif(CE_COMPANY == $key) { $attr="o" }
    elsif(CE_POSITION == $key) { $desc.=$value."\n" }
    elsif(CE_DEPART == $key) { $attr="ou" }
    elsif(CE_OFFICE == $key) { $desc.="CE_OFFICE: $value\n" }
    elsif(CE_TEL_MOBIL == $key) { $attr="mobile" }
    elsif(CE_TEL_RADIO == $key) { $desc.="CE_TEL_RADIO: $value\n" }
    elsif(CE_TEL_CAR == $key) { $desc.="CE_TEL_CAR: $value\n" }
    elsif(CE_FAX_OFFICE == $key) { $desc.="CE_FAX_OFFICE: $value\n" }
    elsif(CE_FAX_PRIVAT == $key) { $attr="facsimileTelephoneNumber" }
    elsif(CE_TEL_PRIVAT2 == $key) { $desc.="CE_TEL_PRIVAT2: $value\n" }
    elsif(CE_BIRTHDAY == $key)
    {
      my ($d, $m, $y)=(localtime($value))[3,4,5];
      $desc.="CE_BIRTHDAY: ".sprintf("%04d-%02d-%02d", 1900+$y, $m+1, $d)."\n";
    }
    elsif(CE_SEKRET == $key) { $desc.="CE_SEKRET: $value\n" }
    elsif(CE_ANNYVERSARY == $key) { $desc.="CE_ANNYVERSARY: $value\n" }
    elsif(CE_TEL_SEKRET == $key) { $desc.="CE_TEL_SEKRET: $value\n" }
    elsif(CE_CHILDREN == $key) { $desc.="CE_CHILDREN: $value\n" }
    elsif(CE_TEL_OFFICE2 == $key) { $desc.="CE_TEL_OFFICE2: $value\n" }
    elsif(CE_WEBPAGE == $key) { $attr="homeurl" }
    elsif(CE_PAGER == $key) { $attr="pager" }
    elsif(CE_MATE == $key) { $desc.="(CE_MATE: $value\n" }
    elsif(CE_SALUTATION == $key) { $desc.=$value."\n" }
    elsif(CE_SEC_NAME == $key) { $attr="givenName" }
    elsif(CE_PRIVAT_STREET == $key) { $attr="homePostalAddress" }
    elsif(CE_PRIVAT_CITY == $key) { $attr="mozillaHomeLocalityName" }
    elsif(CE_PRIVAT_REGION == $key) { $attr="mozillaHomeState" }
    elsif(CE_PRIVAT_ZIP == $key) { $attr="mozillaHomePostalCode" }
    elsif(CE_PRIVAT_COUNTRY == $key) { $attr="mozillaHomeCountryName" }
    elsif(CE_OFFICE_STREET == $key) { $attr="postalAddress" }
    elsif(CE_OFFICE_CITY == $key) { $attr="l" }
    elsif(CE_OFFICE_REGION == $key) { $attr="st" }
    elsif(CE_OFFICE_ZIP == $key) { $attr="postalCode" }
    elsif(CE_OFFICE_COUNTRY == $key) { $attr="c" }
    elsif(CE_ADD_STREET == $key) { $desc.="CE_ADD_STREET: $value\n" }
    elsif(CE_ADD_CITY == $key) { $desc.="CE_ADD_CITY: $value\n" }
    elsif(CE_ADD_REGION == $key) { $desc.="CE_ADD_REGION: $value\n" }
    elsif(CE_ADD_ZIP == $key) { $desc.="CE_ADD_ZIP: $value\n" }
    elsif(CE_ADD_COUNTRY == $key) { $desc.="CE_ADD_COUNTRY: $value\n" }
    elsif(CE_EMAIL == $key) { $attr="mail"; $mail=$value }
    elsif(CE_EMAIL2 == $key) { $attr="mozillaSecondEmail" }
    elsif(CE_EMAIL3 == $key) { $desc.="CE_EMAIL3: $value\n" }
    elsif(CE_NOTE == $key) { $desc.="CE_NOTE: $value\n" }

    if(length $attr)
    {
      $entry->add($attr => $href->{$key});
    }
  }
  my $cn;
  if(length $givenName.$sn) { $cn="$givenName $sn" }
  elsif(length $mail) { $cn=$mail }
  else { $cn="no" }
  $entry->add("cn" => $cn);

  if(length $desc) { $entry->add("description" => $desc) }

  $entry->dn("cn=$cn,mail=$mail");
  $ldif->write_entry($entry);
  print " ok\n";
}
# check for an error
die $rapi->geterror if $rapi->errorstate;


$rapi->CloseDB() ||
  die $rapi->geterror;

$ldif->done;

# eof
