#!/gemini/bin/alpha/perl

# $Header$
#
# Convert dinotrace.txt to dinodoc.h

print "\nchar dinodoc[] = \"\\\n";

$line = 0;

while(<STDIN>) {
    $buf = $_;
    chop $buf;
    $buf =~ s/\\/{backslash}/g;
    $buf =~ s/{backslash}/\\\\/g;
    $buf =~ s/\"/\\\"/g;
    print "$buf\\n\\\n";
    
    $line++;
    if ($line > 30) {
	print "\"\n\"";
        $line = 0;
    }
}

print "\";\n\n";

