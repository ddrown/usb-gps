package USBGPS::ClockSample::MidSecond;

use Moose;
use USBGPS::Timestamp;
extends "USBGPS::ClockSample";

has "pps_data" => ( # last PPS sample
    is => "ro",
    isa => "USBGPS::ClockSample::PPS"
    );

has "time_now" => ( # counter at USB SOF, cycles
    is => "ro",
    isa => "Int"
    );

has "usb_time" => ( # measured transfer time, cycles
    is => "ro",
    isa => "Int"
    );

has "now" => ( # PC time in [sec, usec] format
    is => "ro",
    isa => "ArrayRef[Int]"
    );

has "GPSState" => ( # NMEA data
    is => "ro",
    isa => "USBGPS::GPSState"
    );

sub _build_local_time {
  my($self) = @_;

  return USBGPS::Timestamp->new(
      seconds => $self->now->[0],
      micro_seconds => $self->now->[1]
      );
}

sub _build_remote_time {
  my($self) = @_;

  my $retval;
  eval {
    my $counter_hz = $self->pps_data->st_interval_estimate;

    my $remote_sec = $self->GPSState->counter_adjust($self->time_now, $counter_hz);
    if(not defined($remote_sec)) {
      return undef; # GPSState rejected the sample
    }
    my $hz_past_0us = $self->time_now - $self->pps_data->time_pps + $self->usb_time;
    if($self->time_now < $self->pps_data->time_pps) { # counter wrap
      $hz_past_0us += 2**32;
    }
    my $remote_usec = int($hz_past_0us/$counter_hz*1000000);
    if($remote_usec > 1000000) {
      print "partial counter misfire\n";
      return undef;
    }

    $retval = USBGPS::Timestamp->new(
      seconds => $remote_sec,
      micro_seconds => $remote_usec
      );
  };
  return $retval;
}

sub valid {
  my($self) = @_;
  return defined($self->remote_time);
}

1;
