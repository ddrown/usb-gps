package USBGPS::NTPMessage;

use Moose;
use USBGPS::Timestamp;

has "remote_time" => (
    is => "ro",
    isa => "USBGPS::Timestamp"
    );

has "local_time" => (
    is => "ro",
    isa => "USBGPS::Timestamp"
    );

has "message" => (
    is => "ro",
    lazy => 1,
    builder => "_build_message"
    );

sub _build_message {
  my($self) = @_;

  my $nsamples = 0;
  my $valid = 1;
  my $precision = -16; # 2^-16 = 15 microseconds
  my $leap = 0;
  my $count = 0;
  my $mode = 0;

  my $format = "ll". "ql" . "l" . "ql" . "llll" . "l" . "llllllllll";
  my $message = pack( $format, 
		  $mode, $count,
                  $self->remote_time->seconds, $self->remote_time->micro_seconds,
		  0,
                  $self->local_time->seconds, $self->local_time->micro_seconds,
                  $leap,
		  $precision, $nsamples, $valid,
                  0,0,0,0,0,0,0,0,0,0,0);
  return $message;
}

sub send_to_ntp {
  my($self,$shmid) = @_;

  my $len = length($self->message);
  die "wrong message length" unless($len == 96);
  shmwrite($shmid, $self->message, 0, $len) || die("$!");
}

1;
