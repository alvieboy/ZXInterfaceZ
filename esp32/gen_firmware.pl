#!/bin/perl

use JSON;
use Archive::Tar;
use Digest::SHA;
use File::Slurp;
use strict;
use warnings;

my $argname = undef;

my %params;
my @files;
my @file_contents;

sub compress_rle
{
    my ($infile) = @_;
    my $block;
    my $contents;
    my $digest = Digest::SHA->new('SHA256');
    
    open(my $fh, '-|', "firmwareupdate/rle",$infile,"-") or die;
    while (1) {
        my $len = read($fh, $block, 4096);
        die unless defined $len or $len<0;
        last if $len==0;
        $digest->add($block);
        $contents.=$block;
    }
    
    $_[1] = $digest->hexdigest;
    #print STDERR "RLE compressed ",length($contents),"\n";
    return $contents;
}

sub parse_file_entry
{
    my ($desc) = @_;
    my ($filename, $type, $compress) = split(/:/,$desc);
    my $contents;
    my $rle_sha;
    my $in_sha;
    my $thisfile = {};
    
    die "missing compression in '$desc'" unless defined $compress;

    # calculate in sha   
    my $sha = Digest::SHA->new("SHA256");
    $sha->addfile($filename) or die "Cannot read file";
    $in_sha = $sha->hexdigest;

    my @stat_a = stat($filename);
    my $flen = $stat_a[7] or die "Cannot get file length";
    $thisfile->{'size'} = $flen;
    
    if ($compress eq 'rle') {
        $contents = compress_rle($filename, $rle_sha);
        $thisfile->{'compress'} = 'rle';
        $thisfile->{'compressed_sha256'} = $rle_sha;
        $thisfile->{'sha256'} = $in_sha;
    }
    
    if ($compress eq 'none') {
        $contents = read_file($filename);
        $thisfile->{'compress'} = 'none';
        $thisfile->{'sha256'} = $in_sha;
    }
    
    die "Cannot read contents" unless defined $contents;
    die "Invalid compression scheme" unless exists $thisfile->{'compress'};

    # Mock up file name.
    
    # Strip path
    $filename =~ s/^.*\/([^\/]+)$/${1}/;
    
    #print $filename;
    
    # Hash content.
    $thisfile->{'name'} = $filename;
    $thisfile->{'type'} = $type;
    
    
    push(@files, $thisfile);
    push(@file_contents, $contents);
}

while (my $p = shift @ARGV) {
    if (defined $argname) {
        $params{$argname}=$p;
        undef $argname;
        next;
    }
    # Option with value
    if ($p=~/^\-\-([^=]+)=(.*)$/) {
        $params{$1}=$2;
        next;
    }
    # Option w/o value
    if ($p=~/^\-\-(.*)$/) {
        $argname=$1;
        next;
    }
    parse_file_entry($p);
}
die "Missing value for $argname" if defined $argname;


die "Missing compat" unless exists($params{'compat'});
die "Missing output" unless exists($params{'output'});


my $manifest = {
    'update' => {
        'version' => '1.1',
        'compat' => $params{'compat'},
        'files' => \@files
    }
};

my $tar = new Archive::Tar;

$tar->add_data("MANIFEST.json", encode_json($manifest));

foreach my $file (@files) {
    $tar->add_data($file->{'name'}, shift @file_contents);
}

$tar->write($params{'output'}) or die "Cannot write output file";

print STDERR $params{'output'}, " generated.\n";
