#!/usr/bin/perl

$filename = $ARGV[0];

$newfilename = $filename;
$newfilename =~ s/\.txt/\.xml/gi;

$z = 0;
open (FILE,"<$filename");
foreach $_ (<FILE>)
{
   $file[$z] = $_;
   $z++;
}
close FILE;

open (FILE,">$newfilename");

print FILE "<?xml version=\"1.01\" encoding=\"UTF-8\"?><!DOCTYPE language>\n";

# get the name of the hl
@field = split /\"/, $file[0];
$hlname = @field[1];

# get the extensions of the hl
@field = split /File Extensions =/, $file[0];
@field = split / /, @field[1];

$ext = "";
$c = 1;
while (@field[$c] ne "")
{
  @field[$c] =~ s/\n//gi;
  @field[$c] =~ s/\r//gi;

  if (length(@field[$c]) > 0)
  {
    $ext .= "*.@field[$c];";
  }

  $c++;
}

# get the single line comment
@field = split /Line Comment =/, $file[0];
@field = split / /, @field[1];
$linecomment = @field[1];

# get the multi line comment
@field = split /Block Comment On =/, $file[0];
@field = split / /, @field[1];
$commentstart = @field[1];

@field = split /Block Comment Off =/, $file[0];
@field = split / /, @field[1];
$commentend = @field[1];

print FILE "<language name=\"$hlname\" extensions=\"$ext\" mimetype=\"\" casesensitive=\"0\">\n";
print FILE "  <highlighting>\n";

$i=0;
$s=0;
while ($i <= $z)
{
  if ($file[$i] =~ /\/C[1234567890]/)
  {
    $sections[$s] = $i;

    @field = split /\"/, $file[$sections[$s]];
    $listname[$s] = @field[1];
    if ($listname[$s] eq "")
    {
      $listname[$s] = "WordList$s";
    }
    else
    {
      $listname[$s] .= $s;
    }

    $s++;
  }
  $i++;
}

$n=0;
while (($n < $s) && ($file[$sections[$n]] ne ""))
{
  print FILE "    <list name=\"$listname[$n]\">\n";

  $t = $sections[$n]+1;

  $end = $sections[$n+1];
  if ($end eq "")
  {
    $end = $z+1;
  }

  while ($t < $end)
  {
    @field = split / /, $file[$t];

    $c = 0;
    while (length(@field[$c]) > 0)
    {
      @field[$c] =~ s/\n//gi;
      @field[$c] =~ s/\r//gi;

      @field[$c] =~ s/</\&lt;/gi;
      @field[$c] =~ s/>/\&gt;/gi;
      @field[$c] =~ s/\&/\&amp;/gi;
      @field[$c] =~ s/\"/\&quot;/gi;

      if (length(@field[$c]) > 0)
      {
        print FILE "      <item>@field[$c]</item>\n";
      }

      $c++;
    }

    $t++;
  }

  print FILE "    </list>\n";
  $n++;
}

print FILE "    <contexts>\n";
print FILE "      <context name=\"Normal\" attribute=\"0\" lineEndContext=\"0\">\n";

$n=0;
while (($n < $s) && ($file[$sections[$n]] ne ""))
{
  $str = $n+1;
  print FILE "        <keyword attribute=\"$str\" context=\"0\" String=\"$listname[$n]\" />\n";
  $n++;
}

print FILE "      </context>\n";
print FILE "    </contexts>\n";
print FILE "    <itemDatas>\n";
print FILE "      <itemData name=\"Normal\" defStyleNum=\"dsNormal\"/>\n";

$n=0;
while (($n < $s) && ($file[$sections[$n]] ne ""))
{
  print FILE "      <itemData name=\"$listname[$n]\" defStyleNum=\"dsNormal\" color=\"#ff0000\" selColor=\"#ff0000\" bold=\"1\" italic=\"0\" />\n";
  $n++;
}

print FILE "    </itemDatas>\n";
print FILE "  </highlighting>\n";

if (($linecomment ne "") || ($commentstart ne "") && ($commentend ne ""))
{
  print FILE "  <general>\n";
  print FILE "    <comments>\n";

  if ($linecomment ne "")
  {
    print FILE "      <comment name=\"singleLine\" start=\"$linecomment\" />\n";
  }

  if (($commentstart ne "") && ($commentend ne ""))
  {
    print FILE "      <comment name=\"multiLine\" start=\"$commentstart\" end=\"$commentend\" />\n";
  }

  print FILE "    </comments>\n";
  print FILE "  </general>\n";
}

print FILE "</language>\n";

close FILE;
