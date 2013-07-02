package USBGPS::Filter;

use Moose;
has "filter_window" => (
    is => "rw",
    isa => "Num", # seconds
    default => 0.001
    );

has running_avg => (
    is => "rw",
    isa => "Num", # seconds
    default => 0
    );

has "filter_performance_history" => (
    is => "ro",
    isa => "ArrayRef[Num]", # percent
    default => sub { [] }
    );

sub update_filter {
  my($self,$percent_valid,$offset) = @_;

  push(@{ $self->filter_performance_history },$percent_valid);
  if(@{ $self->filter_performance_history } > 5) { # keep a history of 5 samples of 16 seconds each
    shift(@{ $self->filter_performance_history });
  }
  if($percent_valid == 0) { # no samples matched our filter
    $self->running_avg($offset);
    if($self->filter_window < 0.001) { # assuming the window is at most 1ms 
      $self->filter_window($self->filter_window * 2);
    }
  } else {
    $self->running_avg($self->running_avg * .5 + $offset * .5); # weighted average
    my($min,$avg,$max);
    foreach my $percent (@{ $self->filter_performance_history }) {
      if(not defined($min) or $min > $percent) {
        $min = $percent;
      }
      if(not defined($max) or $max < $percent) {
        $max = $percent;
      }
      $avg += $percent;
    }
    $avg = $avg / @{ $self->filter_performance_history };
    if($avg > 90 and $self->filter_window > 0.0001) { # at least 90% samples within window and the window is larger than 100us
      $self->filter_window($self->filter_window / 2); # cut in half
    } elsif($avg > 70 and $self->filter_window > 0.00002) { # at least 70% samples within window and the window is larger than 10us
      $self->filter_window($self->filter_window * 0.9); # lower 10%
    } elsif($avg < 45 and $self->filter_window < 0.001) { # at most 45% samples within window and the window is smaller than 1ms
      $self->filter_window($self->filter_window * 1.1); # raise 10%
    }
  }
}

sub filter {
  my($self,$samples) = @_;

  my $this_filter_window = $self->filter_window; # keep it to return it
  my(@valid);
  for(my $i = 0; $i < @$samples; $i++) {
    if(abs($self->running_avg - $samples->[$i]{offset}) < $self->filter_window) {
      push(@valid, $samples->[$i]);
    }
  }
  if(not @valid) {
    print STDERR "reset running avg at ".time()."\n";
    @valid = sort { $a->{offset} <=> $b->{offset} } @$samples;
    $self->update_filter(0,$valid[int(@valid/2)]->{offset});
    return $self->filter($samples); # there's now at least 1 sample within 10us of running_avg
  }

  @valid = sort { $a->{offset} <=> $b->{offset} } @valid;
  my $message = $valid[int(@valid/2)];
  my $valid_pct = @valid/@$samples*100;
  $self->update_filter($valid_pct,$message->{offset});

  return ($message, sprintf("%d %0.6f %0.6f",$valid_pct,$this_filter_window,$self->running_avg));
}

1;
