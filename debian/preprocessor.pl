#!/usr/bin/perl

# Copyright (C) 2021 X2Go Project - https://wiki.x2go.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

# Disable some Perl::Critic violations.
#
# I like parentheses.
## no critic (ProhibitParensWithBuiltins)
#
# It's recommended not to use the constant pragma, but we don't want to use
# non-core modules like Readonly.
## no critic (ValuesAndExpressions::ProhibitConstantPragma)

use strict;
use warnings;

# Enable full Unicode handling, if possible.
use 5.012;

# Enable UTF-8 encoded data in the script itself.
use utf8;

# Make text coding issues fatal.
use warnings  qw (FATAL utf8);

#use X2Go::Utils qw (is_int);
use English qw (-no_match_vars);
use Getopt::Long qw (GetOptionsFromArray);
use Pod::Usage;
use Storable qw (dclone);
use Data::Dumper qw (Dumper);
use Encode qw (encode decode);
use Encode::Locale;
use File::Temp qw (tempfile);
use Scalar::Util qw (openhandle looks_like_number);
use IO::Handle;
use File::Copy qw (copy);

# Set up automatic encoding.
if (-t STDIN) {
  binmode STDIN,  ":encoding(console_in)";
}
if (-t STDOUT) {
  binmode STDOUT, ":encoding(console_out)";
}
if (-t STDERR) {
  binmode STDERR, ":encoding(console_out)";
}

# Convert data in ARGV.
exit (Main (map { Encode::decode (locale => $_, (Encode::FB_CROAK | Encode::LEAVE_SRC)); } @ARGV));

BEGIN {
}

# No code past this point should be getting executed!
#
# These are actually supposed to be enums, but since Perl doesn't have a
# proper way of creating enums (at least not natively), we'll emulate that
# using list constants.
#
# Make sure that each token value is unique and referencing the correct
# element index in the TOKENS array.
use constant TOKEN_IF => 0;
use constant TOKEN_EL => 1;
use constant TOKEN_FI => 2;
use constant TOKEN_EQ => 3;
use constant TOKEN_NE => 4;
use constant TOKEN_INVALID => 5;
use constant TOKENS => ( q{@?}, q{@!}, q{@/}, q{==}, q{!=}, q{} );

# Helper function deleting whitespace from the left side of a string.
#
# Takes the string to trim as its only parameter.
#
# Returns the modified string.
sub ltrim {
  my $str = shift;

  $str =~ s/^\s+//;

  return $str;
}

# Helper function deleting whitespace from the right side of a string.
#
# Takes the string to trim as its only parameter.
#
# Returns the modified string.
sub rtrim {
  my $str = shift;

  $str =~ s/\s+$//;

  return $str;
}

# Helper function deleting whitespace from both sides of a string.
#
# Takes the string to trim as its only parameter.
#
# Returns the modified string.
sub trim {
  my $str = shift;

  $str = ltrim ($str);
  $str = rtrim ($str);

  return $str;
}

# Helper function extracting a token from the given string.
#
# Takes a string as its only parameter.
#
# Returns the deitected token as an offset/index in the tokens array.
#
# On error, returns undef.
sub fetch_token {
  my $ret = TOKEN_INVALID;

  my $input = shift;

  if (!(defined ($input))) {
    print {*STDERR} "No input given to token fetching function, erroring out.\n";
    $ret = undef;
  }

  if (defined ($ret)) {
    ## no critic (ControlStructures::ProhibitCStyleForLoops)
    for (my $i = 0; $i < scalar ((TOKENS)); ++$i) {
    ## critic (ControlStructures::ProhibitCStyleForLoops)
      # Luckily, tokens are non-ambiguous.
      my $token_length = length ((TOKENS)[$i]);
      if ((TOKENS)[$i] eq substr ($input, 0, $token_length)) {
        $ret = $i;
        last;
      }
    }
  }

  return $ret;
}

# Helper function parsing a condition and turning it into a boolean value.
#
# Takes the (probable) condition as a string, and a boolean value denoting
# whether debugging is requested or not as its parameters.
#
# Returns a boolean value .
#
# On error, returns undef.
sub parse_condition {
  my $ret = 0;
  my $error_detected = 0;

  my $condition = shift;
  my $debug = shift;

  if (!(defined ($condition))) {
    print {*STDERR} "No condition passed to condition parsing helper, erroring out.\n";
    $error_detected = 1;
  }

  if (!($error_detected)) {
    if (!(defined ($debug))) {
      print {*STDERR} "No debugging argument passed to parsing helper, erroring out.\n";
      $error_detected = 1;
    }
  }

  if (!($error_detected)) {
    if (!(length ($condition))) {
      print {*STDERR} "Empty condition is invalid, erroring out.\n";
      $error_detected = 1;
    }
  }

  my $lhs = undef;
  my $rhs = undef;
  my $token = undef;
  if (!($error_detected)) {
    # First, extract lhs and remove it from $condition.
    if ($condition =~ m/^([^\s]+)\s+(.*)$/) {
      $lhs = $1;
      $condition = $2;
    }
    else {
      print {*STDERR} "Unable to extract left hand side from condition, erroring out.\n";
      $error_detected = 1;
    }
  }

  if (!($error_detected)) {
    $token = fetch_token ($condition);

    if (!(defined ($token))) {
      print {*STDERR} ".\n";
      $error_detected = 1;
    }
    else {
      if ((TOKEN_EQ == $token) || (TOKEN_NE == $token)) {
        $condition = substr ($condition, length ((TOKENS)[$token]));
      }
      else {
        print {*STDERR} "Invalid token found in condition, erroring out.\n";
        $error_detected = 1;
      }
    }
  }

  if (!($error_detected)) {
    # Everything left must be the rhs.
    $rhs = ltrim ($condition);

    if (!(length ($rhs))) {
      print {*STDERR} "No right hand side found while parsing the condition, erroring out.\n";
      $error_detected = 1;
    }
  }

  if (!($error_detected)) {
    # Now find out if both sides look like integers, in which case we'll use
    # integer semantics for the condition.
    # Otherwise, we'll use a string comparison.
    my $string_comp = 1;
    if ((Scalar::Util::looks_like_number ($lhs)) && (Scalar::Util::looks_like_number ($rhs))) {
      $string_comp = 0;
    }

    # And, finally, do the actual comparison.
    if ($string_comp) {
      $ret = ($lhs eq $rhs);
    }
    else {
      $ret = ($lhs == $rhs);
    }

    # For non-equal, just invert.
    if (TOKEN_NE == $token) {
      $ret = (!($ret));
    }
  }

  if ($error_detected) {
    $ret = undef;
  }

  return $ret;
}

# Helper function reading a file, parsing each line and writing it to a
# temporary file handle.
#
# Takes a file path, a temporary file handle and a boolean value denoting
# whether debugging is requested or not as its parameters.
#
# Returns a boolean value denoting success (false) or failure (true).
sub parse_file {
  my $error_detected = 0;

  my $infile = shift;
  my $temp_fh = shift;
  my $debug = shift;

  if (!(defined ($infile))) {
    print {*STDERR} "No input file passed to parsing helper, erroring out.\n";
    $error_detected = 1;
  }

  if (!($error_detected)) {
    if (!(defined ($temp_fh))) {
      print {*STDERR} "No file handle passed to parsing helper, erroring out.\n";
      $error_detected = 1;
    }
  }

  if (!($error_detected)) {
    if (!(defined (openhandle ($temp_fh)))) {
      print {*STDERR} "Invalid file handle passed to parsing helper, erroring out.\n";
      $error_detected = 1;
    }
  }

  if (!($error_detected)) {
    if (!(defined ($debug))) {
      print {*STDERR} "No debugging argument passed to parsing helper, erroring out.\n";
      $error_detected = 1;
    }
  }

  my $read_fh = undef;
  if (!($error_detected)) {
    $error_detected = (!(open ($read_fh, q{<}, $infile)));

    if (($error_detected) || (!(defined ($read_fh)))) {
      print {*STDERR} "Unable to open input file \"${infile}\" for reading in parsing helper, erroring out.\n";
    }
  }

  if (!($error_detected)) {
    my @cond_stack = ( );
    my $skip = 0;
    while (!(eof ($read_fh))) {
      my $line = readline ($read_fh);
      my $skip_once = 0;

      if ($debug) {
        print {*STDERR} "Processing line \"${line}\" ... ";
      }

      my $work = ltrim ($line);
      my $token = fetch_token ($work);

      if (!(defined ($token))) {
        print {*STDERR} "Unable to extract token from line \"${line}\", erroring out.\n"
      }
      elsif ($debug) {
        print {*STDERR} q{extracted token: } . $token . "\n";
      }

      # Check if the token is valid.
      if (TOKEN_FI == $token) {
        if (!(scalar (@cond_stack))) {
          print {*STDERR} "Encountered FI (end of condition) token, but no condition block active, erroring out.\n";
          $error_detected = 1;
          last;
        }

        # Otherwise, it's valid. Let's handle it by removing an onion layer.
        if ($debug) {
          print {*STDERR} "Got FI token, removing one layer.\n";
        }

        # If we're already in a skip section, just decrement the counter.
        if (1 < $skip) {
          --$skip;
        }
        else {
          pop (@cond_stack);

          # Condition stack might be empty now, in which case make sure to not
          # skip anything.
          if (scalar (@cond_stack)) {
            $skip = $cond_stack[$#cond_stack];
          }
          else {
            $skip = 0;
          }
        }

        $skip_once = 1;
      }

      if (TOKEN_EL == $token) {
        if (!(scalar (@cond_stack))) {
          print {*STDERR} "Encountered EL (else branch) token, but no condition block active, erroring out.\n";
          $error_detected = 1;
          last;
        }

        # Okay, valid.
        # In case we're in a nested skipping section, do nothing. Otherwise,
        # handle it by reversing the skip value.
        if (1 < $skip) {
          if ($debug) {
            print {*STDERR} "Got EL token, but we're already in a skipping section, ignoring.\n";
          }
        }
        else {
          if ($debug) {
            print {*STDERR} "Got EL token, reversing skip value \"${skip}\".\n";
          }

          $skip = (!($skip));

          # Also update stack value, so that it's is up-to-date if additional
          # nesting comes up.
          $cond_stack[$#cond_stack] = $skip;
        }

        $skip_once = 1;
      }

      if (TOKEN_IF == $token) {
        # IF is always valid. But we'll have to parse it. Unless we're in a
        # skipping section. In that case, don't parse the condition and keep
        # skipping, incrementing the skipping counter.
        if (!($skip)) {
          $work = substr ($work, length ((TOKENS)[$token]));
          $work = ltrim ($work);
          my $cond = parse_condition ($work, $debug);

          if (!(defined ($cond))) {
            print {*STDERR} "Unable to parse conditional into a boolean value, erroring out.\n";
            $error_detected = 1;
            last;
          }

          $skip = (!($cond));
          push (@cond_stack, $skip);
        }
        else {
          ++$skip;
        }

        $skip_once = 1;
      }

      # Any other token, including TOKEN_INVALID, is actually always valid,
      # since it is not a preprocessor token.
      # Sounds weird at first, but those lines will just be passed-through
      # verbatim.
      if ((!($skip)) && (!($skip_once))) {
        if ($debug) {
          print {*STDERR} "Passing-through line.\n";
        }

        print $temp_fh "${line}";
      }
    }

    if (scalar (@cond_stack)) {
      print {*STDERR} "Reached end of file, but condition stack not empty - runaway/unterminated condition, erroring out.\n";
      $error_detected = 1;
    }
  }

  close ($read_fh);

  return $error_detected;
}

# Helper function handling unknown options or ignoring the well-known
# separator. It scans for options until hitting the first non-option entry.
#
# Takes an array reference with unparsed options and a boolean value denoting
# whether the separating "--" pseudo-option should be skipped or not as its
# parameters.
#
# Returns an array reference containing a boolean value denoting whether a
# separating "--" pseudo-option has been found *and* skipping it was requested,
# and the sanitized version of the original array reference.
#
# On error, returns undef.
sub sanitize_program_options {
  my $ret = undef;
  my $error_detected = 0;
  my $found_separator = 0;

  my $args = shift;
  my $skip_separator = shift;

  if ((!(defined ($args))) || ('ARRAY' ne ref ($args))) {
    print {*STDERR} "Invalid argument array reference passed to program sanitization helper, erroring out.\n";
    $error_detected = 1;
  }

  if (!($error_detected)) {
    if (!(defined ($skip_separator))) {
      print {*STDERR} "No skip-separator parameter passed to program sanitization helper, erroring out.\n";
      $error_detected = 1;
    }
  }

  if (!($error_detected)) {
    $args = Storable::dclone ($args);

    ## no critic (ControlStructures::ProhibitCStyleForLoops)
    for (my $cur_arg = shift (@{$args}); defined ($cur_arg); $cur_arg = shift (@{$args})) {
    ## critic (ControlStructures::ProhibitCStyleForLoops)
      if (q{-} eq substr ($cur_arg, 0, 1)) {
        # Looks like an option so far. Let's continue scanning.

        if (1 == length ($cur_arg)) {
          # But isn't a real option. Add back to argument list and stop
          # processing.
          unshift (@{$args}, $cur_arg);
          last;
        }
        elsif ((2 == length ($cur_arg)) && (q{-} eq substr ($cur_arg, 1, 1))) {
          if ($skip_separator) {
            # Found separating "--" pseudo-option, but skipping requested. Only
            # set the boolean value for our return value and make sure that we
            # don't skip another separating pseudo-option if it comes up again
            # right next to this one.
            $found_separator = 1;
            $skip_separator = 0;
          }
          else {
            # Not skipping separating "--" pseudo-option - i.e., we'll treat this
            # as a non-option.
            unshift (@{$args}, $cur_arg);
            last;
          }
        }
        else {
          # Otherwise this is an actual option.
          # We either want to error out, if no previous separating "--"
          # pseudo-option was found, or ignore it.
          # The weird 0 + (...) construct here is forcing an arithmetic
          # context. Otherwise, the interpreter might use a string context,
          # in which the value "0" is dualvar'd to both an arithmetic 0 and
          # an empty string.
          my $separator_found = (0 + ((!($skip_separator)) | ($found_separator)));
          if ($separator_found) {
            # Put back into array. We'll handle this as not-an-option.
            unshift (@{$args}, $cur_arg);
            last;
          }
          else {
            print {*STDERR} q{Unknown option encountered: } . $cur_arg . "; erroring out.\n";
            $error_detected = 1;
            last;
          }
        }
      }
      else {
        # Definitely not an option, add back to array.
        unshift (@{$args}, $cur_arg);
        last;
      }
    }
  }

  if (!($error_detected)) {
    $ret = [ $found_separator, $args ];
  }

  return $ret;
}

# Main function, no code outside of it shall be executed.
#
# Expects @ARGV to be passed in.
## no critic (NamingConventions::Capitalization)
sub Main {
## critic (NamingConventions::Capitalization)
  my @program_arguments = @_;
  my $error_detected = 0;
  my $found_separator = 0;

  Getopt::Long::Configure ('gnu_getopt', 'no_auto_abbrev', 'pass_through');

  my $help = 0;
  my $man = 0;
  my $debug = 0;
  Getopt::Long::GetOptionsFromArray (\@program_arguments, 'help|?|h' => \$help,
                                                          'man' => \$man,
                                                          'debug|d' => \$debug) or Pod::Usage::pod2usage (2);

  if ($help) {
    Pod::Usage::pod2usage (1);
  }

  if ($man) {
    Pod::Usage::pod2usage (-verbose => 2, -exitval => 3);
  }

  my $sanitized_options = undef;

  if (!($error_detected)) {
    $sanitized_options = sanitize_program_options (\@program_arguments, (!($found_separator)));

    if (!(defined ($sanitized_options))) {
      Pod::Usage::pod2usage (-exitval => 'NOEXIT');
      $error_detected = 4;
    }
  }

  my $infile = undef;

  if (!($error_detected)) {
    if ($debug) {
      print {*STDERR} 'Sanitized program options string as: ' . Data::Dumper::Dumper ($sanitized_options);
    }

    # The shift () operations here actually shift the outer array, not the
    # inner elements.
    # This can be very confusing.
    # The return value is an array consisting of a boolean value and an array
    # reference.
    #
    # Thus, shifting once returns the boolean value, while shifting again
    # returns the options array reference. Crucially, no shift () operation
    # here modifies the modified options array.
    $found_separator |= (0 + shift (@{$sanitized_options}));
    $sanitized_options = shift (@{$sanitized_options});
    @program_arguments = @{$sanitized_options};

    $infile = shift (@program_arguments);

    if (!(defined ($infile))) {
      print {*STDERR} "No input file given as an argument, aborting.\n";
      $error_detected = 5;
    }
  }

  if (!($error_detected)) {
    if ($debug) {
      print {*STDERR} 'Fetched input file as: ' . Data::Dumper::Dumper (\$infile);
    }

    if ((!(-f $infile)) || (!(-r $infile))) {
      print {*STDERR} "Input file \"${infile}\" is not a regular file or readable, aborting.\n";
      $error_detected = 6;
    }
  }

  # In general, we could process the input file line-by-line and cache it in
  # memory.
  # That's a bad idea, though, if the file to process is rather big.
  # Take the easy way out and write to a temporary file instead, which is
  # memory-friendly and lets the operating system handle caching and the like.
  my $temp_fh = undef;
  my $temp_name = undef;

  if (!($error_detected)) {
    ($temp_fh, $temp_name) = File::Temp::tempfile ('preprocTMP' . 'X' x 16, UNLINK => 1, TMPDIR => 1);
    $error_detected = parse_file ($infile, $temp_fh, $debug);

    if ($error_detected) {
      print {*STDERR} "Unable to parse input file \"${infile}\", aborting.\n";
      $error_detected += 6;
    }
  }

  if (!($error_detected)) {
    # Lastly, copy content of temporary file to original one, but make sure to
    # flush all data first.
    $temp_fh->autoflush ();

    File::Copy::copy ($temp_name, $infile);
  }

  return $error_detected;
}

__END__

=pod

=head1 NAME

preprocessor.pl - Simple text file preprocessor supporting conditionals

=head1 SYNOPSIS

=over

=item B<preprocessor.pl> B<--help>|B<-h>|B<-?>

=item B<preprocessor.pl> B<--man>

=item B<preprocessor.pl> [B<--debug>|B<-d>] [B<-->] I<file.in>

=back

=head1 DESCRIPTION

=for comment
Due to parser oddities, breaking a line during an L<dummy|text> formatter code
right after the separating pipe character will generate broken links.
Make sure to only break it during the description or, generally, on space
characters.
A workaround for this has been proposed in
https://github.com/Perl/perl5/issues/18305 .

B<preprocessor.pl> is a utility for preprocessing files with a simple,
hard-coded conditional syntax.

=head1 OPTIONS

=over 8

=item B<--help>|B<-?>|B<-h>

Print a brief help message and exits.

=item B<--man>

Prints the manual page and exits.

=item B<--debug>|B<-d>

Enables noisy debug output.

=back

=head2 CONDITIONAL SYNTAX

This program processes files by evaluating conditions and removing parts that
evaluate to false.

The supported parser tokens are:

=over 8

=item B<@?>

This token is equivalent to an C<if> statement. A condition that evaluates to a
boolean value must follow.

=item B<@!>

This token is equivalent to an C<else> statement. The rest of the line will be
ignored. To this effect, it B<must not> directly be chained with another C<if>
statement to create an C<elseif> statement.

If you need chaining, start another C<if> statement on a dedicated line after
the C<else> statement.

=item B<@/>

This token is equivalent to a C<fi> statement. The rest of the line will be
ignored. This statement terminates a conditional block.

=back

Conditional statements support the following operators:

=over 8

=item I<a> B<==> I<b>

This is a binary operator that checks its two operands for equality.

The semantics are as follows:

=over 2

=item I<a> and I<b> are numbers

The check is being done numerically. Whether arguments are numbers or strings
is determined via C<Scalar::Util::looks_like_number ()>.

=item otherwise

The check is being done using a string comparison. Note that the locale is
respected and influences the order of characters.

=back

=item I<a> B<!=> I<b>

This binary operator reverses the boolean value that would be computed using
the B<==> operator.

=back

=head1 EXAMPLES

Original content:

=over 2

 Constant data
 @? 10 == 1
 Dynamic data
 @? ui != gui
 is this conditional?
 @!
 or this?
 @/ data discarded
 and what about this?
 @!
 Generated data
 @? help == me
 sed -e 's/a/b/g' some.file
 @!
 sed -e 's/b/a/g' some.file
 @/
 end: data
 end: transmission
 @/
 end: all

=back

Preprocessed content:

=over 2

 Constant data
 Generated data
 sed -e 's/b/a/g' some.file
 end: data
 end: transmission
 end: all

=back

=head1 AUTHOR

This manual has been written by
Mihai Moldovan L<E<lt>ionic@ionic.deE<gt>|mailto:ionic@ionic.de> for the X2Go
project (L<https://www.x2go.org|https://www.x2go.org>).

=cut
