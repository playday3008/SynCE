# -*- cperl -*-
#
# Rapi2::Simple
#
# This module provides simple access to the Rapi2 Wrapper.
#
# AUTHOR: Andreas Pohl (d3@dimension3.de)
#
# TODO: Implement access to the complete Rapi2. ;)
#
# $Id$
package Rapi2::Simple;

use strict;

use vars qw($VERSION);
$VERSION='0.01';

use Rapi2;
use Rapi2::Defs;

# Constructor
sub new
{
  my ($class, @args)=@_;
  my $self={@args};

  # Default log level is 0. (no logging)
  $self->{log_level}||=0;

  Rapi2::synce_log_set_level($self->{log_level});

  # Starting the Rapi.
  Rapi2::CeRapiInit();

  $self->{dbh}=-1;
  $self->{errstr}=undef;

  return bless $self, $class;
}

sub log_level
{
  my ($self, $level)=@_;
  Rapi2::synce_log_set_level($level);
  $self->{log_level}=$level;
}

# Returns the errorstate. TRUE for an error, FALSE otherwise.
sub errorstate
{
  defined($_[0]->{errstr});
}

# Returns the error string and resets the errorstate.
sub geterror
{
  my $self=shift;
  my $ret=$self->{errstr};
  $self->{errstr}=undef;
  return $ret;
}

#
# Functions for easy database access.
#

# Open a DB by name or CEOID.
sub OpenDB
{
  my ($self, $param)=@_;
  my $dbh;
  $self->{errstr}=undef;
  if($self->{dbh} != -1)
  {
    $self->{errstr}="already opended database, try CloseDB first";
    return 0;
  }
  if($param!~/[a-zA-Z]/)
  {
    # Try to open a DB per CEOID.
    eval("\$dbh=Rapi2::CeOpenDatabase($param, '');");
    if(! $@)
    {
      # Success.
      $self->{dbh}=$dbh;
      return 1;
    }
  }
  # Now search the DB.
  # NOTE: This is a work around becaus CeOpenDatabase supports no opening
  # by name at this time.
  my ($access, $data);
  ($access, $data)=Rapi2::CeFindAllDatabases();
  unless($access)
  {
    $self->{errstr}="CeFindAllDatabases failed";
    return 0;
  }

  my $oid=0;
  foreach my $db (@$data)
  {
    if($db->FETCH("DbInfo")->{szDbaseName} eq $param)
    {
      $oid=$db->FETCH("OidDb");
      last;
    }
  }
  unless($oid)
  {
    $self->{errstr}="'$param' database does not exists";
    return 0;
  }
  eval("\$dbh=Rapi2::CeOpenDatabase($oid, '')");
  if($@)
  {
    # Error.
    $self->{errstr}="CeOpenDatabase failed: $@";
    return 0;
  }
  $self->{dbh}=$dbh;
  return 1;
}

# Close an opened DB.
sub CloseDB
{
  my $self=shift;
  my $ret;
  $self->{errstr}=undef;
  if($self->{dbh} == -1)
  {
    $self->{errstr}="no database opened";
    return 0;
  }
  eval("\$ret=Rapi2::CeCloseHandle(\$self->{dbh})");
  if($@)
  {
    $self->{errstr}="CeCloseHandle failed: $@";
    return 0;
  }
  unless($ret)
  {
    $self->{errstr}="CeCloseHandle failed";
    return 0;
  }
  $self->{dbh}=-1;
  return 1;
}

# Get the next record and put all entries into a hash.
sub fetch
{
  my $self=shift;
  $self->{errstr}=undef;
  if($self->{dbh} == -1)
  {
    $self->{errstr}="no database opened";
    return 0;
  }
  my ($access, $data)=Rapi2::CeReadRecordProps($self->{dbh});
  return 0 unless $access;

  my $ret={};
  foreach my $prop (@$data)
  {
    # take the propid as key
    my $key=$prop->FETCH("propid");
    my $val=$prop->FETCH("val");
    my $type=$prop->FETCH("type");
    if(CEVT_LPWSTR == $type)
    {
      $ret->{$key}=$val->{lpwstr};
    }
    elsif(CEVT_FILETIME == $type)
    {
      $ret->{$key}=$val->{filetime};
    }
    elsif(CEVT_BLOB == $type)
    {
      $ret->{$key}=$val->{blob}->{lpb};
    }
    elsif(CEVT_UI2 == $type)
    {
      $ret->{$key}=$val->{uiVal};
    }
    elsif(CEVT_UI4 == $type)
    {
      $ret->{$key}=$val->{ulVal};
    }
    elsif(CEVT_I2 == $type)
    {
      $ret->{$key}=$val->{iVal};
    }
    elsif(CEVT_I4 == $type)
    {
      $ret->{$key}=$val->{lVal};
    }
    elsif(CEVT_BOOL == $type)
    {
      $ret->{$key}=$val->{boolVal};
    }
    elsif(CEVT_R8 == $type)
    {
      $ret->{$key}=$val->{dblVal};
    }
    else
    {
      $self->{errstr}="unknown value type";
      return 0;
    }
  }
  return $ret;
}

1;
