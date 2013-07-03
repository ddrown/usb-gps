package USBGPS::SerialData;

use Moose;
use USBGPS::GPSState;

has "state" => (
    is => "rw",
    isa => "USBGPS::GPSState",
    default => sub { USBGPS::GPSState->new() }
    );

has "gps_line" => (
    is => "rw",
    isa => "Str"
    );


sub parse {
  my($self,$now,$line,$interval) = @_;

  # G 3,6,8,27,3,38,32,2,6,13,1,3764701747
  if($line =~ /^G (\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)/) {
    my($fixtype,$fixsatcount,$satcount,$avgsnr,$hour,$minute,$second,$day,$month,$year,$valid,$timestamp) = ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12);

    my $last_gps_state = $self->state;
    $self->state(USBGPS::GPSState->new(
          second => $second,
          minute => $minute,
          hour => $hour,
          day => $day,
          month => $month,
          year => $year,
          fixtype => $fixtype,
          fixsatcount => $fixsatcount,
          satcount => $satcount,
          avgsnr => $avgsnr,
          valid => $valid,
          time_timestamp => $timestamp
          ));

    if(defined($last_gps_state->epoch) and $last_gps_state->epoch == $self->state->epoch) {
      print "GPS time matches last gps time(". $self->state->epoch .") HZnow=",$timestamp," HZthen=",$last_gps_state->time_timestamp,"\n";
      return; # throw away repeat timestamps, TODO: debug this on the microcontroller
    }

    chomp($line);
    $self->gps_line($line);

    if($self->state->epoch != $now->[0]) {
      print "GPS Time Off: ". $self->state->epoch ." != ".$now->[0]."/". $self->gps_line ."\n";
    }
    if(not defined($last_gps_state->fixtype) or $last_gps_state->fixtype != $self->state->fixtype) {
      print "GPS lock from ".$last_gps_state->fixtype_name()." to ".$self->state->fixtype_name()."\n";
    }
    if($self->state->fixtype == 1) {
      print "sats: ".$self->state->satinfo()."\n";
    }
  }
}

1;
