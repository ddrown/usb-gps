package USBGPS::GPSState;

use Moose;
use Time::Local qw(timegm);

has second => (
    is => "ro",
    isa => "Int"
    );
has minute => (
    is => "ro",
    isa => "Int"
    );
has hour => (
    is => "ro",
    isa => "Int"
    );
has day => (
    is => "ro",
    isa => "Int"
    );
has month => (
    is => "ro",
    isa => "Int"
    );
has year => (
    is => "ro",
    isa => "Int"
    );

has "fixtype" => (
    is => "ro",
    isa => "Int" # see below for string names
    );

has "fixsatcount" => (
    is => "ro",
    isa => "Int" # how many sats are used in the fix?
    );

has "satcount" => (
    is => "ro",
    isa => "Int" # how many sats are overhead?
    );

has "avgsnr" => (
    is => "ro",
    isa => "Int" # average snr for sats used in fix
    );

has "valid" => (
    is => "ro",
    isa => "Int" # 0/1 for data valid (has fix)
    );

has "time_timestamp" => (
    is => "ro",
    isa => "Int", # in STM32F4's timer cycles
    );

has "epoch" => ( # unix epoch timestamp
    is => "ro",
    lazy => 1,
    builder => "_build_epoch"
    );

has "lock" => ( # boolean: is locked
    is => "ro",
    lazy => 1,
    builder => "_build_lock"
    );

sub _build_epoch {
  my($self) = @_;
  if(not defined $self->month) {
    return undef;
  }
  return timegm($self->second,$self->minute,$self->hour,$self->day,$self->month - 1,$self->year);
}

sub _build_lock {
  my($self) = @_;
  return $self->fixtype > 1;
}

sub fixtype_name {
  my($self,$type) = @_;

  my(@fixtypes) = ("??","no fix","2D fix","3D fix");
  if(defined($type)) {
    return $fixtypes[$type];
  } elsif(defined($self->fixtype)) {
    return $fixtypes[$self->fixtype];
  } else {
    return $fixtypes[0]; # unknown
  }
}

sub satinfo {
  my($self) = @_;

  return sprintf("%d, %d",$self->satcount, $self->avgsnr);
}

sub logfile_string {
  my($self) = @_;

  return sprintf("%d %d %d %d %d",$self->fixsatcount, $self->satcount, $self->avgsnr, $self->fixtype, $self->valid);
}

sub counter_adjust { # What's the time now (in seconds), given the current counter and the counter HZ
  my($self, $counter_now, $counter_cycles_per_second) = @_;

  my $hz_ago = $counter_now - $self->time_timestamp;
  if($hz_ago < 0) { # counter wrap
    $hz_ago += 2**32;
  }
  my $ms_ago = $hz_ago/$counter_cycles_per_second*1000;
  $ms_ago += 700; # serial time is ~700ms off

  my $remote_sec = $self->epoch;
  if($ms_ago > 2700) {
    print "skip (gps time longer than 2s ago=$ms_ago ms)\n";
    return undef;
  } elsif($ms_ago > 1700) {
    $remote_sec += 2;
  } elsif($ms_ago > 700) {
    $remote_sec++;
  }
 
  return $remote_sec;
}

1;
