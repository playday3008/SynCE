#!/usr/bin/perl
#
# Test the Rapi2 wrapper. Uncomment the tests you want.
#
# $Id$
use strict;
use Rapi2;
use Rapi2::Defs;

Rapi2::CeRapiInit();
Rapi2::synce_log_set_level(0);

# Uncomment the tests you want.
# File access
#t_findallfiles();
#t_firstnextclosefile();
#t_createwriteclosefile("rapitest.txt");
#t_createreadclosefile("rapitest.txt");
#t_copyfile("rapitest.txt", "rapitest.txt_");
#t_createreadclosefile("rapitest.txt_");
#t_deletefile("rapitest.txt_");

# Database access
#t_findalldbs();
#t_findfirstnextclosedb();
#t_readrecordprops();
#t_seekdb();
#t_writerecordprop();
#t_createdb();
#t_deletedb();


# Database access
sub t_createdb
{
  my $n="rapitestdb";
  print "[Test CeCreateDatabase: '$n, type: 5']\n";
  my $oid=Rapi2::CeCreateDatabase($n, 5, undef);
  print "[DONE.]\n\n";
}

sub t_deletedb
{
  my $n="rapitestdb";
  # Searching the Contacts DB and open it.
  my ($access, $data, $oid);
  ($access, $data)=Rapi2::CeFindAllDatabases();
  die "CeFindAllDatabases failed" unless $access;

  $oid=0;
  foreach my $db (@$data)
  {
    if($db->FETCH("DbInfo")->{szDbaseName} eq $n)
    {
      $oid=$db->FETCH("OidDb");
    }
  }
  die "$n not found\n" unless $oid;

  print "[Test CeDeleteDatabase: '$n']\n";
  my $access=Rapi2::CeDeleteDatabase($oid);
  print "[DONE.]\n\n" if $access;
}

sub t_writerecordprop
{
  # Searching the Contacts DB and open it.
  my ($access, $data, $dbh);
  ($access, $data)=Rapi2::CeFindAllDatabases();
  die "CeFindAllDatabases failed" unless $access;

  $dbh=0;
  foreach my $db (@$data)
  {
    if($db->FETCH("DbInfo")->{szDbaseName} eq "Contacts Database")
    {
      $dbh=Rapi2::CeOpenDatabase($db->FETCH("OidDb"), "");
      last;
    }
  }
  die "Contacts Database not found\n" unless $dbh;

  # Now get the CEOID of the last record.
  my ($oid, $index)=Rapi2::CeSeekDatabase($dbh, CEDB_SEEK_BEGINNING, 5);
  die "seek failed" unless $oid;

  print "[Test CeWriteRecordProps: (record 5: propid 0x4083, val='blaaah\@blah.phu')]\n";
  my $val=new Rapi2::CEPROPVAL;
  my $u=new Rapi2::CEVALUNION;
  $u->{lpwstr}='blaaah@blah.phu';
  $val->STORE("propid", 0x4083);
  $val->STORE("type", CEVT_LPWSTR);
  $val->STORE("val", $u);

  my @a;
  push @a, $val;
  my $ret=Rapi2::CeWriteRecordProps($dbh, $oid, \@a);
  print "[DONE.]\n\n";
}

sub t_seekdb
{
  # Searching the Contacts DB and open it.
  my ($access, $data, $dbh);
  ($access, $data)=Rapi2::CeFindAllDatabases();
  die "CeFindAllDatabases failed" unless $access;

  $dbh=0;
  foreach my $db (@$data)
  {
    if($db->FETCH("DbInfo")->{szDbaseName} eq "Contacts Database")
    {
      $dbh=Rapi2::CeOpenDatabase($db->FETCH("OidDb"), "");
      last;
    }
  }
  die "Contacts Database not found\n" unless $dbh;

  print "[Test CeSeekDatabase:]\n";
  my ($oid, $index)=Rapi2::CeSeekDatabase($dbh, CEDB_SEEK_BEGINNING, 5);
  die unless $oid;
  printf "oid: 0x%x index: %d\n[DONE.]\n\n", $oid, $index;
}

sub t_opendb
{
  my $oid=shift;
  print "[Test CeOpenDatabase:]\n";
  my $dbh=Rapi2::CeOpenDatabase($oid, "");
  print "[DONE.]\n\n";
  return $dbh;
}

sub t_readrecordprops
{
  # Searching the Contacts DB and open it.
  my ($access, $data, $dbh);
  ($access, $data)=Rapi2::CeFindAllDatabases();
  die "CeFindAllDatabases failed" unless $access;

  $dbh=0;
  foreach my $db (@$data)
  {
    if($db->FETCH("DbInfo")->{szDbaseName} eq "Contacts Database")
    {
      $dbh=Rapi2::CeOpenDatabase($db->FETCH("OidDb"), "");
      last;
    }
  }
  die "Contacts Database not found\n" unless $dbh;

  print "[Test CeReadRecordProps:]\n";
  do
  {
    ($access, $data)=Rapi2::CeReadRecordProps($dbh);

    if($access)
    {
      foreach my $prop (@$data)
      {
	printf "propid: 0x%x type: 0x%x ", $prop->FETCH("propid"),
	  $prop->FETCH("type");
	if (CEVT_LPWSTR == $prop->FETCH("type"))
	{
	  print $prop->FETCH("val")->{lpwstr};
	}
	print "\n";
      }
      print "\n";
    }
  } while($access);
  print "[DONE.]\n\n";
}

sub t_findalldbs
{
  my ($access, $data, $db);
  print "[Test CeFindAllDatabases:]\n";
  ($access, $data)=Rapi2::CeFindAllDatabases();
  die unless $access;

  foreach $db (@$data)
  {
    my $dbi=$db->FETCH("DbInfo");
    print "$dbi->{szDbaseName}\n";
  }
  print "[DONE.]\n\n";
}

sub t_findfirstnextdb
{
  print "[Test CeFindFirstDatabase:]\n";
  my $dbh=Rapi2::CeFindFirstDatabase();
  my $access;
  do
  {
    my $oid=Rapi2::CeFindNextDatabase($dbh);
    $access=t_oidgetinfo($oid, OBJTYPE_DATABASE);
  } while($access);
  print "[DONE.]\n\n";
}

sub t_oidgetinfo
{
  my $oid=shift;
  my $type=shift;
  my ($access, $oidi)=Rapi2::CeOidGetInfo($oid);
  return 0 unless $access;

  my $u=$oidi->FETCH("u");
  my $ot=$oidi->FETCH("wObjType");
  return 0 unless $type == $ot;

  print "[Test CeOidGetInfo:]\n";
  if(OBJTYPE_FILE == $ot)
  {
    print "file: szFileName=$u->{infFile}->{szFileName}\n";
  }
  elsif(OBJTYPE_DIRECTORY == $ot)
  {
    print "directory: szDirName=$u->{infDirectory}->{szDirName}\n";
  }
  elsif(OBJTYPE_DATABASE == $ot)
  {
    print "database: szDbaseName=$u->{infDatabase}->{szDbaseName}\n";
  }
  elsif(OBJTYPE_RECORD == $ot)
  {
    printf "record: oidParent=0x%x\n", $u->{infRecord}->{oidParent};
  }
  else
  {
    printf "unknown object type: %d\n", $ot;
    return 0;
  }
  print "[DONE.]\n\n";
  return 1;
}

# File access
sub t_findallfiles
{
  my ($access, $data, $file);

  print "[Test CeFindAllFiles:]\n";
  ($access, $data)=Rapi2::CeFindAllFiles("\\*.*",
					 FAF_ATTRIBUTES |
					 FAF_LASTWRITE_TIME |
					 FAF_NAME|FAF_SIZE_LOW |
					 FAF_OID);
  die "Failed to get all files" unless $access;

  foreach $file (@$data)
  {
    print $file->FETCH("cFileName")."\n";
  }
  print "[DONE.]\n\n";
}

sub t_firstnextclosefile
{
  my ($access, $data, $fh, $file);

  # This test is slow, take it in if you want.
  print "[Test CeFindFirstFile/CeFindNextFile:]\n";
  ($fh, $file)=Rapi2::CeFindFirstFile("\\*.*");
  print $file->FETCH("cFileName")."\n";

  do
  {
    ($access, $file)=Rapi2::CeFindNextFile($fh);
    print $file->FETCH("cFileName")."\n" if $access;
  } while ($access);

  print "[DONE.]\n\n";

  print "[Test CeFindClose:]\n";
  $access=Rapi2::CeFindClose($fh);
  print "[DONE.]\n\n" if $access;
}

sub t_createwriteclosefile
{
  my ($access, $data, $fh, $file);
  my $testfile=shift;
  print "[Test CeCreateFile (write): name=$testfile]\n";
  $fh=Rapi2::CeCreateFile("\\$testfile", GENERIC_WRITE, 0, CREATE_ALWAYS);
  print "File opened.\n[DONE.]\n\n";

  print "[Test WriteFile:]\n";
  my $str="test the write function\n";
  $access=Rapi2::CeWriteFile($fh, $str);
  print "[DONE.]\n\n" if $access;

  print "[Test CeCloseHandle:]\n";
  $access=Rapi2::CeCloseHandle($fh);
  print "[DONE.]\n\n" if $access;
}

sub t_createreadclosefile
{
  my ($access, $data, $fh, $file);
  my $testfile=shift;

  print "[Test CeCreateFile (read): '$testfile']\n";
  $fh=Rapi2::CeCreateFile("\\$testfile");
  print "File opened.\n[DONE.]\n\n";

  print "[Test CeReadFile:]\n";
  $file="";
  do
  {
      ($access, $data)=Rapi2::CeReadFile($fh, 256);
      $file.=$data if $access;
  } while (256 == length $data);
  print "file=[$file]\n";
  $access=Rapi2::CeCloseHandle($fh);
  print "[DONE.]\n\n" if $access;
}

sub t_copyfile
{
  my ($access);
  my ($a, $b)=@_;

  print "[Test CeCopyFileA: '$a -> $b']\n";
  $access=Rapi2::CeCopyFileA($a, $b, 1);
  print "[DONE.]\n\n" if $access;
}

sub t_createdir
{
  my ($access);
  my $d=shift;
  print "[Test CeCreateDirectory: '$d']\n";
  $access=Rapi2::CeCreateDirectory($d);
  print "[DONE.]\n\n" if $access;
}

sub t_deletefile
{
  my ($access);
  my $f=shift;
  print "[Test CeDeleteFile: '$f']\n";
  $access=Rapi2::CeDeleteFile($f);
  print "[DONE.]\n\n" if $access;
}

