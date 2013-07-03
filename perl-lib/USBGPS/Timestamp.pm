package USBGPS::Timestamp;

use Moose;

has "seconds" => (
    is => "ro",
    isa => "Int"
    );

has "micro_seconds" => (
    is => "ro",
    isa => "Int"
    );

has "float" => (
    is => "ro",
    isa => "Num",
    lazy => 1,
    builder => "_build_float"
    );

sub _build_float {
  my($self) = @_;

  return $self->seconds + $self->micro_seconds / 1000000;
}

1;
