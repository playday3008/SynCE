use strict;
use RRA;

$|++;

print "version of rra: $RRA::VERSION\n";

my $rra=rra_new();
die "rra_connect failed" unless rra_connect($rra);

# rra_get_object_ids
#my $ids=rra_get_object_ids($rra, 0x2717);
#die "rra_get_object_ids failed" unless defined $ids;

#print "uc=".$ids->FETCH("unchanged")." c=".$ids->FETCH("changed")."\n";
#foreach (@{$ids->FETCH("ids")}){ printf "0x%x\n", $_ }

#my $del=rra_get_deleted_object_ids($rra, 0x2711, $ids);
#die "rra_get_deleted_object_ids failed" unless defined $del;
#
#print "deleted:\n";
#foreach (@$del) { printf "0x%x\n", $_ }

# rra_get_object_types
#my $types=rra_get_object_types($rra);
#die "rra_get_object_types failed" unless defined $types;
#print "id         count    size     date".(" "x16)."name\n";
#foreach (@$types)
#{
#  printf "0x%08x %8d %8d %s %s\n", $_->FETCH("id"), $_->FETCH("count"), $_->FETCH("total_size"),
#    gettime($_->FETCH("modified")), $_->FETCH("name");
#}

# rra_object_get/put (put in an existing object_id)
#my ($data, $len)=rra_object_get($rra, 0x2717, 0x16a8);
#die "rra_object_get failed" unless defined $data;

## convert tests
#my $vc=rra_contact_to_vcard(RRA_CONTACT_ID_UNKNOWN, $data, $len, RRA_CONTACT_VERSION_3_0);
#die "rra_contact_to_vcard failed" unless defined $vc;
#print $vc;

# rra_object_get/put (put in an existing object_id)
print "rra_object_get($rra, 0x2717, 0xa002e9d);\n";
my ($data, $len)=rra_object_get($rra, 0x2717, 0xa002e9d);
die "rra_object_get failed" unless defined $data;
printf "len=%d\n", $len;
print "---\n$data\n----\n";

my $tzi=rra_get_time_zone_information($rra);
print "tzi=".$tzi->{Bias}."\n";

my $ve=rra_appointment_to_vevent(0, $data, $len, 0, $tzi);
die "rra_appointment_to_vevent failed" unless defined $ve;
print $ve."\n";

my ($id, $ap, $aplen)=rra_appointment_from_vevent($ve, 0, $tzi);
die "rra_appointment_to_vevent failed" unless defined $ap;
print "aplen=$aplen\n";
print "---\n$ap\n----\n";

#my $oid=rra_object_put($rra, 0x2712, 0x0, 0, $data, $len);
#die "rra_object_put failed" unless defined $oid;
#printf "0x%x\n", $oid;

# partner tests
#my $partner=rra_partner_get_current($rra);
#die "rra_partner_get_current failed" unless defined $partner;
#printf "current partner: %d\n", $partner;
#
#my $p_id=rra_partner_get_id($rra, $partner);
#die "rra_partner_get_id failed" unless defined $p_id;
#printf "id: 0x%x\n", $p_id;
#
#my $name=rra_partner_get_name($rra, $partner);
#die "rra_partner_get_name failed" unless defined $name;
#print "name: $name\n";


rra_disconnect($rra);


sub gettime
{
  my $t=shift;
  return " "x19 unless $t;
  my ($sec, $min, $hour, $day, $mon, $year)=localtime($t);
  return sprintf("%04d-%02d-%02d %02d:%02d:%02d", 1900+$year, $mon+1, $day, $hour, $min, $sec);
}
