#!/usr/bin/perl -w


my $arg = qr/[^,]*/;

while(<>) {
  chomp;
  if (/^HAMMER_FN_DECL_NOARG\(([^,]*), ([^,]*)\);/) {
    print "$1 $2(void);\n";
    print "$1 $2__m(HAllocator* mm__);\n";
  } elsif (/^HAMMER_FN_DECL\(([^,]*), ([^,]*), ([^)]*)\);/) {
    print "$1 $2($3);\n";
    print "$1 $2__m(HAllocator* mm__, $3);\n";
  } elsif (/^HAMMER_FN_DECL_VARARGS_ATTR\((__attribute__\(\([^)]*\)\)), ([^,]*), ([^,]*), ([^)]*)\);/) {
    print "$2 $3($4, ...);\n";
    print "$2 $3__m(HAllocator *mm__, $4, ...);\n";
    print "$2 $3__a(void* args);\n";
    print "$2 $3__ma(HAllocator* mm__, void* args);\n";
  } elsif (/^HAMMER_FN_DECL/) {
    print "\e[1;31m!!!\e[0m " . $_ . "\n";
  }
}
