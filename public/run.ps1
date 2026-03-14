$OK = 0
$KO = 0
$in = Get-ChildItem -Path test*.in -Name
foreach ($i in $in) {
 $test = $i.Replace(".in", "")
 $out = $i.Replace(".in", ".out")
 $res = $i.Replace(".in", ".res")
 $diff = $i.Replace(".in", ".diff")
 Get-Content -Path $i | ..\proj | Set-Content -Path $res
 $hash = (Get-FileHash $out -Algorithm MD5).Hash -eq (Get-FileHash $res -Algorithm MD5).Hash
# $result = Get-Content -Path $res
# $expected = Get-Content -Path $out
# if ( $expected -eq $result ) {
 if ( $hash ) {
  Write-Host ($test + " PASSED") -ForegroundColor Green
  $OK++
 } else {
  Write-Host ($test + " FAILED") -ForegroundColor Red
  $KO++
 }
}
if ( $KO -eq 0 ) {
  Write-Host "passed ALL tests" -ForegroundColor Green
} else {
  Write-Host "passed " $OK " tests in " ($OK + $KO) " tests" -ForegroundColor Blue
}
