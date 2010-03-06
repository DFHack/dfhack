#!/usr/bin/perl

#local $/ = "\n\n";
while(<>){
    if(/([0-9A-Z]+).*?([a-z]+_.+?)st\n/s){
        print "<class vtable=\"0x$1\" name=\"$2\" />\n";
    }
}