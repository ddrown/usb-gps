package USBGPS::ClockSample::PPS;

use Moose;
use USBGPS::Timestamp;
extends "USBGPS::ClockSample";

has "time_pps" => ( # counter at PPS signal, cycles
    is => "ro",
    isa => "Int"
    );

has "time_now" => ( # counter at USB SOF, cycles
    is => "ro",
    isa => "Int"
    );

has "st_interval" => ( # measured counter interval, HZ
    is => "ro",
    isa => "Int"
    );

has "st_interval_estimate" => ( # estimate of the true counter interval, HZ
    is => "ro",
    isa => "Num"
    );

has "GPSState" => ( # NMEA data
    is => "ro",
    isa => "USBGPS::GPSState"
    );

has "usb_time" => ( # measured transfer time, cycles
    is => "ro",
    isa => "Int"
    );

has "usb_polls" => ( # number of SOF frames since last PPS, count
    is => "ro",
    isa => "Int"
    );

has "now" => ( # PC time in [sec, usec] format
    is => "ro",
    isa => "ArrayRef[Int]"
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

  my $counter_hz = $self->st_interval_estimate;

  my $remote_sec = $self->GPSState->counter_adjust($self->time_now, $counter_hz);
  if(not defined($remote_sec)) {
    return undef; # GPSState rejected the sample
  }
  my $hz_past_0us = $self->time_now - $self->time_pps + $self->usb_time;
  if($self->time_now < $self->time_pps) { # counter wrap
    $hz_past_0us += 2**32;
  }
  my $remote_usec = int($hz_past_0us/$counter_hz*1000000);
  if($remote_usec > 1000000) {
    print "partial counter misfire\n";
    return undef;
  }

  return USBGPS::Timestamp->new(
      seconds => $remote_sec,
      micro_seconds => $remote_usec
      );
}

sub valid {
  my($self) = @_;

  if($self->usb_polls < 499 or $self->usb_polls > 500) {
    print "USB SOF polls between PPS: ". $self->usb_polls ."\n";
  }
  if($self->st_interval > 10501000 or $self->st_interval < 10499999) {
    print "skip (remote timer=". $self->st_interval ." HZ)\n";
    return undef; # skip PPS pulses that happen outside of a 500ppm window
  }
  return 1;
}

1;
