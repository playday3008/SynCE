# -*- cperl -*-
#
# Rapi2::Simple
#
# This module provides simple access to the Rapi2 Wrapper.
#
# AUTHOR: Andreas Pohl <osar@users.sourceforge.net>
#
# $Id$
package Rapi2::Simple;

use strict;
use Carp;

use vars qw($VERSION);
$VERSION='0.03';

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

  # Object type (db, file)
  $self->{type}=undef;
  # DB handle
  $self->{dbh}=-1;
  # File handle
  $self->{fh}=-1;
  # RegKey handle
  $self->{hkey}=-1;
  # error string
  $self->{errstr}=undef;

  bless $self, $class;

  if(exists $self->{file} && exists $self->{db})
  {
    croak "only one object type allowed";
  }
  if(exists $self->{db})
  {
    $self->OpenDB($self->{db});
    delete $self->{db};
  }
  elsif(exists $self->{file})
  {
    if(ref($self->{file}) eq "ARRAY")
    {
      $self->OpenFile(@{$self->{file}});
    }
    else
    {
      $self->OpenFile($self->{file});
    }
    delete $self->{file};
  }
  elsif(exists $self->{key})
  {
    if(ref($self->{key}) eq "ARRAY")
    {
      $self->OpenKey(@{$self->{key}});
      delete $self->{key};
    }
  }
  return $self;
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


#######################################
###  D A T A B A S E   A C C E S S  ###
#######################################

# Open a DB by name or CEOID.
sub OpenDB
{
  my ($self, $param)=@_;
  my $dbh;
  $self->{errstr}=undef;
  if(defined $self->{type} && $self->{type} ne "db")
  {
    $self->{errstr}="this is no database object";
    return 0;
  }
  if($self->{dbh} != -1)
  {
    $self->{errstr}="already opended database, try CloseDB first";
    return 0;
  }
  if($param!~/[a-zA-Z]/)
  {
    # Try to open a DB by CEOID.
    eval '$dbh=Rapi2::CeOpenDatabase($param, "")';
    if(! $@)
    {
      # Success.
      $self->{dbh}=$dbh;
      $self->{type}="db";
      return 1;
    }
  }
  # Now search the DB.
  # NOTE: This is a work around because CeOpenDatabase supports no opening
  # by name at this time.
  my ($access, $data);
  eval '($access, $data)=Rapi2::CeFindAllDatabases()';
  if(! $access || $@)
  {
    $self->{errstr}="CeFindAllDatabases failed: $@";
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
  eval '$dbh=Rapi2::CeOpenDatabase($oid, "")';
  if($@)
  {
    # Error.
    $self->{errstr}="CeOpenDatabase failed: $@";
    return 0;
  }
  $self->{dbh}=$dbh;
  $self->{type}="db";
  return 1;
}

# Close an opened DB.
sub CloseDB
{
  my $self=shift;
  my $ret;
  $self->{errstr}=undef;
  if($self->{type} ne "db")
  {
    $self->{errstr}="this is no database object";
    return 0;
  }
  if($self->{dbh} == -1)
  {
    $self->{errstr}="no database opened";
    return 0;
  }
  eval '$ret=Rapi2::CeCloseHandle($self->{dbh})';
  if(! $ret || $@)
  {
    $self->{errstr}="CeCloseHandle failed: $@";
    return 0;
  }
  $self->{dbh}=-1;
  $self->{type}=undef;
  return 1;
}

# Get the next record and put all entries into a hash.
sub fetch
{
  my $self=shift;
  $self->{errstr}=undef;
  if($self->{type} ne "db")
  {
    $self->{errstr}="this is no database object";
    return 0;
  }
  if($self->{dbh} == -1)
  {
    $self->{errstr}="no database opened";
    return 0;
  }
  my ($access, $data);
  eval '($access, $data)=Rapi2::CeReadRecordProps($self->{dbh})';
  $self->{errstr}="CeReadRecordProps failed: $@" if $@;
  return 0 if ! $access || $@;

  my $ret={};
  foreach my $prop (@$data)
  {
    # take the propid as key
    my $key=$prop->FETCH("propid");
    my $val=$prop->FETCH("val");
    my $type=$prop->FETCH("type");
    if(CEVT_LPWSTR == $type)
    {
      $ret->{$key}=[$val->{lpwstr}, $type];
    }
    elsif(CEVT_FILETIME == $type)
    {
      $ret->{$key}=[$val->{filetime}, $type];
    }
    elsif(CEVT_BLOB == $type)
    {
      $ret->{$key}=[$val->{blob}->{lpb}, $type];
    }
    elsif(CEVT_UI2 == $type)
    {
      $ret->{$key}=[$val->{uiVal}, $type];
    }
    elsif(CEVT_UI4 == $type)
    {
      $ret->{$key}=[$val->{ulVal}, $type];
    }
    elsif(CEVT_I2 == $type)
    {
      $ret->{$key}=[$val->{iVal}, $type];
    }
    elsif(CEVT_I4 == $type)
    {
      $ret->{$key}=[$val->{lVal}, $type];
    }
    elsif(CEVT_BOOL == $type)
    {
      $ret->{$key}=[$val->{boolVal}, $type];
    }
    elsif(CEVT_R8 == $type)
    {
      $ret->{$key}=[$val->{dblVal}, $type];
    }
    else
    {
      $self->{errstr}=sprintf "unknown value type: 0x%x", $type;
      return 0;
    }
  }
  return $ret;
}

# Write a record to an opened DB.
sub store
{
  my ($self, $data, $oid)=@_;
  $self->{errstr}=undef;
  if($self->{type} ne "db")
  {
    $self->{errstr}="this is no database object";
    return 0;
  }
  if($self->{dbh} == -1)
  {
    $self->{errstr}="no database opened";
    return 0;
  }
  if(ref($data) ne "HASH")
  {
    $self->{errstr}="expected a hash reference for argument 1";
    return 0;
  }
  $oid||=0;
  my @props;
  foreach my $propid (keys %$data)
  {
    my ($value, $type)=@{$data->{$propid}};
    my $prop=new Rapi2::CEPROPVAL;
    my $val=new Rapi2::CEVALUNION;
    $prop->{propid}=$propid;
    $prop->{type}=$type;

    if(CEVT_LPWSTR == $type)
    {
      $val->{lpwstr}=$value;
    }
    elsif(CEVT_FILETIME == $type)
    {
      $val->{filetime}=$value;
    }
    elsif(CEVT_BLOB == $type)
    {
      $val->{blob}->{lpb}=$value;
    }
    elsif(CEVT_UI2 == $type)
    {
      $val->{uiVal}=$value;
    }
    elsif(CEVT_UI4 == $type)
    {
      $val->{ulVal}=$value;
    }
    elsif(CEVT_I2 == $type)
    {
      $val->{iVal}=$value;
    }
    elsif(CEVT_I4 == $type)
    {
      $val->{lVal}=$value;
    }
    elsif(CEVT_BOOL == $type)
    {
      $val->{boolVal}=$value;
    }
    elsif(CEVT_R8 == $type)
    {
      $val->{dblVal}=$value;
    }
    else
    {
      $self->{errstr}=sprintf "unknown value type: 0x%x", $type;
      return 0;
    }
    $prop->{val}=$val;

    push @props, $prop;
  }
  my $access;
  eval '$access=Rapi2::CeWriteRecordProps($self->{dbh}, $oid, \@props)';
  if(! $access || $@)
  {
    $self->{errstr}="CeWriteRecordProps failed: $@";
    return 0;
  }
  return 1;
}

# List all databases.
sub DBList
{
  my $self=shift;
  $self->{errstr}=undef;
  my ($access, $list);
  eval '($access, $list)=Rapi2::CeFindAllDatabases()';
  if(! $access || $@)
  {
    $self->{errstr}="CeFindAllDatabases failed: $@";
    return 0;
  }

  my @ret;
  foreach my $db (@$list)
  {
    push @ret, {
		OidDb => $db->FETCH("OidDb"),
		DbInfo => $db->FETCH("DbInfo")
	       };
  }
  return @ret;
}



###############################
###  F I L E   A C C E S S  ###
###############################

# Open a file
sub OpenFile
{
  my ($self, $param, $fname)=@_;
  $self->{errstr}=undef;
  if(defined $self->{type} && $self->{type} ne "file")
  {
    $self->{errstr}="this is no file object";
    return 0;
  }
  if($self->{fh} != -1)
  {
    $self->{errstr}="already opended file, call CloseFile first";
    return 0;
  }
  my ($mode, $name);
  if(defined $fname)
  {
    ($mode, $name)=($param, $fname);
  }
  else
  {
    unless ($param=~/^([<>]{0,2})(.+)/)
    {
      $self->{errstr}="no filename";
      return 0;
    }
    ($mode, $name)=($1, $2);
  }
  my @args;
  if($mode eq "<" || ! length $mode)
  {
    # read mode
    @args=();
    $self->{fmode}="read";
  }
  elsif($mode eq ">")
  {
    # write mode
    @args=(GENERIC_WRITE, 0, CREATE_ALWAYS);
    $self->{fmode}="write";
  }
  # elsif($mode eq ">>")
  # {
  #   # append mode
  #   @args=(GENERIC_WRITE, 0, OPEN_EXISTING);
  #   $self->{fmode}="append";
  # }
  else
  {
    $self->{errstr}="no valid mode '$mode'";
    return 0;
  }
  my $fh;
  eval '$fh=Rapi2::CeCreateFile($name, @args)';
  if($@)
  {
    $self->{errstr}="CeCreateFile failed: $@";
    return 0;
  }
  # If we are in append mode, jump to the end of the file.
  if($mode eq ">>")
  {
    # TODO: Implement CeSetFilePointer!
  }
  $self->{fh}=$fh;
  $self->{type}="file";
  return 1;
}

# Close an opened file.
sub CloseFile
{
  my $self=shift;
  my $ret;
  $self->{errstr}=undef;
  if($self->{type} ne "file")
  {
    $self->{errstr}="this is no file object";
    return 0;
  }
  if($self->{fh} == -1)
  {
    $self->{errstr}="no file opened";
    return 0;
  }
  eval '$ret=Rapi2::CeCloseHandle($self->{fh})';
  if(! $ret || $@)
  {
    $self->{errstr}="CeCloseHandle failed: $@";
    return 0;
  }
  $self->{fh}=-1;
  $self->{type}=undef;
  return 1;
}

# Read a number of bytes from an opened file.
sub ReadFile
{
  my ($self, $num)=@_;
  $self->{errstr}=undef;
  if($self->{type} ne "file")
  {
    $self->{errstr}="this is no file object";
    return undef;
  }
  if($self->{fh} == -1)
  {
    $self->{errstr}="no file opened";
    return undef;
  }
  if($self->{fmode} ne "read")
  {
    $self->{errstr}="file opened in the wrong mode";
    return undef;
  }
  $num||=1024;
  my ($access, $data);
  eval '($access, $data)=Rapi2::CeReadFile($self->{fh}, $num)';
  if(! $access || $@)
  {
    $self->{errstr}="CeReadFile failed: $@";
    return undef;
  }
  return $data;
}

# Write something to an opened file.
sub WriteFile
{
  my ($self, $data)=@_;
  $self->{errstr}=undef;
  if($self->{type} ne "file")
  {
    $self->{errstr}="this is no file object";
    return 0;
  }
  if($self->{fh} == -1)
  {
    $self->{errstr}="no file opened";
    return 0;
  }
  if($self->{fmode} ne "write" && $self->{fmode} ne "append")
  {
    $self->{errstr}="file opened in the wrong mode";
    return 0;
  }
  my $access;
  eval '$access=Rapi2::CeWriteFile($self->{fh}, $data)';
  if(! $access || $@)
  {
    $self->{errstr}="CeWriteFile failed: $@";
    return 0;
  }
  return 1;
}


# Copy a file A to file B.
sub CopyFile
{
  my ($self, $a, $b)=@_;
  $self->{errstr}=undef;
  my $access;
  eval '$access=Rapi2::CeCopyFileA($a, $b, 1)';
  if(! $access || $@)
  {
    $self->{errstr}="CeCopyFileA failed: $@";
    return 0;
  }
  return 1;
}

# Move a file A to file B.
sub MoveFile
{
  my ($self, $a, $b)=@_;
  $self->{errstr}=undef;
  my $access;
  eval '$access=Rapi2::CeMoveFile($a, $b)';
  if(! $access || $@)
  {
    $self->{errstr}="CeMoveFile failed: $@";
    return 0;
  }
  return 1;
}

# Delete a File.
sub DeleteFile
{
  my ($self, $a)=@_;
  $self->{errstr}=undef;
  my $access;
  eval '$access=Rapi2::CeDeleteFile($a)';
  if(! $access || $@)
  {
    $self->{errstr}="CeDeleteFile failed: $@";
    return 0;
  }
  return 1;
}

# Delete a directory.
sub DeleteDir
{
  my ($self, $a)=@_;
  $self->{errstr}=undef;
  my $access;
  eval '$access=Rapi2::CeRemoveDirectory($a)';
  if(! $access || $@)
  {
    $self->{errstr}="CeRemoveDirectory failed: $@";
    return 0;
  }
  return 1;
}

sub FileList
{
  my ($self, $path, $flags)=@_;
  $self->{errstr}=undef;
  $flags||=FAF_ATTRIBUTES | FAF_LASTWRITE_TIME | FAF_SIZE_LOW | FAF_OID | FAF_NAME;
  my ($access, $list);
  eval '($access, $list)=Rapi2::CeFindAllFiles($path, $flags)';
  if(! $access || $@)
  {
    $self->{errstr}="CeFindAllFiles failed: $@";
    return 0;
  }

  my @ret;
  foreach my $file (@$list)
  {
    #push @ret, $file;
    push @ret, {
		dwOID => $file->FETCH("dwOID"),
		dwFileAttributes => $file->FETCH("dwFileAttributes"),
		nFileSizeHigh => $file->FETCH("nFileSizeHigh"),
		nFileSizeLow => $file->FETCH("nFileSizeLow"),
		ftCreationTime => $file->FETCH("ftCreationTime"),
		ftLastAccessTime => $file->FETCH("ftLastAccessTime"),
		ftLastWriteTime => $file->FETCH("ftLastWriteTime"),
		cFileName => $file->FETCH("cFileName")
	       };
  }
  return @ret;
}

# Get/Set file attributes.
sub FileAttr
{
  my ($self, $file, $attr)=@_;
  $self->{errstr}=undef;

  if(defined $attr)
  {
    my $access;
    eval '$access=Rapi2::CeSetFileAttributes($file, $attr)';
    if(! $access || $@)
    {
      $self->{errstr}="CeSetFileAttributes failed: $@";
      return 0;
    }
    return 1;
  }
  else
  {
    eval '$attr=Rapi2::CeGetFileAttributes($file)';
    if($@)
    {
      $self->{errstr}="CeGetFileAttributes failed: $@";
      return 0;
    }
    return $attr;
  }
}

# Get special folders.
sub GetSpecialFolderPath
{
  my ($self, $num)=@_;
  my ($access, $str);
  eval '($access, $str)=Rapi2::CeGetSpecialFolderPath($num)';
  if(! $access || $@)
  {
    $self->{errstr}="CeGetSpecialFolderPath failed: $@";
    return undef;
  }
  return $str;
}


#######################################
###  R E G I S T R Y   A C C E S S  ###
#######################################

# Open a registry key or a subkey of an already opened key.
sub OpenKey
{
  my ($self, $hkey, $subkey)=@_;
  $self->{errstr}=undef;

  if(defined $self->{type} && $self->{type} ne "reg")
  {
    $self->{errstr}="this is no registry object";
    return 0;
  }
  if(! defined($subkey))
  {
    return $self->OpenSubKey($hkey);
  }
  elsif($self->{hkey} != -1)
  {
    $self->{errstr}="already opened registry key, call CloseKey first";
    return 0;
  }
  elsif(! length($subkey) && (HKEY_CLASSES_ROOT == $hkey ||
			      HKEY_CURRENT_USER == $hkey ||
			      HKEY_LOCAL_MACHINE == $hkey ||
			      HKEY_USERS == $hkey))
  {
    $self->{hkey}=$hkey;
  }
  else
  {
    my ($access, $nhkey);
    eval '($access, $nhkey)=Rapi2::CeRegOpenKeyEx($hkey, $subkey)';
    if (ERROR_SUCCESS != $access || $@)
    {
      $self->{errstr}="CeRegOpenKeyEx failed: $@";
      return 0;
    }
    $self->{hkey}=$nhkey;
  }
  $self->{type}="reg";
  return 1;
}

# Open a subkey of an already opened registry key.
sub OpenSubKey
{
  my ($self, $subkey)=@_;
  $self->{errstr}=undef;

  if(defined $self->{type} && $self->{type} ne "reg")
  {
    $self->{errstr}="this is no registry object";
    return 0;
  }
  if($self->{hkey} == -1)
  {
    $self->{errstr}="no registry key opened";
    return 0;
  }
  my ($access, $nhkey);
  eval '($access, $nhkey)=Rapi2::CeRegOpenKeyEx($self->{hkey}, $subkey)';
  if(ERROR_SUCCESS != $access || $@)
  {
    $self->{errstr}="CeRegOpenKeyEx failed: $@";
    return 0;
  }
  # Now close the unneeded key.
  eval '$access=Rapi2::CeRegCloseKey($self->{hkey})';
  if(ERROR_SUCCESS != $access || $@)
  {
    # Set no error, if the close command failed, warn instead.
    warn "CeRegCloseKey failed: $@";
  }
  $self->{hkey}=$nhkey;
  $self->{type}="reg";
  return 1;
}

# Close an opened registry key.
sub CloseKey
{
  my $self=shift;
  $self->{errstr}=undef;

  if($self->{type} ne "reg")
  {
    $self->{errstr}="this is no registry object";
    return 0;
  }
  if($self->{hkey} == -1)
  {
    $self->{errstr}="no registry key opened";
    return 0;
  }
  my $access;
  eval '$access=Rapi2::CeRegCloseKey($self->{hkey})';
  if(ERROR_SUCCESS != $access || $@)
  {
    $self->{errstr}="CeRegCloseKey failed: $@";
    return 0;
  }
  $self->{hkey}=-1;
  $self->{type}=undef;
  return 1;
}

# Get the names of all subkey names.
sub GetSubKeyNames
{
  my $self=shift;
  $self->{errstr}=undef;

  if($self->{type} ne "reg")
  {
    $self->{errstr}="this is no registry object";
    return 0;
  }
  if($self->{hkey} == -1)
  {
    $self->{errstr}="no registry key opened";
    return 0;
  }
  my ($access, $name, @ret);
  my $i=0;
  do
  {
    eval '($access, $name)=Rapi2::CeRegEnumKeyEx($self->{hkey}, $i++)';
    if($@)
    {
      $self->{errstr}="CeRegEnumKeyEx failed: $@";
      return 0;
    }
    push @ret, $name if ERROR_SUCCESS == $access;
  } while(ERROR_SUCCESS == $access);
  return @ret;
}

# Get all value names.
sub GetValueNames
{
  my $self=shift;
  $self->{errstr}=undef;

  if($self->{type} ne "reg")
  {
    $self->{errstr}="this is no registry object";
    return 0;
  }
  if($self->{hkey} == -1)
  {
    $self->{errstr}="no registry key opened";
    return 0;
  }
  my ($access, $name, @ret);
  my $i=0;
  do
  {
    eval '($access, $name)=Rapi2::CeRegEnumValue($self->{hkey}, $i++)';
    if($@)
    {
      $self->{errstr}="CeRegEnumValue failed: $@";
      return 0;
    }
    push @ret, $name if ERROR_SUCCESS == $access;
  } while(ERROR_SUCCESS == $access);
  return @ret;
}

# Set/Get a value.
sub KeyValue
{
  my ($self, $name, $type, $val)=@_;
  $self->{errstr}=undef;

  if($self->{type} ne "reg")
  {
    $self->{errstr}="this is no registry object";
    return 0;
  }
  if($self->{hkey} == -1)
  {
    $self->{errstr}="no registry key opened";
    return 0;
  }
  if(defined($type) && defined($val))
  {
    # Set the value's type/data.
    my $access;
    eval '$access=Rapi2::CeRegSetValueEx($self->{hkey}, $name, $type, $val)';
    if (ERROR_SUCCESS != $access || $@)
    {
      $self->{errstr}="CeRegSetValueEx failed: $@";
      return 0;
    }
    return 1;
  }
  else
  {
    # Get the value's type/data.
    my ($access, $type, $val);
    eval '($access, $type, $val)=Rapi2::CeRegQueryValueEx($self->{hkey}, $name)';
    if (ERROR_SUCCESS != $access || $@)
    {
      $self->{errstr}="CeRegQueryValueEx failed: $@";
      return 0;
    }
    return ($type, $val);
  }
}

# Get infos to for a key.
sub GetInfoKey
{
  my $self=shift;
  $self->{errstr}=undef;

  if($self->{type} ne "reg")
  {
    $self->{errstr}="this is no registry object";
    return 0;
  }
  if($self->{hkey} == -1)
  {
    $self->{errstr}="no registry key opened";
    return 0;
  }
  my ($access, $cSubKeys, $cbMaxSubKeyLen, $cbMaxClassLen, $cValues,
      $cbMaxValueNameLen, $cbMaxValueLen, $cbSecurityDescriptor,
      $ftLastWriteTime);
  eval
  {
    ($access, $cSubKeys, $cbMaxSubKeyLen, $cbMaxClassLen, $cValues,
     $cbMaxValueNameLen, $cbMaxValueLen, $cbSecurityDescriptor,
     $ftLastWriteTime)=Rapi2::CeRegQueryInfoKey($self->{hkey});
  };
  if(ERROR_SUCCESS != $access || $@)
  {
    $self->{errstr}="CeRegQueryInfoKey failed: $@";
    return 0;
  }
  return ($cSubKeys, $cbMaxSubKeyLen, $cbMaxClassLen, $cValues,
	  $cbMaxValueNameLen, $cbMaxValueLen, $cbSecurityDescriptor,
	  $ftLastWriteTime);
}

1;


__END__

=head1 NAME

B<Rapi2::Simple> - Very easy access to the Rapi2 Wrapper

=head1 SYNOPSIS

  use Rapi2::Simple;
  use Rapi2::Defs;

  my $rapi;

  $rapi=new Rapi2::Simple;
  $rapi->OpenDB("Contacts Database") ||
    die $rapi->geterror;

  $rapi=new Rapi2::Simple(db => "Contacts Database");
  die $rapi->geterror if $rapi->errorstate;

  # Read out all Contacts.
  while(my $href=$rapi->fetch)
  {
    # do somthing with $href ...
  }
  die $rapi->geterror if $rapi->errorstate;

  # Create a new Contact.
  $rapi->store({
	        ::CE_TITLE => ['New Contact', CEVT_LPWSTR],
	        ::CE_FIRST_NAME => ['first', CEVT_LPWSTR],
	        ::CE_LAST_NAME => ['last', CEVT_LPWSTR],
	        ::CE_EMAIL => ['mail@aaa.bbb', CEVT_LPWSTR]
	       }) || die $rapi->geterror;

  $rapi->CloseDB() ||
    die $rapi->geterror;

  # Read out some registry stuff.
  $rapi=new Rapi2::Simple(key => [HKEY_CURRENT_USER, 'ControlPanel\Desktop']);
  die $rapi->geterror if $rapi->errorstate;
  printf "type=%d, data=%s\n", $rapi->KeyValue("wallpaper");

=head1 DESCRIPTION

B<Rapi2::Simple> is a class providing simple access to Pocket PCs
using the Rapi2 librapi2 wrapper.

=head1 CONSTRUCTOR

=over

=item new ([OPTIONS])

Create a new Rapi2::Simple object.

B<OPTIONS>

C<log_level =E<gt> LOG_LEVEL> - LOG_LEVEL is the log level set by the
synce_log_set_level function.

Available levels are:

  0 - No logging (default)
  1 - Errors only
  2 - Errors and warnings
  3 - Everything

C<db =E<gt> NAME | CEOID> - If you want to to create a database
object, you can specify the database you want to open either by the
NAME or by the CEOID.

C<file =E<gt> EXPR | [MODE,NAME]> - If you want to create a file
object, you can specify the file and the opening mode by an expression
EXPR or the MODE and the NAME of the file. See I<OpenFile> for
details.

C<key =E<gt> [HKEY, SUBKEY]> - If you want to create a registry
object, you can specify the registry key you want to open. See the
I<OpenKey> method for details.

You should check the error state after creating an object.

=back

=head1 METHODS

All methods set the error string if they fail.

=over

=item errorstate ()

Return the objects error state. It is I<TRUE> if the error string is
defined, which means, that an error occured and I<FALSE> if the object
is not in error state.

=item geterror ()

Return the error string and set it to I<undef>.

=back

B<Database access methods>

=over

=item OpenDB (NAME | CEOID)

Open a database either by NAME or by the given CEOID. On success
I<TRUE> will be returned, I<FALSE> otherwise.

=item CloseDB ()

Close an opened database. On success I<TRUE> will be returned,
I<FALSE> otherwise.

=item fetch ()

Get the next record from an opened database. A hash reference will be
returned. The propids of the properties of a record are the keys. All
values are array references to arrays that hold the properties value
in the first field and the type in the second.

If this method will be called after the last record was returned, it
returns I<FALSE>. On an error it set the error string and returns
I<FALSE> too. So check the error state.

=item store (DATA [, CEOID])

Write a record to an opened database. DATA is a hash reference, that
is from the same structure as the references you get from L<fetch
()>. The keys are the propids of the properties you want to write, and
the values are array references. These arrays hold the properties
value in the first field and the type in the second.

If you specify the CEOID of a record, the properties will be written
to it, otherwise a new record will be created.

On success I<TRUE> will be returned, I<FALSE> otherwise.

=item DBList ()

Get a list of all databases. An array containing hash references, will
be returned on success, I<FALSE> otherwise.

Every hash has the following keys: (CEDB_FIND_DATA structure)

  OidDb
  DbInfo (CEDBASEINFO structure)
    dwFlags
    szDbaseName
    dwDbaseType
    wNumRecords
    wNumSortOrder
    dwSize
    ftLastModified
    rgSortSpecs (SORTORDERSPEC structure)
      propid
      dwFlags

The best way to find out what the keys mean, is to contact the MSDN.

=back

B<File access methods>

=over

=item OpenFile (EXPR | MODE,NAME)

Open a file. The mode (reading or writing) is specified in the kind of
Perls normal open function. This means you can either specify an
expression EXPR including both, the mode of opening and the filename,
or you specify the MODE and NAME as separate arguments.

If MODE is '<' or nothing, the file will be opened for reading. If
MODE is '>', the file will be opened for writing.

On success I<TRUE> will be returned, I<FALSE> otherwise.

=item CloseFile ()

Close an opened file. On success I<TRUE> will be returned, I<FALSE>
otherwise.

=item ReadFile ([NUM])

Read NUM bytes from an opened file. If NUM is not specified, 1024 will
be used. On success a scalar containing the read bytes will be
returned, I<undef> otherwise.

=item WriteFile (DATA)

Write DATA to an opened file. DATA is a scalar. On success I<TRUE> is
returned, I<FALSE> otherwise.

=item CopyFile (FILE1, FILE2)

Copy FILE1 to FILE2. FILE1 and FILE2 are strings, that specify the
file names. On success I<TRUE> will be returned, I<FALSE> otherwise.

=item MoveFile (FILE1, FILE2)

Move FILE1 to FILE2. FILE1 and FILE2 are strings, that specify the
file names. On success I<TRUE> will be returned, I<FALSE> otherwise.

=item DeleteFile (FILE)

Delete the file FILE. FILE is a string, that specifies the file name.
On success I<TRUE> will be returned, I<FALSE> otherwise.

=item DeleteDir (DIR)

Delete the directory DIR. DIR is a string, that specifies the
directory name. On success I<TRUE> will be returned, I<FALSE>
otherwise.

=item FileList (PATH [, FLAGS])

Get a list of all files in PATH. An array containing hash references,
will be returned on success, I<FALSE> otherwise.

Every hash has the following keys:

  dwOID
  dwFileAttributes
  nFileSizeHigh
  nFileSizeLow
  dwCreationTime
  dwLastAccessTime
  dwLastWriteTime
  cFileName

The best way to find out what the keys mean, is to contact the
MSDN. The hash keys are the members of the CE_FIND_DATA structure.

--- START - from the MSDN ---

FLAGS - A combination of filter and retrieval flags. The filter flags
specify what kinds of files to document, and the retrieval flags
specify which members of the CE_FIND_DATA structure to retrieve.

The I<filter flags> can be a combination of the following values:

B<FAF_ATTRIB_CHILDREN>: Only retrieve information for directories
which have child items.

B<FAF_ATTRIB_NO_HIDDEN>: Do not retrieve information for files or
directories which have the hidden attribute set.

B<FAF_FOLDERS_ONLY>: Only retrieve information for directories.

B<FAF_NO_HIDDEN_SYS_ROMMODULES>: Do not retrieve information for ROM
files or directories.

The I<retrieval flags> can be a combination of the following values:

B<FAF_ATTRIBUTES>: Retrieve the file attributes and copy them to the
I<dwFileAttributes> member.

B<FAF_CREATION_TIME>: Retrieve the file creation time and copy it to
the I<ftCreationTime> member.

B<FAF_LASTACCESS_TIME>: Retrieve the time when the file was last
accessed and copy it to the I<ftLastAccessTime> member.

B<FAF_LASTWRITE_TIME>: Retrieve the time when the file was last
written to and copy it to the I<ftLastWriteTime> member.

B<FAF_SIZE_HIGH>: Retrieve the high-order DWORD value of the file size
and copy it to the I<nFileSizeHigh> member.

B<FAF_SIZE_LOW>: Retrieve the low-order DWORD value of the file size
and copy it to the I<nFileSizeLow> member.

B<FAF_OID>: Retrieve the object identifier of the file and copy it to
the I<dwOID> member.

B<FAF_NAME>: Retrieve the file name and copy it to the I<cFileName>
member.

--- END - from the MSDN ---

If you do not specify FLAGS, the following combination will be use:

  FAF_ATTRIBUTES | FAF_LASTWRITE_TIME | FAF_SIZE_LOW | FAF_OID | FAF_NAME

=item GetSpecialFolderPath (NUM)

The the path of a special folder specified by NUM. This parameter can be
one of the following values: (from the MSDN)

C<CSIDL_BITBUCKET>: Recycle bin - file system directory containing
file objects in the user's recycle bin. The location of this directory
is not in the registry; it is marked with the hidden and system
attributes to prevent the user from moving or deleting it.

C<CSIDL_COMMON_DESKTOP>: File system directory that contains files and
folders that appear on the desktop for all users.

C<CSIDL_COMMON_PROGRAMS>: File system directory that contains the
directories for the common program groups that appear on the Start
menu for all users.

C<CSIDL_COMMON_STARTMENU>: File system directory that contains the
programs and folders that appear on the Start menu for all users.

C<CSIDL_COMMON_STARTUP>: File system directory that contains the
programs that appear in the Startup folder for all users. The system
starts these programs whenever any user logs on to a Windows desktop
platform.

C<CSIDL_CONTROLS>: Control Panel - virtual folder containing icons for
the control panel applications.

C<CSIDL_DESKTOP>: Windows desktop - virtual folder at the root of the
name space.

C<CSIDL_DESKTOPDIRECTORY>: File system directory used to physically
store file objects on the desktop - not to be confused with the
desktop folder itself.

C<CSIDL_DRIVES>: My Computer - virtual folder containing everything on
the local computer: storage devices, printers, and Control Panel.  The
folder can also contain mapped network drives.

C<CSIDL_FONTS>: Virtual folder containing fonts.

C<CSIDL_NETHOOD>: File system directory containing objects that appear
in the network neighborhood.

C<CSIDL_NETWORK>: Network Neighborhood - virtual folder representing
the top level of the network hierarchy.

C<CSIDL_PERSONAL>: File system directory that serves as a common
repository for documents.

C<CSIDL_PRINTERS>: Printers folder - virtual folder containing
installed printers.

C<CSIDL_PROGRAMS>: File system directory that contains the user's
program groups which are also file system directories.

C<CSIDL_RECENT>: File system directory containing the user's most
recently used documents.

C<CSIDL_SENDTO>: File system directory containing Send To menu items.

C<CSIDL_STARTMENU>: File system directory containing Start menu items.

C<CSIDL_STARTUP>: File system directory that corresponds to the user's
Startup program group.

C<CSIDL_TEMPLATES>: File system directory that serves as a common
repository for document templates.

On success a scalar containing the the path will be returned, I<undef>
otherwise.

=back

B<Registry access methods>

=over

=item OpenKey ([HKEY,] SUBKEY)

Open a registry key or open a subkey of an already opened registry
key.

HKEY is a handle to a currently open key or any of the following
predefined reserved handle values:

  HKEY_CLASSES_ROOT
  HKEY_CURRENT_CONFIG
  HKEY_CURRENT_USER
  HKEY_LOCAL_MACHINE

SUBKEY is a string containing the name of the subkey to open. If this
parameter empty, the key identified by the HKEY parameter will be
opened.

If only one parameter is given, the I<OpenSubKey> method will be
called.

On success I<TRUE> will be returned, I<FALSE> otherwise.

=item OpenSubKey (SUBKEY)

Open a subkey of a currently open key. SUBKEY is a string containing
the name of the subkey to open. On success I<TRUE> will be returned,
I<FALSE> otherwise.

=item CloseKey ()

Close a currently open key. On success I<TRUE> will be returned, I<FALSE>
otherwise.

=item GetSubKeyNames ()

Get the names of all subkeys of a currently open key.

An array containing all names will be returned on success, I<FALSE>
otherwise.

=item GetValueNames ()

Get the names of all values of a currently open key.

An array containing all names will be returned on success, I<FALSE>
otherwise.

=item KeyValue (NAME [, TYPE, DATA])

Set or get the TYPE and the DATA of a specified value NAME of a
currently open key. The value will be changed, if you specify the TYPE
and the DATA aguments, and the method returns I<TRUE> on success. If
you only sepcify the NAME of the value, an array, that holds the TYPE
in the first field and the DATA in the second field, will be returned.

If the method fails, it returns I<FALSE>.

=item GetInfoKey ()

Get information about the currently open registry key. The following
array will be returned:

($cSubKeys, $cbMaxSubKeyLen, $cbMaxClassLen, $cValues,
$cbMaxValueNameLen, $cbMaxValueLen, $cbSecurityDescriptor,
$ftLastWriteTime)

  $cSubKeys             - the number of subkeys
  $cbMaxSubKeyLen       - the length of the longest subkey name
  $cbMaxClassLen        - the length of the longest subkey class
                          string
  $cValues              - the number of values
  $cbMaxValueNameLen    - the length of the longest value name
  $cbMaxValueLen        - the length, in bytes, of the longest data
                          component
  $cbSecurityDescriptor - the length, in bytes, of the key's
                          security descriptor
  $ftLastWriteTime      - the last time that the key or any of its
                          value entries was modified

If the method fails, I<FALSE> will be returned.

=back

=head1 TODO

Translate this documentation to 'good' english. ;)

=head1 SEE ALSO

L<Rapi2>

L<synce(1)>

=head1 AUTHOR

Andreas Pohl (osar@users.sourceforge.net)

=cut

