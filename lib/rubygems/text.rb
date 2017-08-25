require 'rubygems'

##
# A collection of text-wrangling methods

module Gem::Text

  ##
  # Remove any non-printable characters and make the text suitable for
  # printing.
  def clean_text(text)
    text.gsub(/[\000-\b\v-\f\016-\037\177]/, ".".freeze)
  end

  def truncate_text(text, description, max_length = 100_000)
    raise ArgumentError, "max_length must be positive" unless max_length > 0
    return text if text.size <= max_length
    "Truncating #{description} to #{max_length.to_s.reverse.gsub(/...(?=.)/,'\&,').reverse} characters:\n" + text[0, max_length]
  end

  ##
  # Wraps +text+ to +wrap+ characters and optionally indents by +indent+
  # characters

  def format_text(text, wrap, indent=0)
    result = []
    work = clean_text(text)

    while work.length > wrap do
      if work =~ /^(.{0,#{wrap}})[ \n]/ then
        result << $1.rstrip
        work.slice!(0, $&.length)
      else
        result << work.slice!(0, wrap)
      end
    end

    result << work if work.length.nonzero?
    result.join("\n").gsub(/^/, " " * indent)
  end

  # This code is based directly on the Text gem implementation
  # Returns a value representing the "cost" of transforming str1 into str2
  def levenshtein_distance str1, str2
    s = str1
    t = str2
    n = s.length
    m = t.length
    max = n/2

    return m if (0 == n)
    return n if (0 == m)
    return n if (n - m).abs > max

    d = (0..m).to_a
    x = nil

    n.times do |i|
      e = i+1

      m.times do |j|
        cost = (s[i] == t[j]) ? 0 : 1
        x = [
             d[j+1] + 1, # insertion
             e + 1,      # deletion
             d[j] + cost # substitution
            ].min
        d[j] = e
        e = x
      end

      d[m] = x
    end

    return x
  end
end

