package USBGPS::Filter::Percentile;

use Moose;
has "filter_window" => (
    is => "rw",
    isa => "Num", # seconds
    default => 0.001
    );

has moving_avg => (
    is => "rw",
    isa => "Num", # seconds
    default => 0
    );

has "sample_history" => (
    is => "ro",
    isa => "ArrayRef[Num]", # percent
    default => sub { [] }
    );

sub update_filter {
  my($self,$percent_valid,$chosen_offset,$samples) = @_;

  push(@{ $self->sample_history }, $samples);
  if(@{ $self->sample_history } > 12) {
    shift(@{ $self->sample_history });
  }
  if($percent_valid == 0) { # no samples matched our filter, reset moving avg
    $self->moving_avg($chosen_offset);
  } else {
    $self->moving_avg($self->moving_avg * .5 + $chosen_offset * .5); # weighted moving average
  }

  my(@allsamples);
  foreach my $sampleset (@{ $self->sample_history }) {
    foreach my $sample (@$sampleset) {
      push(@allsamples, abs($self->moving_avg - $sample->offset));
    }
  }
  @allsamples = sort { $a <=> $b } @allsamples;
  my $n80percentile = $allsamples[int(@allsamples * .8)];
  if($n80percentile < 0.000002) { # 2us is the min
    $n80percentile = 0.000002;
  }
  if($n80percentile > 0.001) { # 1ms is the max
    $n80percentile = 0.001;
  }
  $self->filter_window($n80percentile);
}

sub filter {
  my($self,$samples) = @_;

  my $this_filter_window = $self->filter_window; # keep it to return it
  my(@valid);
  for(my $i = 0; $i < @$samples; $i++) {
    if(abs($self->moving_avg - $samples->[$i]->offset) < $self->filter_window) {
      push(@valid, $samples->[$i]);
    }
  }
  if(not @valid) {
    print STDERR "reset moving avg at ".time()."\n";
    @valid = sort { $a->offset <=> $b->offset } @$samples;
    $self->update_filter(0,$valid[int(@valid/2)]->offset,$samples);
    return $self->filter($samples); # there's now at least 1 sample within 10us of moving_avg
  }

  @valid = sort { $a->offset <=> $b->offset } @valid;
  my $message = $valid[int(@valid/2)];
  my $valid_pct = @valid/@$samples*100;
  $self->update_filter($valid_pct,$message->offset,$samples);

  return ($message, sprintf("%d %0.6f %0.6f",$valid_pct,$this_filter_window,$self->moving_avg));
}

1;
