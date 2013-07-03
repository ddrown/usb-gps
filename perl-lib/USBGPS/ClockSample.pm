package USBGPS::ClockSample;

use Moose;
use USBGPS::Timestamp;
use USBGPS::NTPMessage;

has "remote_time" => (
    is => "ro",
    isa => "USBGPS::Timestamp",
    lazy => 1,
    builder => "_build_remote_time"
    );

has "local_time" => (
    is => "ro",
    isa => "USBGPS::Timestamp",
    lazy => 1,
    builder => "_build_local_time"
    );

has "ntp_message" => (
    is => "ro",
    isa => "USBGPS::NTPMessage",
    lazy => 1,
    builder => "_build_ntp_message"
    );

has "offset" => (
    is => "ro",
    lazy => 1,
    builder => "_build_offset"
    );

sub _build_ntp_message {
  my($self) = @_;

  return USBGPS::NTPMessage->new(
      local_time => $self->local_time,
      remote_time => $self->remote_time
      );
}

sub _build_local_time {
  my($self) = @_;

  die("do this in the subclass");
}

sub _build_remote_time {
  my($self) = @_;

  die("do this in the subclass");
}

sub _build_offset {
  my($self) = @_;

  return $self->remote_time->float - $self->local_time->float;
}

1;
